export type ExecutionStatus = "em_execucao" | "concluido" | "interrompido";
export type ExecutionResult = "sucesso" | "falha";

export type HistoryItem = {
  id: string;
  algoritmo: string;
  labirinto_nome: string;
  iniciada_em: string;
  finalizada_em: string | null;
  tempo_total_ms: number | null;
  status: ExecutionStatus;
  resultado: ExecutionResult | null;
  velocidade_media_cm_s: number | null;
  quantidade_celulas_visitadas: number | null;
  rpm: number | null;
  bateria_consumida_pct: number | null;
};

export type HistoryResponse = { items: HistoryItem[] };

export type ReportExecution = {
  id: string;
  maze: string;
  algorithm: string;
  date: string;
  status: string;
  duration: string;
  averageSpeed: string;
  cellsVisited: number;
  averageRpm: number;
  batteryConsumed: string;
  distance: string;
};
