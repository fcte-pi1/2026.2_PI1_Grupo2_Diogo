from datetime import datetime
import pytest

@pytest.fixture
def history_row():
    return {
        "id": 847,
        "labirinto_id": 1,
        "labirinto_nome": "Labirinto 4x4",
        "labirinto_tipo": "4x4",
        "iniciada_em": datetime(2026, 10, 25, 10, 0),
        "finalizada_em": datetime(2026, 10, 25, 10, 0, 42),
        "tempo_total_ms": 42000,
        "status": "concluido",
        "resultado": "sucesso",
        "velocidade_media_cm_s": 18.0,
        "algoritmo": "Flood Fill",
        "rpm": 120.0,
        "bateria_consumida_pct": 5.0,
        "quantidade_celulas_visitadas": 7,
    }

def test_history_returns_200_and_default_pagination(unit_client, history_row):
    client, db = unit_client
    result = db.execute.return_value
    result.scalar_one.return_value = 1
    result.mappings.return_value.all.return_value = [history_row]
    r = client.get("/api/sessions/history")
    assert r.status_code == 200
    assert r.json() == {
        "total": 1,
        "limit": 20,
        "offset": 0,
        "items": [{
            **history_row,
            "id": "847",
            "labirinto_id": "1",
            "iniciada_em": "2026-10-25T10:00:00",
            "finalizada_em": "2026-10-25T10:00:42",
        }],
    }
    assert db.execute.call_count == 2
    db.commit.assert_not_called()

def test_history_returns_empty_page_when_no_executions(unit_client):
    client, db = unit_client
    result = db.execute.return_value
    result.scalar_one.return_value = 0
    result.mappings.return_value.all.return_value = []
    r = client.get("/api/sessions/history")
    assert r.status_code == 200
    assert r.json() == {
        "total": 0,
        "limit": 20,
        "offset": 0,
        "items": [],
    }

def test_history_preserves_null_fields_for_active_execution(unit_client, history_row):
    client, db = unit_client
    nullable_fields = (
        "finalizada_em",
        "tempo_total_ms",
        "resultado",
        "velocidade_media_cm_s",
        "quantidade_celulas_visitadas",
    )
    history_row["status"] = "em_execucao"
    for field in nullable_fields:
        history_row[field] = None
    result = db.execute.return_value
    result.scalar_one.return_value = 1
    result.mappings.return_value.all.return_value = [history_row]
    r = client.get("/api/sessions/history")
    assert r.status_code == 200
    item = r.json()["items"][0]
    assert item["status"] == "em_execucao"
    for field in nullable_fields:
        assert item[field] is None

@pytest.mark.parametrize("filters", [
    pytest.param({"labirinto_id": 1}, id="maze"),
    pytest.param({"status": "em_execucao"}, id="active"),
    pytest.param({"status": "concluido"}, id="completed"),
    pytest.param({"status": "interrompido"}, id="interrupted"),
    pytest.param({"resultado": "sucesso"}, id="success"),
    pytest.param({"resultado": "falha"}, id="failure"),
    pytest.param(
        {"labirinto_id": 1, "status": "concluido", "resultado": "sucesso"},
        id="combined",
    ),
])
def test_history_passes_filters_to_both_database_queries(unit_client, filters):
    client, db = unit_client
    result = db.execute.return_value
    result.scalar_one.return_value = 0
    result.mappings.return_value.all.return_value = []
    r = client.get("/api/sessions/history", params=filters)
    assert r.status_code == 200
    count_call, page_call = db.execute.call_args_list
    assert count_call.args[1] == filters
    assert page_call.args[1] == {**filters, "limit": 20, "offset": 0}
    for query_call in (count_call, page_call):
        sql = str(query_call.args[0])
        for field in filters:
            assert f"e.{field} = :{field}" in sql

def test_history_keeps_total_independent_of_pagination(unit_client, history_row):
    client, db = unit_client
    result = db.execute.return_value
    result.scalar_one.return_value = 7
    result.mappings.return_value.all.return_value = [history_row]
    r = client.get("/api/sessions/history", params={"limit": 1, "offset": 6})
    assert r.status_code == 200
    data = r.json()
    assert data["total"] == 7
    assert data["limit"] == 1
    assert data["offset"] == 6
    assert len(data["items"]) == 1
    assert data["items"][0]["id"] == "847"
    count_call, page_call = db.execute.call_args_list
    assert count_call.args[1] == {}
    assert page_call.args[1] == {"limit": 1, "offset": 6}

def test_history_returns_empty_page_when_offset_exceeds_total(unit_client):
    client, db = unit_client
    result = db.execute.return_value
    result.scalar_one.return_value = 7
    result.mappings.return_value.all.return_value = []
    r = client.get("/api/sessions/history", params={"limit": 2, "offset": 10})
    assert r.status_code == 200
    assert r.json() == {
        "total": 7,
        "limit": 2,
        "offset": 10,
        "items": [],
    }

@pytest.mark.parametrize("limit", [1, 100])
def test_history_accepts_limit_boundaries(unit_client, limit):
    client, db = unit_client
    result = db.execute.return_value
    result.scalar_one.return_value = 0
    result.mappings.return_value.all.return_value = []
    r = client.get("/api/sessions/history", params={"limit": limit})
    assert r.status_code == 200
    assert r.json()["limit"] == limit

@pytest.mark.parametrize("params", [
    pytest.param({"limit": 0}, id="zero-limit"),
    pytest.param({"limit": -1}, id="negative-limit"),
    pytest.param({"limit": 101}, id="limit-above-maximum"),
    pytest.param({"limit": "abc"}, id="non-integer-limit"),
    pytest.param({"offset": -1}, id="negative-offset"),
    pytest.param({"offset": "abc"}, id="non-integer-offset"),
    pytest.param({"labirinto_id": "abc"}, id="non-integer-maze-id"),
    pytest.param({"status": "invalid"}, id="invalid-status"),
    pytest.param({"resultado": "invalid"}, id="invalid-result"),
])
def test_history_rejects_invalid_query_parameters(unit_client, params):
    client, db = unit_client
    r = client.get("/api/sessions/history", params=params)
    assert r.status_code == 422
    db.execute.assert_not_called()
    db.commit.assert_not_called()
