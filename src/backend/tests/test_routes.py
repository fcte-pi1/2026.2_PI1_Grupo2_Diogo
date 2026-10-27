from datetime import datetime
import pytest

@pytest.fixture
def active_execution():
    return {
        "id": 847,
        "labirinto_id": 1,
        "iniciada_em": datetime(2026, 10, 25, 10, 0),
        "finalizada_em": None,
        "tempo_total_ms": None,
        "status": "em_execucao",
        "resultado": None,
        "velocidade_media_cm_s": None,
        "criado_em": datetime(2026, 10, 25, 10, 0),
        "algoritmo": "Flood Fill",
    }

def test_active_session_returns_200_and_session(unit_client, active_execution):
    client, db = unit_client
    db.execute.return_value.mappings.return_value.first.return_value = active_execution
    r = client.get("/api/sessions/active")
    assert r.status_code == 200
    data = r.json()
    assert data is not None
    assert data["id"] == "847"
    assert data["labirinto_id"] == "1"
    assert data["algorithm"] == "Flood Fill"
    assert data["status"] == "em_execucao"

def test_active_session_returns_null_when_no_session(unit_client):
    client, db = unit_client
    db.execute.return_value.mappings.return_value.first.return_value = None
    r = client.get("/api/sessions/active")
    assert r.status_code == 200
    assert r.json() is None


def test_active_session_shape_matches_model(unit_client, active_execution):
    client, db = unit_client
    db.execute.return_value.mappings.return_value.first.return_value = active_execution
    r = client.get("/api/sessions/active")
    assert r.status_code == 200
    data = r.json()
    assert set(data.keys()) == {
        "id",
        "labirinto_id",
        "iniciada_em",
        "finalizada_em",
        "tempo_total_ms",
        "status",
        "resultado",
        "velocidade_media_cm_s",
        "criado_em",
        "algorithm",
    }

@pytest.mark.parametrize("tipo", ["4x4", "8x4", "12x4"])
def test_start_execution_returns_201_and_session(unit_client, active_execution, tipo):
    client, db = unit_client
    result = db.execute.return_value
    result.scalar_one_or_none.return_value = None
    result.scalar_one.return_value = 1
    result.mappings.return_value.one.return_value = active_execution
    r = client.post("/api/sessions", json={"tipo": tipo})
    assert r.status_code == 201
    assert r.json()["id"] == "847"
    assert r.json()["status"] == "em_execucao"
    db.commit.assert_called_once()

def test_start_execution_returns_409_when_session_is_active(unit_client):
    client, db = unit_client
    db.execute.return_value.scalar_one_or_none.return_value = 847
    r = client.post("/api/sessions", json={"tipo": "4x4"})
    assert r.status_code == 409
    db.commit.assert_not_called()

def test_finish_execution_returns_404_for_unknown_session(unit_client):
    client, db = unit_client
    db.execute.return_value.mappings.return_value.first.return_value = None
    r = client.post(
        "/api/sessions/999/finish",
        json={"status": "interrompido"},
    )
    assert r.status_code == 404
    db.commit.assert_not_called()
def test_start_execution_rejects_invalid_maze_type(unit_client):
    client, db = unit_client
    r = client.post("/api/sessions", json={"tipo": "16x16"})
    assert r.status_code == 422
    db.execute.assert_not_called()
    db.commit.assert_not_called()

@pytest.mark.parametrize("status, outcome", [
    ("concluido", "sucesso"),
    ("interrompido", "falha"),
])
def test_finish_execution_returns_200_and_result(unit_client, status, outcome):
    client, db = unit_client
    db.execute.return_value.mappings.return_value.first.return_value = {
        "id": 847,
        "status": status,
        "resultado": outcome,
        "finalizada_em": datetime(2026, 10, 25, 10, 0, 42),
        "tempo_total_ms": 42000,
    }
    r = client.post(
        "/api/sessions/847/finish",
        json={"status": status, "resultado": outcome},
    )
    assert r.status_code == 200
    assert r.json() == {
        "id": 847,
        "status": status,
        "resultado": outcome,
        "finalizada_em": "2026-10-25T10:00:42",
        "tempo_total_ms": 42000,
    }
    assert db.execute.call_args.args[1] == {
        "id": 847,
        "status": status,
        "resultado": outcome,
    }
    db.commit.assert_called_once()

@pytest.mark.parametrize("payload", [
    {"status": "em_execucao"},
    {"status": "concluido", "resultado": "invalido"},
])
def test_finish_execution_rejects_invalid_payload(unit_client, payload):
    client, db = unit_client
    r = client.post("/api/sessions/847/finish", json=payload)
    assert r.status_code == 422
    db.execute.assert_not_called()
    db.commit.assert_not_called()
