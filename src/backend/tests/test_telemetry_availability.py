"""RNF04: recepção, perda de sinal e recuperação, sem acessar banco real."""
from datetime import datetime, timedelta, timezone

import pytest
from fastapi.testclient import TestClient
from sqlalchemy.exc import OperationalError

from app.core.config import settings
from app.core.database import get_db
from app.main import app
from app.routes import telemetry as routes
from app.services.telemetry_service import TelemetryService


class Result:
    def __init__(self, value):
        self.value = value

    def mappings(self):
        return self

    def first(self):
        return self.value

    def scalar_one(self):
        return self.value


class Database:
    def __init__(self):
        self.active = False
        self.fail = False
        self.fail_commit = False
        self.commits = 0
        self.rollbacks = 0
        self.inserts = []
        self.last_telemetry = None
        self.last_movement = None

    def execute(self, query, params=None):
        if self.fail:
            raise OperationalError("test", {}, Exception("Banco indisponível"))
        sql = str(query)
        if "INSERT INTO" in sql:
            self.inserts.append((sql, params))
            return Result(len(self.inserts))
        if "FROM execucao" in sql:
            return Result({"id": 2, "elapsed_ms": 1500} if self.active else None)
        if "FROM amostra_telemetria WHERE" in sql:
            return Result(self.last_telemetry)
        if "FROM amostra_movimento WHERE" in sql:
            return Result(self.last_movement)
        return Result(None)

    def commit(self):
        if self.fail_commit:
            raise OperationalError("test", {}, Exception("Commit indisponível"))
        self.commits += 1

    def rollback(self):
        self.rollbacks += 1

    def __enter__(self):
        return self

    def __exit__(self, *args):
        if args[0] is not None:
            self.rollback()


@pytest.fixture
def environment(monkeypatch):
    service = TelemetryService()
    db = Database()
    monkeypatch.setattr(routes, "telemetry_service", service)
    monkeypatch.setattr(routes, "SessionLocal", lambda: db)
    monkeypatch.setattr(settings, "TELEMETRY_UPDATE_INTERVAL_MS", 10)
    previous = app.dependency_overrides.copy()
    app.dependency_overrides[get_db] = lambda: db
    try:
        with TestClient(app) as client:
            yield client, service, db
    finally:
        app.dependency_overrides.clear()
        app.dependency_overrides.update(previous)


def packet(offset_ms=0, battery=87):
    return {
        "position": {"row": 1, "col": 2},
        "metrics": {"speed_cm_s": 18, "battery_pct": battery, "rpmLeft": 120, "rpmRight": 140},
        "movement": {"giro_x_deg_s": 1, "giro_y_deg_s": 2, "giro_z_deg_s": 3},
        "generated_at": (datetime.now(timezone.utc) + timedelta(milliseconds=offset_ms)).isoformat(),
    }


def test_packets_before_a_race_enable_monitoring_without_writing(environment):
    client, _, db = environment
    response = client.post("/api/telemetry", json=packet())
    assert response.json() == {"status": "received", "saved": False, "execucao_id": None}
    current = client.get("/api/telemetry/current").json()
    assert current["connected"] is True
    assert current["metrics"]["rpmLeft"] == 120
    assert current["metrics"]["rpmRight"] == 140
    assert current["movement"]["giro_z_deg_s"] == 3
    assert not db.inserts


@pytest.mark.parametrize("position", [
    {"position": {"row": 2, "col": 3}},
    {"row": 2, "col": 3},
    {"pos_y": 2, "pos_x": 3},
])
def test_flat_esp_packets_preserve_supported_position_formats(environment, position):
    client, _, db = environment
    db.active = True
    response = client.post("/api/telemetry", json={
        "bateria": 80, "velocidade": 12.5, "rpm_motor": 300, **position,
    })
    assert response.json()["saved"] is True
    assert db.inserts[1][1]["pos_x"] == 3
    assert db.inserts[1][1]["pos_y"] == 2
    assert client.get("/api/telemetry/current").json()["position"] == {"row": 2, "col": 3}


def test_legacy_packet_without_position_keeps_the_last_stored_cell(environment):
    client, _, db = environment
    db.active = True
    db.last_movement = {
        "pos_x": 3, "pos_y": 2, "giro_x_deg_s": 0, "giro_y_deg_s": 0, "giro_z_deg_s": 0,
    }
    response = client.post("/api/telemetry", json={"bateria": 80, "velocidade": 12.5, "rpm_motor": 300})
    assert response.json()["saved"] is True
    assert db.inserts[1][1]["pos_x"] == 3
    assert db.inserts[1][1]["pos_y"] == 2
    assert client.get("/api/telemetry/current").json()["position"] == {"row": 2, "col": 3}


