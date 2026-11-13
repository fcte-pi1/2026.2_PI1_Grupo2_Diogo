"""Testes de integração do endpoint POST /api/telemetry.

Estes testes fazem INSERT/DELETE de verdade. Rode-os num banco de teste
(ex.: micromouse_test), nunca no banco de desenvolvimento.
"""
import pytest
from fastapi.testclient import TestClient
from sqlalchemy import text

from app.core.config import settings
from app.core.database import get_db  # ajuste se o get_db estiver em outro módulo
from app.main import app

client = TestClient(app)

URL = "/api/telemetry"  # prefixo /api + prefixo do router; confira no telemetry.py

# Payload no formato EspTelemetryInput (sem posição)
ESP = {"connected": True, "battery_pct": 80.0, "speed_cm_s": 12.5, "rpm": 300}

# Payload no formato TelemetryData (com posição).
# Ajuste para os campos reais do schema: se ele exigir mais campos, o teste
# retorna 422 e a mensagem do erro mostra o que falta.
TELEMETRY_DATA = {
    "metrics": {"battery_pct": 70.0, "speed_cm_s": 20.0, "rpm": 450},
    "position": {"col": 3, "row": 5},
}


# --------------------------------------------------------------------------
# Proteção: não roda contra o banco de desenvolvimento
# --------------------------------------------------------------------------
@pytest.fixture(autouse=True)
def somente_banco_de_teste():
    nome_do_banco = getattr(settings, "POSTGRES_DB", None)
    if isinstance(nome_do_banco, str) and "test" not in nome_do_banco.lower():
        pytest.skip(
            f"Banco '{nome_do_banco}' não parece ser de teste. "
            "Aponte POSTGRES_DB para um banco como micromouse_test."
        )


# --------------------------------------------------------------------------
# Fixtures
# --------------------------------------------------------------------------
@pytest.fixture
def db():
    session = next(get_db())
    yield session
    session.close()


@pytest.fixture
def labirinto_id(db):
    existente = db.execute(text("SELECT id FROM labirinto LIMIT 1")).scalar()
    if existente is not None:
        yield existente
        return

    # Ajuste as colunas ao schema real de labirinto (pode haver outras NOT NULL)
    novo = db.execute(
        text("INSERT INTO labirinto (nome) VALUES ('labirinto de teste') RETURNING id")
    ).scalar_one()
    db.commit()
    yield novo
    db.execute(text("DELETE FROM labirinto WHERE id = :i"), {"i": novo})
    db.commit()


@pytest.fixture
def execucao_ativa(db, labirinto_id):
    exec_id = db.execute(
        text(
            "INSERT INTO execucao (status, iniciada_em, labirinto_id) "
            "VALUES ('em_execucao', LOCALTIMESTAMP, :lab) RETURNING id"
        ),
        {"lab": labirinto_id},
    ).scalar_one()
    db.commit()

    yield exec_id

    db.rollback()
    for tabela in ("amostra_telemetria", "amostra_movimento"):
        db.execute(text(f"DELETE FROM {tabela} WHERE execucao_id = :i"), {"i": exec_id})
    db.execute(text("DELETE FROM execucao WHERE id = :i"), {"i": exec_id})
    db.commit()


def contar(db, tabela, exec_id):
    return db.execute(
        text(f"SELECT COUNT(*) FROM {tabela} WHERE execucao_id = :i"), {"i": exec_id}
    ).scalar_one()


# --------------------------------------------------------------------------
# Testes
# --------------------------------------------------------------------------
def test_sem_execucao_ativa_recebe_sem_gravar(db):
    ativa = db.execute(
        text("SELECT COUNT(*) FROM execucao WHERE status = 'em_execucao'")
    ).scalar_one()
    if ativa:
        pytest.skip("Existe execução ativa no banco; use um banco de teste limpo.")

    r = client.post(URL, json=ESP)

    assert r.status_code == 200
    assert r.json() == {"status": "received", "saved": False, "execucao_id": None}


def test_primeira_amostra_e_salva(db, execucao_ativa):
    r = client.post(URL, json=ESP)

    assert r.status_code == 200
    body = r.json()
    assert body["status"] == "saved"
    assert body["saved"] is True
    assert body["execucao_id"] == execucao_ativa
    assert "telemetry_sample_id" in body
    assert "movement_sample_id" in body
    assert contar(db, "amostra_telemetria", execucao_ativa) == 1
    assert contar(db, "amostra_movimento", execucao_ativa) == 1


