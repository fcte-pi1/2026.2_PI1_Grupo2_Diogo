from datetime import datetime, timezone

from app.core.config import settings
from app.schemas.telemetry import TelemetryData, TelemetryResponse


class TelemetryService:
    def __init__(self):
        self._last_telemetry: TelemetryData | None = None

    def update(self, telemetry: TelemetryData) -> bool:
        """
        Atualiza a telemetria somente se o dado recebido
        for mais recente que o último dado válido.
        """

        if self._last_telemetry is not None:
            if telemetry.generated_at <= self._last_telemetry.generated_at:
                return False

        self._last_telemetry = telemetry

        return True

    def get_current(self) -> TelemetryResponse | None:
        """
        Retorna o último dado válido e calcula sua idade
        e se está defasado.
        """

        if self._last_telemetry is None:
            return None

        now = datetime.now(timezone.utc)

        generated_at = self._last_telemetry.generated_at

        if generated_at.tzinfo is None:
            generated_at = generated_at.replace(tzinfo=timezone.utc)

        age_ms = (
            now - generated_at
        ).total_seconds() * 1000

        stale_limit_ms = (
            settings.TELEMETRY_UPDATE_INTERVAL_MS
            + settings.TELEMETRY_LATENCY_TOLERANCE_MS
        )

        stale = age_ms > stale_limit_ms

        return TelemetryResponse(
            **self._last_telemetry.model_dump(),
            served_at=now,
            age_ms=round(age_ms, 2),
            stale=stale,
        )


telemetry_service = TelemetryService()