def test_loss_preserves_readings_and_new_packet_restores_connection(environment):
    client, service, _ = environment
    client.post("/api/telemetry", json=packet())
    service._last_esp_received_at -= timedelta(seconds=6)
    lost = client.get("/api/telemetry/current").json()
    assert lost["connected"] is False
    assert lost["stale"] is True
    assert lost["metrics"]["battery_pct"] == 87
    client.post("/api/telemetry", json=packet(1, battery=86))
    resumed = client.get("/api/telemetry/current").json()
    assert resumed["connected"] is True
    assert resumed["stale"] is False
    assert resumed["metrics"]["battery_pct"] == 86


def test_live_data_survives_database_failure_without_claiming_it_was_saved(environment):
    client, _, db = environment
    db.fail = True
    response = client.post("/api/telemetry", json=packet())
    assert response.status_code == 503
    current = client.get("/api/telemetry/current").json()
    assert current["has_data"] is True
    assert current["metrics"]["battery_pct"] == 87
    assert db.rollbacks == 1
    assert db.commits == 0


def test_empty_unavailable_database_still_returns_connection_state(environment):
    client, _, db = environment
    db.fail = True
    response = client.get("/api/telemetry/current")
    assert response.status_code == 200
    assert response.json()["connected"] is False
    assert response.json()["metrics"] is None


def test_failed_transaction_does_not_report_saved(environment):
    client, _, db = environment
    db.active = True
    db.fail_commit = True
    assert client.post("/api/telemetry", json=packet()).status_code == 503
    assert db.commits == 0
    assert db.rollbacks == 1
    assert client.get("/api/telemetry/current").json()["telemetry_sample_id"] is None


def test_out_of_order_packet_does_not_replace_or_persist_old_values(environment):
    client, _, db = environment
    db.active = True
    client.post("/api/telemetry", json=packet(0, battery=86))
    response = client.post("/api/telemetry", json=packet(-10000, battery=99))
    assert response.json()["status"] == "ignored"
    assert len(db.inserts) == 2
    assert client.get("/api/telemetry/current").json()["metrics"]["battery_pct"] == 86


def test_active_race_persists_both_wheels_and_movement(environment):
    client, _, db = environment
    db.active = True
    response = client.post("/api/telemetry", json=packet())
    assert response.json()["saved"] is True
    assert db.inserts[0][1]["rpm_left"] == 120
    assert db.inserts[0][1]["rpm_right"] == 140
    assert db.inserts[1][1]["gyro_z"] == 3
    assert db.commits == 1
    assert client.get("/api/telemetry/current").json()["execucao_id"] == 2


def test_unchanged_values_refresh_live_reading_without_an_extra_write(environment):
    client, _, db = environment
    db.active = True
    client.post("/api/telemetry", json=packet())
    db.last_telemetry = {
        "tempo_relativo_ms": 1500, "nivel_bateria_pct": 87,
        "velocidade_instantanea_cm_s": 18, "rpm_motor_esq": 120, "rpm_motor_dir": 140,
    }
    db.last_movement = {
        "pos_x": 2, "pos_y": 1, "giro_x_deg_s": 1, "giro_y_deg_s": 2, "giro_z_deg_s": 3,
    }
    latest = packet(1)
    response = client.post("/api/telemetry", json=latest)
    assert response.json()["status"] == "unchanged"
    assert db.commits == 1
    current = client.get("/api/telemetry/current").json()
    assert datetime.fromisoformat(current["generated_at"]) == datetime.fromisoformat(latest["generated_at"])
    assert current["connected"] is True
    assert current["stale"] is False


def test_websocket_handles_invalid_packet_then_database_failure_and_recovery(environment):
    client, _, db = environment
    with client.websocket_connect("/ws/telemetry") as socket:
        socket.send_text("invalid json")
        assert socket.receive_json()["status"] == "error"
        db.fail = True
        socket.send_json(packet())
        assert socket.receive_json()["code"] == 503
        db.fail = False
        db.active = True
        socket.send_json(packet(1, battery=86))
        assert socket.receive_json()["saved"] is True


def test_new_subscriber_receives_latest_reading_after_reconnection(environment):
    client, _, _ = environment
    client.post("/api/telemetry", json=packet())
    with client.websocket_connect("/ws/telemetry/current") as socket:
        assert socket.receive_json()["metrics"]["battery_pct"] == 87
    client.post("/api/telemetry", json=packet(1, battery=86))
    with client.websocket_connect("/ws/telemetry/current") as socket:
        assert socket.receive_json()["metrics"]["battery_pct"] == 86


def test_stream_recovers_when_database_returns_without_reopening_socket(environment):
    client, _, db = environment
    db.fail = True
    with client.websocket_connect("/ws/telemetry/current") as socket:
        assert socket.receive_json()["has_data"] is False
        db.fail = False
        client.post("/api/telemetry", json=packet())
        for _ in range(10):
            snapshot = socket.receive_json()
            if snapshot["has_data"]:
                break
        assert snapshot["connected"] is True
        assert snapshot["metrics"]["battery_pct"] == 87
