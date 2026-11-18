"""Paredes detectadas: mapa lido da ESP (GET /api/maze) e gravado por execução."""
import httpx
import pytest
from sqlalchemy.exc import OperationalError

from app.core.config import settings
from app.schemas.telemetry import EspMaze
from app.services import esp_maze
from app.services.esp_maze import LESTE, NORTE, OESTE, SUL


def esp_map(largura=4, altura=4, paredes=None, pose=(0, 0)):
    """Mapa no formato da ESP: celulas[y][x] = [conhecidas, paredes, visitada]."""
    celulas = []
    for y in range(altura):
        linha = []
        for x in range(largura):
            # Bordas fechadas, como no Grid::init do firmware.
            borda = (
                (NORTE if y == altura - 1 else 0) | (LESTE if x == largura - 1 else 0)
                | (SUL if y == 0 else 0) | (OESTE if x == 0 else 0)
            )
            linha.append([borda, borda, 0])
        celulas.append(linha)
    for (x, y), mascara in (paredes or {}).items():
        celulas[y][x][0] |= mascara
        celulas[y][x][1] |= mascara
    return {
        "largura": largura, "altura": altura, "revisao": 3,
        "objetivo": {"x": largura - 1, "y": altura - 1},
        "pose": None if pose is None else {"x": pose[0], "y": pose[1], "direcao": "N"},
        "celulas": celulas,
    }


class Result:
    def __init__(self, rows):
        self.rows = rows

    def mappings(self):
        return self

    def all(self):
        return self.rows


class WallsDatabase:
    def __init__(self, stored=()):
        self.stored = [{"x": x, "y": y, "lado": lado} for x, y, lado in stored]
        self.deleted, self.inserted = [], []
        self.commits = self.rollbacks = 0
        self.fail = False

    def execute(self, query, params=None):
        if self.fail:
            raise OperationalError("test", {}, Exception("Banco indisponível"))
        sql = str(query)
        if sql.startswith("DELETE"):
            self.deleted.extend(params)
        elif sql.startswith("INSERT"):
            self.inserted.extend(params)
        else:
            return Result(self.stored)

    def commit(self):
        self.commits += 1

    def rollback(self):
        self.rollbacks += 1

    def __enter__(self):
        return self

    def __exit__(self, *args):
        pass


def row(execucao_id, x, y, lado):
    return {"execucao_id": execucao_id, "celula_x": x, "celula_y": y, "lado": lado}


def test_internal_walls_skip_the_border_and_keep_each_wall_once():
    maze = EspMaze.model_validate(esp_map(paredes={
        (1, 0): NORTE, (1, 1): SUL,   # mesma parede vista das duas células
        (0, 2): LESTE, (1, 2): OESTE,
        (2, 3): SUL,                  # só um lado marcado
    }))
    assert esp_maze.internal_walls(maze) == {
        (1, 0, "norte"), (0, 2, "leste"), (2, 2, "norte"),
    }


def test_sync_adds_new_walls_and_removes_the_ones_the_robot_reopened():
    db = WallsDatabase(stored=[(1, 0, "norte"), (2, 2, "leste")])
    changed = esp_maze.sync_walls(db, 7, {(1, 0, "norte"), (0, 2, "leste")})
    assert changed is True
    assert db.deleted == [row(7, 2, 2, "leste")]
    assert db.inserted == [row(7, 0, 2, "leste")]
    assert db.commits == 1


def test_sync_without_changes_does_not_write():
    db = WallsDatabase(stored=[(1, 0, "norte")])
    assert esp_maze.sync_walls(db, 7, {(1, 0, "norte")}) is False
    assert db.deleted == db.inserted == []
    assert db.commits == 0


@pytest.fixture
def esp(monkeypatch):
    """ESP falsa: devolve o mapa configurado e conta as leituras."""
    state = {"map": esp_map(), "fetches": 0}
    db = WallsDatabase()

    def fetch():
        state["fetches"] += 1
        return None if state["map"] is None else EspMaze.model_validate(state["map"])

    monkeypatch.setattr(settings, "MICROMOUSE_ESP_BASE_URL", "http://192.168.4.1")
    monkeypatch.setattr(esp_maze, "fetch_esp_maze", fetch)
    monkeypatch.setattr(esp_maze, "SessionLocal", lambda: db)
    return state, db


def test_race_receives_the_robot_walls_and_maze_size(esp):
    state, db = esp
    state["map"] = esp_map(4, 8, paredes={(0, 1): NORTE, (2, 5): LESTE}, pose=(0, 1))
    esp_maze.sync_from_esp(5)
    assert db.inserted == [row(5, 0, 1, "norte"), row(5, 2, 5, "leste")]
    assert esp_maze.maze_size_for(5) == {"columns": 4, "rows": 8}
    assert esp_maze.maze_size_for(6) is None


def test_walls_are_kept_while_the_robot_has_no_pose(esp):
    state, db = esp
    state["map"] = esp_map(pose=None)
    esp_maze.sync_from_esp(5)
    assert db.deleted == db.inserted == []
    assert esp_maze.maze_size_for(5) is None


def test_reads_at_most_once_per_interval_and_can_be_disabled(esp, monkeypatch):
    state, _ = esp
    esp_maze.sync_from_esp(5)
    esp_maze.sync_from_esp(5)
    assert state["fetches"] == 1
    monkeypatch.setattr(esp_maze, "_last_fetch", float("-inf"))
    monkeypatch.setattr(settings, "MICROMOUSE_ESP_BASE_URL", "")
    esp_maze.sync_from_esp(5)
    assert state["fetches"] == 1


def test_unreachable_esp_or_database_failure_does_not_break_the_telemetry(esp, monkeypatch):
    state, db = esp
    state["map"] = None
    esp_maze.sync_from_esp(5)
    assert db.commits == 0
    state["map"] = esp_map(paredes={(1, 1): NORTE})
    db.fail = True
    monkeypatch.setattr(esp_maze, "_last_fetch", float("-inf"))
    esp_maze.sync_from_esp(5)
    assert db.rollbacks == 1


def fake_esp(monkeypatch, response):
    requested = []
    real_client = httpx.Client

    def handler(request):
        requested.append(str(request.url))
        return response

    monkeypatch.setattr(
        esp_maze.httpx, "Client",
        lambda **kwargs: real_client(transport=httpx.MockTransport(handler), **kwargs),
    )
    return requested


def test_fetch_reads_the_maze_from_the_configured_esp(monkeypatch):
    monkeypatch.setattr(settings, "MICROMOUSE_ESP_BASE_URL", "http://192.168.4.1/")
    requested = fake_esp(monkeypatch, httpx.Response(200, json=esp_map(4, 12, pose=(0, 3))))
    maze = esp_maze.fetch_esp_maze()
    assert requested == ["http://192.168.4.1/api/maze"]
    assert (maze.largura, maze.altura, maze.pose.y) == (4, 12, 3)


@pytest.mark.parametrize("response", [
    httpx.Response(404, json={"erro": "Rota nao encontrada"}),
    httpx.Response(200, json={**esp_map(), "altura": 5}),
    httpx.Response(200, json={**esp_map(), "celulas": [[[0, 16, 0]] * 4] * 4}),
    httpx.Response(200, text="sem json"),
])
def test_fetch_ignores_errors_and_invalid_maps(monkeypatch, response):
    monkeypatch.setattr(settings, "MICROMOUSE_ESP_BASE_URL", "http://192.168.4.1")
    fake_esp(monkeypatch, response)
    assert esp_maze.fetch_esp_maze() is None
