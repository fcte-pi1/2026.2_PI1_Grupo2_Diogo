from app.models import Metrics
from datetime import datetime, timezone

from app.mock import state
from app.mock.seed import build_seed_path
from app.models import ExecutionStatus
from app.models import Event
from app.models import EventType

ROUTE = build_seed_path()


SPEED_CM_S = 18.0
RPM = 101.0
BATTERY_DRAIN = 0.5
started_at = None

def add_event(message: str)-> None:
    state.events.append(Event(timestamp=datetime.now(timezone.utc), type=EventType.info, message=message))

def start_run() -> None:
    global started_at
    started_at = datetime.now(timezone.utc)
    state.metrics = Metrics(speed_cm_s=0, rpm=0, battery_pct=100)
    state.path = [ROUTE[0]]
    state.execution.status = ExecutionStatus.running
    state.execution.tempo_total_ms = 0
    state.events = []
    add_event("Session started")


def tick() -> None:
    step = len(state.path)
    if step == len(ROUTE):
        return
    state.path.append(ROUTE[step])
    state.execution.tempo_total_ms += 1000
    state.metrics = Metrics(speed_cm_s=SPEED_CM_S, rpm=RPM, battery_pct=state.metrics.battery_pct - BATTERY_DRAIN)
    add_event(f"avancou para: {ROUTE[step].row}, {ROUTE[step].col}")
    if step == len(ROUTE) - 1:
        state.execution.status = ExecutionStatus.finished
        state.metrics = Metrics(speed_cm_s=0, rpm=0, battery_pct=state.metrics.battery_pct)
        add_event("finalizou em {:.2f} segundos".format(state.execution.tempo_total_ms/1000))


def update()-> None:
    if state.execution is None or started_at is None:
        return
    seconds = int((datetime.now(timezone.utc) - started_at).total_seconds())
    while state.execution.status == ExecutionStatus.running and state.execution.tempo_total_ms < seconds * 1000:
        tick()