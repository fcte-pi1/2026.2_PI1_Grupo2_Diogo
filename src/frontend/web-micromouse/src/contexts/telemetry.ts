import { createContext } from "react";
import type { TelemetryState } from "../types/index";

export const TelemetryContext = createContext<TelemetryState | null>(null);
