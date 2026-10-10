import pytest
from fastapi.testclient import TestClient

from app.main import app
from app.mock import state
from app.mock.seed import EXECUTION_ID

client = TestClient(app)

def test_maze_returns_grid():
    r = client.get(f"/api/sessions/{EXECUTION_ID}/maze")
    assert r.status_code == 200
    body = r.json()

    n = body["size"]
    assert len(body["grid"]) == n
    assert all(len(row) == n for row in body["grid"])

    # start e goal dentro do grid
    assert 0 <= body["start"]["row"] < n
    assert 0 <= body["start"]["col"] < n
    assert 0 <= body["goal"]["row"] < n
    assert 0 <= body["goal"]["col"] < n
    assert body["start"] != body["goal"]

    for row in body["grid"]:
        for cell in row:
            assert set(cell.keys()) =={
                "parede_norte", "parede_sul", "parede_leste", "parede_oeste",
            }
            assert all(isinstance(v, bool) for v in cell.values())

def test_path_returns_trajectory():
    r = client.get(f"/api/sessions/{EXECUTION_ID}/path")
    assert r.status_code == 200
    assert len(r.json()) == len(state.path)

@pytest.mark.parametrize("resource", ["maze", "path", "metrics", "events"])
def test_returns_404_for_unknown_session(resource):
    r = client.get(f"/api/sessions/unknown_session/{resource}")
    assert r.status_code == 404
    assert r.json() == {"detail": "No active session found"}


@pytest.mark.parametrize("resource", ["maze", "path", "metrics", "events"])
def test_path_returns_404_when_no_session(resource):
    saved = state.execution
    state.execution = None
    try:
        r = client.get(f"/api/sessions/{EXECUTION_ID}/{resource}")
        assert r.status_code == 404
        assert r.json() == {"detail": "No active session found"}
    finally:
        state.execution = saved

def test_metrics_returns_current_metrics():
    r = client.get(f"/api/sessions/{EXECUTION_ID}/metrics")
    assert r.status_code == 200
    assert set(r.json().keys()) == {"speed_cm_s", "rpm", "battery_pct"}

def test_events_returns_log():
    r = client.get(f"/api/sessions/{EXECUTION_ID}/events")
    assert r.status_code == 200
    assert len(r.json()) == len(state.events)
    