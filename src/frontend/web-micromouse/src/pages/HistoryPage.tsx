import { useEffect, useState } from "react";
import { api } from "../services/api";
import { Table, type HistoryRun } from "../components/historico/Table";
import { ButtonSelect } from "../components/historico/ButtonSelect";

type HistoryResponse = {
  total: number;
  limit: number;
  offset: number;
  items: HistoryRun[];
};

const PAGE_SIZE = 20;

export function HistoryPage() {
  const [filter, setFilter] = useState("todos");
  const [page, setPage] = useState(0);
  const [history, setHistory] = useState<HistoryResponse | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(false);

  useEffect(() => {
    let active = true;
    setLoading(true);
    setError(false);

    const params: Record<string, string | number> = {
      limit: PAGE_SIZE,
      offset: page * PAGE_SIZE,
    };
    if (filter !== "todos") params.resultado = filter;

    api.get<HistoryResponse>("/api/sessions/history", { params })
      .then(({ data }) => {
        if (active) setHistory(data);
      })
      .catch(() => {
        if (active) setError(true);
      })
      .finally(() => {
        if (active) setLoading(false);
      });

    return () => { active = false; };
  }, [filter, page]);

  const runs = history?.items ?? [];
  const completed = runs.filter((run) => run.resultado != null);
  const successes = completed.filter((run) => run.resultado === "sucesso").length;
  const successRate = completed.length ? (successes / completed.length) * 100 : 0;
  const timedRuns = runs.filter((run) => run.tempo_total_ms != null);
  const averageTime = timedRuns.length
    ? timedRuns.reduce((sum, run) => sum + (run.tempo_total_ms ?? 0), 0) / timedRuns.length
    : null;
  const averageCells = runs.length
    ? runs.reduce((sum, run) => sum + (run.quantidade_celulas_visitadas ?? 0), 0) / runs.length
    : null;

  return (
    <div className="flex-1 overflow-y-auto px-4 pb-4 pt-34 text-slate-200 sm:p-12">
      <header>
        <h1 className="text-[26px] text-white">Histórico</h1>
        <span className="text-[12px] text-white/30">
          {history?.total ?? 0} execuções registradas
        </span>
      </header>

      <main>
        <div className="mt-15.25 flex items-center justify-between">
          <div className="flex gap-5">
            <ButtonSelect label="Todos" value="todos" name="filtro" checked={filter === "todos"} onChange={() => { setFilter("todos"); setPage(0); }} />
            <ButtonSelect label="Sucesso" value="sucesso" name="filtro" checked={filter === "sucesso"} onChange={() => { setFilter("sucesso"); setPage(0); }} />
            <ButtonSelect label="Falhou" value="falha" name="filtro" checked={filter === "falha"} onChange={() => { setFilter("falha"); setPage(0); }} />
          </div>
          <span className="text-[12px] text-white/30">
            {history?.total ?? 0} execuções registradas
          </span>
        </div>

        <Table runs={runs} loading={loading} error={error} />

        <div className="mt-4 flex items-center justify-between text-sm text-white/60">
          <span>
            {history && history.total > 0
              ? `Exibindo ${page * PAGE_SIZE + 1}–${Math.min((page + 1) * PAGE_SIZE, history.total)} de ${history.total}`
              : ""}
          </span>
          <div className="flex gap-2">
            <button type="button" disabled={page === 0 || loading} onClick={() => setPage((current) => current - 1)} className="rounded-md border border-white/20 px-3 py-1.5 disabled:opacity-40">Anterior</button>
            <button type="button" disabled={!history || (page + 1) * PAGE_SIZE >= history.total || loading} onClick={() => setPage((current) => current + 1)} className="rounded-md border border-white/20 px-3 py-1.5 disabled:opacity-40">Próxima</button>
          </div>
        </div>

        <div className="mt-5 flex justify-between gap-4">
          <SummaryCard value={completed.length ? `${successRate.toFixed(0)}%` : "—"} label="Taxa de sucesso" color="text-[#00D492]" />
          <SummaryCard value={averageTime == null ? "—" : formatDuration(averageTime)} label="Tempo médio nesta página" color="text-[#00D3F3]" />
          <SummaryCard value={averageCells == null ? "—" : averageCells.toFixed(0)} label="Média de células nesta página" color="text-white" />
        </div>
      </main>
    </div>
  );
}

function SummaryCard({ value, label, color }: { value: string; label: string; color: string }) {
  return (
    <div className="flex h-22 w-75.5 flex-col items-center justify-center rounded-xl border border-[#1D293D]/60 bg-[#0B1628]">
      <p className={`text-[16px] font-bold ${color}`}>{value}</p>
      <span className="text-center text-[12px] text-white/60">{label}</span>
    </div>
  );
}

function formatDuration(milliseconds: number) {
  const totalSeconds = Math.floor(milliseconds / 1000);
  const minutes = Math.floor(totalSeconds / 60);
  const seconds = totalSeconds % 60;
  return `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
}
