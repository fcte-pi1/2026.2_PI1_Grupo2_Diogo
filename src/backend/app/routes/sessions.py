from fastapi import APIRouter, HTTPException

from app.mock import state
from app.models import Execution
from app.models import PathPoint
from app.models import Metrics
from app.models import Event
from app.models import Maze

router = APIRouter(tags=["sessions"])
SESSION_NOT_FOUND = {"description": "No active session found"} #usar pra avisar o 404

def check_active_session(session_id: str) -> None: #da o 404 se nao existir
    if state.execution is None or state.execution.id != session_id:
        raise HTTPException(status_code=404, detail="No active session found")


@router.get("/sessions/active", response_model=Execution | None)
def get_active_session() -> Execution | None:
    return state.execution

@router.get("/sessions/{session_id}/maze", response_model=Maze, responses={404: SESSION_NOT_FOUND})
def get_session_maze(session_id: str) -> Maze:
    check_active_session(session_id)
    return state.maze

@router.get("/sessions/{session_id}/path", response_model=list[PathPoint], responses={404: SESSION_NOT_FOUND})
def get_session_path(session_id: str) -> list[PathPoint] :
    check_active_session(session_id)
    return state.path

@router.get("/sessions/{session_id}/metrics", response_model=Metrics, responses={404: SESSION_NOT_FOUND})
def get_session_metrics(session_id: str) -> Metrics:
    check_active_session(session_id)
    return state.metrics

@router.get("/sessions/{session_id}/events", response_model=list[Event], responses={404: SESSION_NOT_FOUND})
def get_session_events(session_id: str) -> list[Event]:
    check_active_session(session_id)
    return state.events

