export interface TelemetrySnapshot {
  has_data: boolean;
  connected: boolean;
  stale: boolean;
  generated_at: string | null;
  age_ms: number | null;
  connection_age_ms: number | null;
  connection_timeout_ms: number;
  stale_after_ms: number;
  execucao_id: number | null;
  metrics: {
    battery_pct: number;
    speed_cm_s: number;
    rpm: number;
    rpmLeft: number;
    rpmRight: number;
  } | null;
  movement: {
    giro_x_deg_s: number;
    giro_y_deg_s: number;
    giro_z_deg_s: number;
  } | null;
  position: { row: number; col: number } | null;
}

export type TelemetryStreamStatus = "connecting" | "open" | "reconnecting";

export interface TelemetryState {
  data: TelemetrySnapshot | null;
  streamStatus: TelemetryStreamStatus;
  connected: boolean;
  stale: boolean;
  ageMs: number | null;
}
