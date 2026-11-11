import { useEffect, useState, type ReactNode } from "react";
import { TelemetryContext } from "./telemetry";
import { parseTelemetrySnapshot, telemetryWebSocketUrl } from "../services/telemetry";
import type { TelemetrySnapshot, TelemetryStreamStatus } from "../types/telemetry";

const SILENCE_TIMEOUT_MS = 5000;
const MAX_RETRY_DELAY_MS = 10000;

export function TelemetryProvider({ children }: { children: ReactNode }) {
  const [frame, setFrame] = useState<{ data: TelemetrySnapshot; receivedAt: number } | null>(null);
  const [streamStatus, setStreamStatus] = useState<TelemetryStreamStatus>("connecting");
  const [now, setNow] = useState(Date.now);

  useEffect(() => {
    let socket: WebSocket | null = null;
    let disposed = false;
    let retryDelay = 1000;
    let lastMessageAt = Date.now();
    let retryTimer: number | undefined;

    function closeSocket() {
      if (!socket) return;
      socket.onmessage = null;
      socket.onclose = null;
      socket.onerror = null;
      socket.close();
      socket = null;
    }

    function retry() {
      if (disposed || retryTimer !== undefined) return;
      closeSocket();
      setStreamStatus("reconnecting");
      retryTimer = window.setTimeout(() => {
        retryTimer = undefined;
        connect();
      }, retryDelay);
      retryDelay = Math.min(retryDelay * 2, MAX_RETRY_DELAY_MS);
    }

    function connect() {
      if (disposed) return;
      lastMessageAt = Date.now();
      try {
        socket = new WebSocket(telemetryWebSocketUrl());
        socket.onmessage = event => {
          if (disposed || typeof event.data !== "string") return;
          const data = parseTelemetrySnapshot(event.data);
          if (!data) return;
          lastMessageAt = Date.now();
          retryDelay = 1000;
          setFrame(previous => ({
            // Uma resposta sem leituras não apaga os últimos valores conhecidos.
            data: !data.has_data && previous?.data.has_data ? {
              ...previous.data,
              connected: data.connected,
              stale: true,
              age_ms: (previous.data.age_ms ?? 0) + lastMessageAt - previous.receivedAt,
              connection_age_ms: data.connection_age_ms,
            } : data,
            receivedAt: lastMessageAt,
          }));
          setNow(lastMessageAt);
          setStreamStatus("open");
        };
        socket.onclose = retry;
        socket.onerror = retry;
      } catch {
        retry();
      }
    }

    // Adia a abertura para que a montagem de verificação do StrictMode possa limpar tudo.
    const startTimer = window.setTimeout(connect, 0);
    const clock = window.setInterval(() => {
      const timestamp = Date.now();
      setNow(timestamp);
      if (socket && timestamp - lastMessageAt > SILENCE_TIMEOUT_MS) retry();
    }, 1000);

    return () => {
      disposed = true;
      window.clearTimeout(startTimer);
      window.clearTimeout(retryTimer);
      window.clearInterval(clock);
      closeSocket();
    };
  }, []);

  const data = frame?.data ?? null;
  const elapsed = frame ? Math.max(0, now - frame.receivedAt) : 0;
  const ageMs = data?.age_ms == null ? null : data.age_ms + elapsed;
  const connected = streamStatus === "open" && Boolean(data?.connected)
    && data?.connection_age_ms != null
    && data.connection_age_ms + elapsed <= data.connection_timeout_ms;
  const stale = streamStatus !== "open" || !connected || data?.stale !== false
    || ageMs === null || ageMs > (data?.stale_after_ms ?? 1250);

  return (
    <TelemetryContext.Provider value={{ data, streamStatus, connected, stale, ageMs }}>
      {children}
    </TelemetryContext.Provider>
  );
}
