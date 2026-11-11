from datetime import datetime, timezone
from threading import RLock

from app.core.config import settings
from app.schemas.telemetry import TelemetryData, TelemetryResponse


class TelemetryService:
    """Mantém a última leitura válida, independentemente da gravação da corrida."""

    def __init__(self):
        self._lock = RLock()
        self._last_telemetry: TelemetryData | None = None
        self._last_esp_received_at: datetime | None = None
        self._esp_reports_connected = False
        self._metadata: dict = {}

    def mark_esp_packet_received(self, connected: bool = True) -> None:
        with self._lock:
            self._last_esp_received_at = datetime.now(timezone.utc)
            self._esp_reports_connected = connected

    def get_connection_status(self) -> dict:
        with self._lock:
            last_seen = self._last_esp_received_at
            age_ms = None if last_seen is None else max(
                0.0, (datetime.now(timezone.utc) - last_seen).total_seconds() * 1000
            )
            return {
                "connected": bool(
                    last_seen is not None
                    and self._esp_reports_connected
                    and age_ms <= settings.ESP_CONNECTION_TIMEOUT_MS
                ),
                "last_seen": last_seen,
                "age_ms": None if age_ms is None else round(age_ms, 2),
            }

    def update(self, telemetry: TelemetryData) -> bool:
        with self._lock:
            if self._last_telemetry is not None:
                if telemetry.generated_at <= self._last_telemetry.generated_at:
                    return False
            self._last_telemetry = telemetry.model_copy(deep=True)
            self._metadata = {}
            return True

    def associate_execution(self, telemetry: TelemetryData, **metadata) -> None:
        """Uma gravação atrasada não pode alterar a identificação da leitura seguinte."""
        with self._lock:
            if self._last_telemetry is not None and (
                self._last_telemetry.generated_at == telemetry.generated_at
            ):
                self._metadata.update(metadata)

    def restore(self, telemetry: TelemetryData, **metadata) -> None:
        """Recupera o último registro após reinício, sem marcar a ESP como conectada."""
        with self._lock:
            if self._last_telemetry is None:
                self.update(telemetry)
                self._metadata.update(metadata)

    def get_current(self) -> TelemetryResponse | None:
        with self._lock:
            if self._last_telemetry is None:
                return None
            now = datetime.now(timezone.utc)
            age_ms = max(
                0.0, (now - self._last_telemetry.generated_at).total_seconds() * 1000
            )
            stale_limit = (
                settings.TELEMETRY_UPDATE_INTERVAL_MS
                + settings.TELEMETRY_LATENCY_TOLERANCE_MS
            )
            return TelemetryResponse(
                **self._last_telemetry.model_dump(),
                served_at=now,
                age_ms=round(age_ms, 2),
                stale=age_ms > stale_limit,
            )

    def get_snapshot(self) -> dict:
        with self._lock:
            current = self.get_current()
            connection = self.get_connection_status()
            metrics = None if current is None else current.metrics
            return {
                "execucao_id": self._metadata.get("execucao_id"),
                "telemetry_sample_id": self._metadata.get("telemetry_sample_id"),
                "tempo_relativo_ms": self._metadata.get("tempo_relativo_ms"),
                "has_data": current is not None,
                "position": None if current is None else current.position.model_dump(),
                "metrics": None if metrics is None else {
                    "speed_cm_s": metrics.speed_cm_s,
                    "battery_pct": metrics.battery_pct,
                    "rpm": metrics.rpm,
                    "rpmLeft": metrics.rpm_left if metrics.rpm_left is not None else metrics.rpm,
                    "rpmRight": metrics.rpm_right if metrics.rpm_right is not None else metrics.rpm,
                },
                "movement": (
                    None if current is None or current.movement is None
                    else current.movement.model_dump()
                ),
                "events": (
                    [] if current is None
                    else [event.model_dump() for event in current.events]
                ),
                "generated_at": None if current is None else current.generated_at,
                "served_at": datetime.now(timezone.utc),
                "age_ms": None if current is None else current.age_ms,
                "stale": current is None or current.stale or not connection["connected"],
                "connected": connection["connected"],
                "last_seen": connection["last_seen"],
                "connection_age_ms": connection["age_ms"],
                "connection_timeout_ms": settings.ESP_CONNECTION_TIMEOUT_MS,
                "stale_after_ms": (
                    settings.TELEMETRY_UPDATE_INTERVAL_MS
                    + settings.TELEMETRY_LATENCY_TOLERANCE_MS
                ),
            }


telemetry_service = TelemetryService()
