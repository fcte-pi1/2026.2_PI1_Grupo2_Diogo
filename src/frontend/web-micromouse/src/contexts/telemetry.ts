import { createContext } from "react";
import type { TelemetryState } from "../types/telemetry";

export const TelemetryContext = createContext<TelemetryState | null>(null);
