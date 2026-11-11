import type { TelemetrySnapshot } from "../types/telemetry";

export function telemetryWebSocketUrl(): string {
  const base = new URL(import.meta.env.VITE_API_URL || window.location.origin, window.location.origin);
  const url = new URL("/ws/telemetry/current", base);
  url.protocol = base.protocol === "https:" ? "wss:" : "ws:";
  return url.href;
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null;
}

function isNumber(value: unknown): value is number {
  return typeof value === "number" && Number.isFinite(value);
}

export function parseTelemetrySnapshot(message: string): TelemetrySnapshot | null {
  try {
    const value: unknown = JSON.parse(message);
    if (!isRecord(value)
      || typeof value.has_data !== "boolean"
      || typeof value.connected !== "boolean"
      || typeof value.stale !== "boolean"
      || !isNumber(value.connection_timeout_ms) || value.connection_timeout_ms <= 0
      || !isNumber(value.stale_after_ms) || value.stale_after_ms <= 0
      || !(value.age_ms === null || (isNumber(value.age_ms) && value.age_ms >= 0))
      || !(value.connection_age_ms === null || (isNumber(value.connection_age_ms) && value.connection_age_ms >= 0))) {
      return null;
    }
    if (value.has_data) {
      if (typeof value.generated_at !== "string" || !Number.isFinite(Date.parse(value.generated_at))
        || !isNumber(value.age_ms) || !isRecord(value.metrics)
        || !["battery_pct", "speed_cm_s", "rpm", "rpmLeft", "rpmRight"].every(key => isNumber((value.metrics as Record<string, unknown>)[key]))
        || !isRecord(value.position) || !isNumber(value.position.row) || !isNumber(value.position.col)) {
        return null;
      }
    } else if (value.metrics !== null || value.generated_at !== null || value.position !== null) {
      return null;
    }
    if (value.movement !== null && (!isRecord(value.movement)
      || !["giro_x_deg_s", "giro_y_deg_s", "giro_z_deg_s"].every(key => isNumber((value.movement as Record<string, unknown>)[key])))) {
      return null;
    }
    return value as unknown as TelemetrySnapshot;
  } catch {
    return null;
  }
}