def test_formato_esp_grava_posicao_zero(db, execucao_ativa):
    client.post(URL, json=ESP)

    pos = db.execute(
        text("SELECT pos_x, pos_y FROM amostra_movimento WHERE execucao_id = :i"),
        {"i": execucao_ativa},
    ).mappings().first()

    assert (pos["pos_x"], pos["pos_y"]) == (0, 0)


def test_formato_telemetry_data_grava_posicao(db, execucao_ativa):
    r = client.post(URL, json=TELEMETRY_DATA)

    assert r.status_code == 200, r.text
    assert r.json()["status"] == "saved"

    pos = db.execute(
        text("SELECT pos_x, pos_y FROM amostra_movimento WHERE execucao_id = :i"),
        {"i": execucao_ativa},
    ).mappings().first()
    assert (pos["pos_x"], pos["pos_y"]) == (3, 5)  # pos_x = col, pos_y = row

    tel = db.execute(
        text(
            "SELECT nivel_bateria_pct, velocidade_instantanea_cm_s, rpm_motor_esq "
            "FROM amostra_telemetria WHERE execucao_id = :i"
        ),
        {"i": execucao_ativa},
    ).mappings().first()
    assert tel["nivel_bateria_pct"] == pytest.approx(70.0)
    assert tel["velocidade_instantanea_cm_s"] == pytest.approx(20.0)
    assert tel["rpm_motor_esq"] == 450


def test_amostra_repetida_dentro_do_heartbeat_nao_salva(db, execucao_ativa, monkeypatch):
    monkeypatch.setattr(settings, "TELEMETRY_HEARTBEAT_INTERVAL_MS", 60_000)

    client.post(URL, json=ESP)
    r = client.post(URL, json=ESP)

    body = r.json()
    assert r.status_code == 200
    assert body["status"] == "unchanged"
    assert body["saved"] is False
    assert body["execucao_id"] == execucao_ativa
    assert body["next_sample_in_ms"] > 0
    assert contar(db, "amostra_telemetria", execucao_ativa) == 1
    assert contar(db, "amostra_movimento", execucao_ativa) == 1


def test_amostra_repetida_com_heartbeat_vencido_salva(db, execucao_ativa, monkeypatch):
    monkeypatch.setattr(settings, "TELEMETRY_HEARTBEAT_INTERVAL_MS", 0)

    client.post(URL, json=ESP)
    r = client.post(URL, json=ESP)

    assert r.json()["status"] == "saved"
    assert contar(db, "amostra_telemetria", execucao_ativa) == 2
    assert contar(db, "amostra_movimento", execucao_ativa) == 2


@pytest.mark.parametrize(
    "campo, novo_valor",
    [
        ("battery_pct", 79.0),
        ("speed_cm_s", 15.0),
        ("rpm", 310),
    ],
)
def test_amostra_com_valor_diferente_salva(db, execucao_ativa, monkeypatch, campo, novo_valor):
    monkeypatch.setattr(settings, "TELEMETRY_HEARTBEAT_INTERVAL_MS", 60_000)

    client.post(URL, json=ESP)
    r = client.post(URL, json={**ESP, campo: novo_valor})

    assert r.json()["status"] == "saved"
    assert contar(db, "amostra_telemetria", execucao_ativa) == 2


def test_mudanca_de_posicao_salva(db, execucao_ativa, monkeypatch):
    monkeypatch.setattr(settings, "TELEMETRY_HEARTBEAT_INTERVAL_MS", 60_000)

    client.post(URL, json=TELEMETRY_DATA)
    mudou = {**TELEMETRY_DATA, "position": {"col": 4, "row": 5}}
    r = client.post(URL, json=mudou)

    assert r.json()["status"] == "saved"
    assert contar(db, "amostra_movimento", execucao_ativa) == 2


def test_chama_mark_esp_packet_received(execucao_ativa, monkeypatch):
    from app.routes import telemetry as telemetry_routes

    chamadas = []
    monkeypatch.setattr(
        telemetry_routes.telemetry_service,
        "mark_esp_packet_received",
        lambda connected: chamadas.append(connected),
    )

    client.post(URL, json={**ESP, "connected": False})

    assert chamadas == [False]


def test_payload_invalido_retorna_422(execucao_ativa):
    r = client.post(URL, json={"battery_pct": "abc"})

    assert r.status_code == 422


def test_corpo_vazio_retorna_422(execucao_ativa):
    r = client.post(URL, json={})

    assert r.status_code == 422