from datetime import datetime

from pydantic import BaseModel, Field


class PositionData(BaseModel):
    row: int
    col: int


class MetricsData(BaseModel):
    speed_cm_s: float = Field(ge=0)
    rpm: float = Field(ge=0)
    battery_pct: float = Field(ge=0, le=100)


class TelemetryEventData(BaseModel):
    timestamp: datetime
    type: str
    message: str


class TelemetryData(BaseModel):
    position: PositionData
    metrics: MetricsData
    events: list[TelemetryEventData] = Field(default_factory=list)
    generated_at: datetime


class TelemetryResponse(TelemetryData):
    served_at: datetime
    age_ms: float
    stale: bool