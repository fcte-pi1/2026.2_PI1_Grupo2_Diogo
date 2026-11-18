from unittest.mock import MagicMock
import pytest
from fastapi.testclient import TestClient
from app.core.config import settings
from app.core.database import get_db
from app.main import app
from app.services import esp_maze


@pytest.fixture(autouse=True)
def esp_maze_offline(monkeypatch):
    """Nenhum teste acessa a ESP real; os testes do mapa usam dados falsos."""
    monkeypatch.setattr(settings, "MICROMOUSE_ESP_BASE_URL", "")
    monkeypatch.setattr(esp_maze, "_last_fetch", float("-inf"))
    monkeypatch.setattr(esp_maze, "_maze_size", None)
    monkeypatch.setattr(esp_maze, "_warned", False)


@pytest.fixture
def unit_client():
    db = MagicMock()
    previus_overrides = app.dependency_overrides.copy()
    app.dependency_overrides[get_db] = lambda: db
    try:
        with TestClient(app) as client:
            yield client, db
    finally:
        app.dependency_overrides.clear()
        app.dependency_overrides.update(previus_overrides)