from datetime import datetime, timezone

import pytest

from app.routes.telemetry import get_current_telemetry
from app.services.telemetry_service import TelemetryService

class FakeResult:
    def __init__(self, sample):
        self.sample = sample

    def mappings(self):
        return self

    def first(self):
        return self.sample

class FakeDB:
    def __init__(self, sample):
        self.sample = sample
    def execute(self, query):
        assert "FROM amostra_telemetria" in str(query)
        return FakeResult(self.sample)

def test_current_telemetry_returns_last_database_sample(esp_service):
    esp_service.mark_esp_packet_received()
    sample = {
        "telemetry_sample_id": 23,
        "execucao_id": 2,
        "tempo_relativo_ms": 1500,
        "nivel_bateria_pct": 87.0,
        "consumo_eletrico_ma": 0.0,
        "velocidade_instantanea_cm_s": 0.325,
        "rpm_motor_esq": 120,
        "rpm_motor_dir": 120,
        "giro_x_deg_s": 0.0,
        "giro_y_deg_s": 0.0,
        "giro_z_deg_s": 0.0,
        "pos_x": 3,
        "pos_y": 1,
        "generated_at": datetime.now(timezone.utc),
    }

    result = get_current_telemetry(db=FakeDB(sample))
    assert result["execucao_id"] == 2
    assert result["telemetry_sample_id"] == 23
    assert result["position"] == {"row": 1, "col": 3}
    assert result["metrics"]["rpm"] == 120
    assert result["connected"] is True
    assert result["stale"] is False

def test_empty_database_returns_connection_status_without_readings(esp_service):
    esp_service.mark_esp_packet_received()
    result = get_current_telemetry(db=FakeDB(None))
    assert result["connected"] is True
    assert result["has_data"] is False
    assert result["metrics"] is None
    assert result["stale"] is True

@pytest.fixture(autouse=True)
def esp_service(monkeypatch):
    service = TelemetryService()
    monkeypatch.setattr("app.routes.telemetry.telemetry_service", service)
    return service

@pytest.fixture
def telemetry_payload():
    return {
        "position": {"row": 1, "col": 2},
        "metrics": {
            "speed_cm_s": 18.0,
            "rpm": 120,
            "battery_pct": 87.0,
        },
        "events": [],
        "generated_at": "2026-10-25T10:00:00Z",
    }

@pytest.fixture
def saved_samples():
    return (
        {
            "tempo_relativo_ms": 1000,
            "nivel_bateria_pct": 87.0,
            "velocidade_instantanea_cm_s": 18.0,
            "rpm_motor_esq": 120,
        },
        {"pos_x": 2, "pos_y": 1},
    )
