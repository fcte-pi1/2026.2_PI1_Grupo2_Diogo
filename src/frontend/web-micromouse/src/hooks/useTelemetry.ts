import { useContext } from "react";
import { TelemetryContext } from "../contexts/telemetry";

export function useTelemetry() {
  const telemetry = useContext(TelemetryContext);
  if (!telemetry) throw new Error("useTelemetry requer TelemetryProvider.");
  return telemetry;
}
