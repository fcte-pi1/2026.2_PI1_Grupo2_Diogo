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

