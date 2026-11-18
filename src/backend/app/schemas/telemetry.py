from datetime import datetime, timezone

from pydantic import AliasChoices, BaseModel, ConfigDict, Field, field_validator, model_validator


class PositionData(BaseModel):
    row: int
    col: int


class MetricsData(BaseModel):
    model_config = ConfigDict(allow_inf_nan=False)

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


class MovementData(BaseModel):
    model_config = ConfigDict(allow_inf_nan=False)

    giro_x_deg_s: float
    giro_y_deg_s: float
    giro_z_deg_s: float


class TelemetryData(BaseModel):
    position: PositionData
    metrics: MetricsData
    events: list[TelemetryEventData] = Field(default_factory=list)
    generated_at: datetime
    movement: MovementData | None = None

    @field_validator("generated_at")
    @classmethod
    def normalize_generated_at(cls, value: datetime) -> datetime:
        if value.tzinfo is None:
            return value.replace(tzinfo=timezone.utc)
        return value.astimezone(timezone.utc)


class TelemetryResponse(TelemetryData):
    served_at: datetime
    age_ms: float
    stale: bool


class EstimatedPosition(BaseModel):
    x: int = Field(ge=0)
    y: int = Field(ge=0)


class EspTelemetryInput(BaseModel):
    """Formato simples enviado pela ESP, com posição opcional."""

    model_config = ConfigDict(populate_by_name=True, allow_inf_nan=False)

    estimated_position: EstimatedPosition | None = Field(
        default=None, alias="posicao_estimada"
    )

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


class EspMazePose(BaseModel):
    x: int = Field(ge=0)
    y: int = Field(ge=0)


class EspMaze(BaseModel):
    """Mapa que a ESP mantém em GET /api/maze.

    celulas[y][x] = [conhecidas, paredes, visitada], com máscaras N=1, L=2,
    S=4 e O=8. A origem (0,0) é o canto sudoeste e y cresce para o norte.
    """

    largura: int = Field(ge=2, le=16)
    altura: int = Field(ge=2, le=16)
    pose: EspMazePose | None = None
    celulas: list[list[list[int]]]

    @model_validator(mode="after")
    def validate_grid(self):
        if len(self.celulas) != self.altura or any(
            len(linha) != self.largura for linha in self.celulas
        ):
            raise ValueError("celulas deve ter altura x largura posições.")
        if any(
            len(celula) < 2 or not all(0 <= mascara <= 15 for mascara in celula[:2])
            for linha in self.celulas
            for celula in linha
        ):
            raise ValueError("Cada célula deve trazer as máscaras de 0 a 15.")
        return self
