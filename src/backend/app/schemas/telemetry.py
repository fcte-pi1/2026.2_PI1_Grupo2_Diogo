from datetime import datetime

from pydantic import AliasChoices, BaseModel, ConfigDict, Field, model_validator


class PositionData(BaseModel):
    row: int
    col: int


class MetricsData(BaseModel):
    speed_cm_s: float = Field(ge=0)
    rpm: float | None = Field(default=None, ge=0)
    rpm_left: float | None = Field(
        default=None, ge=0, validation_alias=AliasChoices("rpmLeft", "rpm_left")
    )
    rpm_right: float | None = Field(
        default=None, ge=0, validation_alias=AliasChoices("rpmRight", "rpm_right")
    )
    battery_pct: float = Field(ge=0, le=100)

    @model_validator(mode="after")
    def fill_average_rpm(self):
        if self.rpm is None:
            if self.rpm_left is None or self.rpm_right is None:
                raise ValueError(
                    "Informe metrics.rpm ou os dois campos metrics.rpmLeft e metrics.rpmRight."
                )
            self.rpm = (self.rpm_left + self.rpm_right) / 2
        return self


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


class EspTelemetryInput(BaseModel):
    """Formato simples enviado pela ESP, com posição opcional."""

    model_config = ConfigDict(populate_by_name=True)

    battery_pct: float = Field(alias="bateria", ge=0, le=100)
    speed_cm_s: float = Field(alias="velocidade", ge=0)
    rpm: int = Field(alias="rpm_motor", ge=0)
    connected: bool = True
    position: PositionData | None = None
    row: int | None = Field(
        default=None, validation_alias=AliasChoices("row", "pos_y")
    )
    col: int | None = Field(
        default=None, validation_alias=AliasChoices("col", "pos_x")
    )

    @model_validator(mode="after")
    def validate_position_pair(self):
        if (self.row is None) != (self.col is None):
            raise ValueError("Informe row e col juntos quando enviar a posição.")
        return self
