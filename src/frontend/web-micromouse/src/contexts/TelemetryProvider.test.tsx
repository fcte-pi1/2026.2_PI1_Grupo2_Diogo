import { StrictMode } from "react";
import { act, cleanup, render, screen } from "@testing-library/react";
import { afterEach, beforeEach, expect, test, vi } from "vitest";
import { TelemetryProvider } from "./TelemetryProvider";
import { useTelemetry } from "../hooks/useTelemetry";
import type { TelemetrySnapshot } from "../types/telemetry";

class MockWebSocket {
  static instances: MockWebSocket[] = [];
  onmessage: ((event: { data: string }) => void) | null = null;
  onclose: (() => void) | null = null;
  onerror: (() => void) | null = null;
  close = vi.fn();

  url: string;
  constructor(url: string) {
    this.url = url;
    MockWebSocket.instances.push(this);
  }

  receive(value: unknown) {
    this.onmessage?.({ data: JSON.stringify(value) });
  }
}

function snapshot(overrides: Partial<TelemetrySnapshot> = {}): TelemetrySnapshot {
  return {
    has_data: true, connected: true, stale: false, execucao_id: 2,
    generated_at: new Date().toISOString(), age_ms: 0, connection_age_ms: 0,
    connection_timeout_ms: 5000, stale_after_ms: 1250,
    metrics: { battery_pct: 87, speed_cm_s: 18, rpm: 130, rpmLeft: 120, rpmRight: 140 },
    movement: null, position: { row: 1, col: 2 }, ...overrides,
  };
}

function Consumer({ name = "state" }: { name?: string }) {
  const value = useTelemetry();
  return <output data-testid={name}>{JSON.stringify(value)}</output>;
}

function state() {
  return JSON.parse(screen.getByTestId("state").textContent ?? "{}");
}

function mount() {
  const result = render(<TelemetryProvider><Consumer /></TelemetryProvider>);
  act(() => vi.advanceTimersByTime(0));
  return result;
}

beforeEach(() => {
  vi.useFakeTimers();
  vi.stubGlobal("WebSocket", MockWebSocket);
  MockWebSocket.instances = [];
});

afterEach(() => {
  cleanup();
  vi.unstubAllGlobals();
  vi.useRealTimers();
});

test("consumidores compartilham uma conexão, inclusive no StrictMode", () => {
  render(<StrictMode><TelemetryProvider><Consumer /><Consumer name="sidebar" /></TelemetryProvider></StrictMode>);
  act(() => vi.advanceTimersByTime(0));
  expect(MockWebSocket.instances).toHaveLength(1);
  expect(MockWebSocket.instances[0].url).toMatch(/\/ws\/telemetry\/current$/);
  expect(state().connected).toBe(false);
});

test("preserva valores na queda e recebe dados novos após reconectar", () => {
  mount();
  act(() => MockWebSocket.instances[0].receive(snapshot()));
  expect(state().connected).toBe(true);
  act(() => MockWebSocket.instances[0].onclose?.());
  expect(state().streamStatus).toBe("reconnecting");
  expect(state().stale).toBe(true);
  expect(state().data.metrics.battery_pct).toBe(87);
  act(() => vi.advanceTimersByTime(1000));
  expect(MockWebSocket.instances).toHaveLength(2);
  expect(state().connected).toBe(false);
  act(() => MockWebSocket.instances[1].receive(snapshot({ metrics: { ...snapshot().metrics!, battery_pct: 86 } })));
  expect(state().connected).toBe(true);
  expect(state().stale).toBe(false);
  expect(state().data.metrics.battery_pct).toBe(86);
});

test("faz novas tentativas com espera crescente e cancela tudo ao desmontar", () => {
  const view = mount();
  act(() => MockWebSocket.instances[0].onerror?.());
  act(() => vi.advanceTimersByTime(1000));
  act(() => MockWebSocket.instances[1].onerror?.());
  act(() => vi.advanceTimersByTime(1999));
  expect(MockWebSocket.instances).toHaveLength(2);
  act(() => vi.advanceTimersByTime(1));
  expect(MockWebSocket.instances).toHaveLength(3);
  view.unmount();
  expect(MockWebSocket.instances[2].close).toHaveBeenCalledOnce();
  act(() => vi.advanceTimersByTime(20000));
  expect(MockWebSocket.instances).toHaveLength(3);
  expect(vi.getTimerCount()).toBe(0);
});

test("detecta silêncio mesmo quando o navegador não sinaliza fechamento", () => {
  mount();
  act(() => MockWebSocket.instances[0].receive(snapshot()));
  act(() => vi.advanceTimersByTime(2000));
  expect(state().stale).toBe(true);
  expect(state().connected).toBe(true);
  act(() => vi.advanceTimersByTime(4000));
  expect(state().streamStatus).toBe("reconnecting");
  expect(state().connected).toBe(false);
  expect(state().data.metrics.battery_pct).toBe(87);
});

test("resposta vazia após reinício mantém as leituras anteriores como antigas", () => {
  mount();
  act(() => MockWebSocket.instances[0].receive(snapshot()));
  act(() => vi.advanceTimersByTime(1000));
  act(() => MockWebSocket.instances[0].receive(snapshot({ has_data: false, connected: false, stale: true, generated_at: null, age_ms: null, metrics: null, position: null })));
  expect(state().data.metrics.battery_pct).toBe(87);
  expect(state().ageMs).toBe(1000);
  expect(state().stale).toBe(true);
});

test("mensagens inválidas não sobrescrevem a última leitura válida", () => {
  mount();
  act(() => MockWebSocket.instances[0].receive(snapshot()));
  act(() => MockWebSocket.instances[0].onmessage?.({ data: "invalid" }));
  act(() => MockWebSocket.instances[0].receive(snapshot({ metrics: { ...snapshot().metrics!, battery_pct: "invalid" } } as unknown as TelemetrySnapshot)));
  expect(state().data.metrics.battery_pct).toBe(87);
});

test("mantém a telemetria disponível durante dez minutos de atualizações", () => {
  mount();
  for (let second = 0; second < 600; second++) {
    act(() => {
      vi.advanceTimersByTime(1000);
      MockWebSocket.instances[0].receive(snapshot());
    });
    expect(state().connected).toBe(true);
    expect(state().stale).toBe(false);
  }
  expect(MockWebSocket.instances).toHaveLength(1);
});
