export interface BatteryData {
  percentage: number;
  voltage: number;
  current: number;
  estimatedRechargeMinutes: number;
}

export interface MotorData {
  leftRpm: number;
  rightRpm: number;
}

export interface GyroscopeData {
  x: number;
  y: number;
  z: number;
}

export interface TelemetryData {
  connected: boolean;
  battery: BatteryData;
  averageSpeed: number;
  motors: MotorData;
  gyro: GyroscopeData;
}




export interface PropsExec {
  connected:boolean;
  execucao_id?: number;
  id?: string | number;
  algorithm: string;
  iniciada_em?: string | Date;
  labirinto_id: number;
  status: string;
  metrics?: {
    battery_pct: number;
    rpm: number;
    speed_cm_s: number;
    rpmLeft: number;
    rpmRight: number;
  };
  movement?: {
    giro_x_deg_s: number;
    giro_y_deg_s: number;
    giro_z_deg_s: number;
  };
  position: {
    col: number;
    row: number;
  };
}

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
  subscribe: (listener: (data: TelemetrySnapshot) => void) => () => void;
}
