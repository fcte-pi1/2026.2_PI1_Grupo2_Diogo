import { describe, expect, it } from "vitest";
import { formatDuration, formatReportDate, toReportExecution } from "./report";

describe("report utilities", () => {
  it.each([
    [null, "—"],
    [0, "—"],
    [135000, "02:15"],
    [3661000, "61:01"],
  ] as const)("formats duration %s", (input, expected) => {
    expect(formatDuration(input)).toBe(expected);
  });

  it("formats valid dates and handles invalid dates", () => {
    expect(formatReportDate("invalid")).toBe("Data indisponível");
    expect(formatReportDate("2026-10-25T14:30:00Z")).toContain("25 out 2026");
  });

  it("converts history data into report fields with defaults and units", () => {
    expect(toReportExecution({
      id: "42",
      algoritmo: "Flood Fill",
      labirinto_nome: "Labirinto 4x4",
      iniciada_em: "2026-10-25T14:30:00Z",
      finalizada_em: null,
      tempo_total_ms: 135000,
      status: "concluido",
      resultado: "sucesso",
      velocidade_media_cm_s: 8.4,
      quantidade_celulas_visitadas: 12,
      rpm: 320,
      bateria_consumida_pct: 19,
    })).toMatchObject({
      id: "42",
      status: "Concluída",
      duration: "02:15",
      averageSpeed: "8.4 cm/s",
      cellsVisited: 12,
      averageRpm: 320,
      batteryConsumed: "19%",
      distance: "—",
    });
  });

  it("maps failed and interrupted executions to their display status", () => {
    const base = {
      id: "1", algoritmo: "A", labirinto_nome: "Maze", iniciada_em: "invalid",
      finalizada_em: null, tempo_total_ms: null, velocidade_media_cm_s: null,
      quantidade_celulas_visitadas: null, rpm: null, bateria_consumida_pct: null,
    };
    expect(toReportExecution({ ...base, status: "concluido", resultado: "falha" }).status).toBe("Não concluída");
    expect(toReportExecution({ ...base, status: "interrompido", resultado: null }).status).toBe("Interrompida");
    expect(toReportExecution({ ...base, status: "em_execucao", resultado: null })).toMatchObject({
      status: "Em execução", averageSpeed: "—", averageRpm: 0, batteryConsumed: "—",
    });
  });
});
