from fastapi.testclient import TestClient
from app.main import app

client = TestClient(app)


def test_root():
    r = client.get('/')
    assert r.status_code == 200
    assert r.json()["message"] == "Micromouse API funcionando"

def test_health_conectado():
    r = client.get("/health")
    assert r.status_code == 200
    assert r.json()["database"] == "connected"

def test_cors_libera_origem_do_frontend():
    r = client.get("/health", headers={"Origin": "http://localhost:5173"})
    assert r.headers["access-control-allow-origin"] == "http://localhost:5173"

def test_cors_libera_origem_0_0_0_0_na_rota_de_sessao_ativa(unit_client):
    test_client, db = unit_client
    db.execute.return_value.mappings.return_value.first.return_value = None

    r = test_client.get(
        "/api/sessions/active",
        headers={"Origin": "http://0.0.0.0:5173"},
    )

    assert r.status_code == 200
    assert r.headers["access-control-allow-origin"] == "http://0.0.0.0:5173"

def test_sessao_inexistente_retorna_404():
    r = client.get("/api/sessions/-99999")
    assert r.status_code == 404
