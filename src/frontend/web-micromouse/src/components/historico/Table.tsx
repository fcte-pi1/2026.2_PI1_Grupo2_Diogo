import { RunRow } from "./RunRow";

export interface HistoryRun {
  id: string;
  labirinto_id: string;
  labirinto_nome: string;
  labirinto_tipo: string;
  iniciada_em: string;
  finalizada_em: string | null;
  tempo_total_ms: number | null;
  status: string;
  resultado: string | null;
  velocidade_media_cm_s: number | null;
  quantidade_celulas_visitadas: number | null;
}

interface TableProps {
  runs: HistoryRun[];
  loading: boolean;
  error: boolean;
}

export function Table({ runs, loading, error }: TableProps) {
  let message = "";
  if (loading) message = "Carregando histórico…";
  else if (error) message = "Não foi possível carregar o histórico. Verifique a conexão com o backend.";
  else if (runs.length === 0) message = "Nenhuma execução encontrada.";

  return (
    <div className="mt-5 max-h-125 overflow-x-auto rounded-xl border border-[#1D293D]/60">
      <table className="w-full bg-[#0B1628] text-sm text-white">
        <thead className="border-b border-[#1D293D] text-[#8A99AD]">
          <tr className="text-left">
            <th className="px-5 py-4 font-medium">ID</th>
            <th className="px-5 py-4 font-medium">LABIRINTO</th>
            <th className="px-5 py-4 font-medium">DATA</th>
            <th className="px-5 py-4 font-medium">TEMPO</th>
            <th className="px-5 py-4 font-medium">ALGORITMO</th>
            <th className="px-5 py-4 font-medium">CÉLULAS</th>
            <th className="px-5 py-4 font-medium">STATUS</th>
            <th className="px-5 py-4 font-medium">RESULTADO</th>
          </tr>
        </thead>
        <tbody>
          {message ? (
            <tr><td colSpan={8} className="px-5 py-10 text-center text-[#8A99AD]">{message}</td></tr>
          ) : runs.map((run) => <RunRow key={run.id} run={run} />)}
        </tbody>
      </table>
    </div>
  );
}
