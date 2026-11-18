"""Paredes detectadas: lê o mapa da ESP e grava na tabela parede_detectada.

O firmware guarda o mapa do labirinto na própria ESP (GET /api/maze) e não
manda as paredes na telemetria. Quando chega telemetria da ESP durante uma
execução ativa, o backend busca esse mapa em segundo plano e deixa a tabela
igual a ele: grava as paredes novas e apaga as que o robô reabriu.

As coordenadas ficam no referencial da ESP: origem (0,0) no canto sudoeste,
x cresce para leste e y para o norte. Cada parede interna é gravada uma vez,
como "norte" (entre y e y+1) ou "leste" (entre x e x+1). As bordas externas
não entram: o firmware já começa com elas fechadas.
"""
import logging
import threading
import time

import httpx
from sqlalchemy import text
from sqlalchemy.exc import SQLAlchemyError
from sqlalchemy.orm import Session

from app.core.config import settings
from app.core.database import SessionLocal
from app.schemas.telemetry import EspMaze

logger = logging.getLogger(__name__)

# Bits das máscaras de parede do firmware (maze_state.h).
NORTE, LESTE, SUL, OESTE = 1, 2, 4, 8
MIN_INTERVAL_S = 1.0
TIMEOUT_S = 1.0

_lock = threading.Lock()
_last_fetch = float("-inf")
_maze_size: dict | None = None
_warned = False


def fetch_esp_maze() -> EspMaze | None:
    global _warned
    url = settings.MICROMOUSE_ESP_BASE_URL.rstrip("/") + "/api/maze"
    try:
        with httpx.Client(timeout=TIMEOUT_S, trust_env=False) as client:
            response = client.get(url)
            response.raise_for_status()
            maze = EspMaze.model_validate(response.json())
    except (httpx.HTTPError, ValueError):
        if not _warned:
            logger.warning("Mapa da ESP indisponível em %s.", url, exc_info=True)
            _warned = True
        return None
    _warned = False
    return maze


def internal_walls(maze: EspMaze) -> set[tuple[int, int, str]]:
    walls = set()
    for y, row in enumerate(maze.celulas):
        for x, cell in enumerate(row):
            mask = cell[1]
            if mask & NORTE and y + 1 < maze.altura:
                walls.add((x, y, "norte"))
            if mask & SUL and y > 0:
                walls.add((x, y - 1, "norte"))
            if mask & LESTE and x + 1 < maze.largura:
                walls.add((x, y, "leste"))
            if mask & OESTE and x > 0:
                walls.add((x - 1, y, "leste"))
    return walls


def sync_walls(db: Session, execucao_id: int, walls: set[tuple[int, int, str]]) -> bool:
    """Deixa as paredes gravadas da execução iguais às do mapa; True se mudou algo."""
    stored = {
        (row["x"], row["y"], row["lado"])
        for row in db.execute(
            text(
                "SELECT celula_x AS x, celula_y AS y, lado FROM parede_detectada "
                "WHERE execucao_id = :execucao_id"
            ),
            {"execucao_id": execucao_id},
        ).mappings().all()
    }
    removed = sorted(stored - walls)
    added = sorted(walls - stored)

    def rows(items):
        return [
            {"execucao_id": execucao_id, "celula_x": x, "celula_y": y, "lado": lado}
            for x, y, lado in items
        ]

    if removed:
        db.execute(
            text(
                "DELETE FROM parede_detectada WHERE execucao_id = :execucao_id "
                "AND celula_x = :celula_x AND celula_y = :celula_y AND lado = :lado"
            ),
            rows(removed),
        )
    if added:
        db.execute(
            text(
                "INSERT INTO parede_detectada (execucao_id, celula_x, celula_y, lado) "
                "VALUES (:execucao_id, :celula_x, :celula_y, :lado) ON CONFLICT DO NOTHING"
            ),
            rows(added),
        )
    if removed or added:
        db.commit()
    return bool(removed or added)


def sync_from_esp(execucao_id: int) -> None:
    """Busca o mapa na ESP e grava as paredes da execução. Roda em segundo plano."""
    global _last_fetch, _maze_size
    if not settings.MICROMOUSE_ESP_BASE_URL or not _lock.acquire(blocking=False):
        return
    try:
        now = time.monotonic()
        if now - _last_fetch < MIN_INTERVAL_S:
            return
        _last_fetch = now
        maze = fetch_esp_maze()
        # Sem pose, a ESP ainda não iniciou a missão (o mapa é o vazio do boot)
        # ou perdeu a posição; nesses casos as paredes gravadas são mantidas.
        if maze is None or maze.pose is None:
            return
        _maze_size = {"execucao_id": execucao_id, "columns": maze.largura, "rows": maze.altura}
        with SessionLocal() as db:
            try:
                sync_walls(db, execucao_id, internal_walls(maze))
            except SQLAlchemyError:
                db.rollback()
                logger.warning(
                    "Não foi possível gravar as paredes da execução %s.",
                    execucao_id,
                    exc_info=True,
                )
    finally:
        _lock.release()


def maze_size_for(execucao_id: int | None) -> dict | None:
    """Tamanho do mapa da ESP (colunas = largura, linhas = altura) na execução."""
    if execucao_id is None or _maze_size is None or _maze_size["execucao_id"] != execucao_id:
        return None
    return {"columns": _maze_size["columns"], "rows": _maze_size["rows"]}
