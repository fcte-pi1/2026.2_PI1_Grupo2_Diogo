import { format } from "date-fns";
import { ptBR } from "date-fns/locale";
import type { HistoryItem, ReportExecution } from "../types/report";

export const REPORT_CONTENT = [
  "Resumo executivo",
  "Mapa do labirinto explorado",
  "Gráficos de telemetria",
  "Log de eventos",
  "Análise de algoritmo",
  "Comparativo com execuções anteriores",
];

export function formatDuration(milliseconds: number | null): string {
  if (milliseconds == null || milliseconds <= 0) return "—";
  const seconds = Math.floor(milliseconds / 1000);
  return `${String(Math.floor(seconds / 60)).padStart(2, "0")}:${String(seconds % 60).padStart(2, "0")}`;
}

export function formatReportDate(date: string): string {
  const parsed = new Date(date);
  return Number.isNaN(parsed.getTime()) ? "Data indisponível" : format(parsed, "dd MMM yyyy", { locale: ptBR });
}

export function toReportExecution(item: HistoryItem): ReportExecution {
  const status = item.status === "concluido"
    ? item.resultado === "falha" ? "Não concluída" : "Concluída"
    : item.status === "interrompido" ? "Interrompida" : "Em execução";

  return {
    id: item.id,
    maze: item.labirinto_nome || "Labirinto",
    algorithm: item.algoritmo || "Desconhecida",
    date: formatReportDate(item.iniciada_em),
    status,
    duration: formatDuration(item.tempo_total_ms),
    averageSpeed: item.velocidade_media_cm_s == null ? "—" : `${item.velocidade_media_cm_s} cm/s`,
    cellsVisited: item.quantidade_celulas_visitadas ?? 0,
    averageRpm: item.rpm ?? 0,
    batteryConsumed: item.bateria_consumida_pct == null ? "—" : `${item.bateria_consumida_pct}%`,
    distance: "—",
  };
}
