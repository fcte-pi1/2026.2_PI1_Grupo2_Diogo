from fastapi import APIRouter, HTTPException

from app.schemas.telemetry import TelemetryResponse
from app.services.telemetry_service import telemetry_service


router = APIRouter(
    prefix="/telemetry",
    tags=["Telemetry"],
)

@router.get(
    "/current",
    response_model=TelemetryResponse,
)
def get_current_telemetry():

    telemetry = telemetry_service.get_current()

    if telemetry is None:
        raise HTTPException(
            status_code=503,
            detail="Nenhum dado de telemetria disponível.",
        )

    return telemetry    