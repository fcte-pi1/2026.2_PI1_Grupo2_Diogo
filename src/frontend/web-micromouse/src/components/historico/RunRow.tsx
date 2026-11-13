import { format, isValid, parseISO } from "date-fns";
import { ptBR } from "date-fns/locale";
import type { HistoryRun } from "./Table";

export function RunRow({ run }: { run: HistoryRun }) {
  const startedAt = parseISO(run.iniciada_em);
  const date = isValid(startedAt)
    ? format(startedAt, "dd MMM yyyy HH:mm", { locale: ptBR })
    : "—";
  const seconds = Math.floor((run.tempo_total_ms ?? 0) / 1000);
  const duration = run.tempo_total_ms == null
    ? "—"
    : `${String(Math.floor(seconds / 60)).padStart(2, "0")}:${String(seconds % 60).padStart(2, "0")}`;
  const success = run.resultado === "sucesso";
  const status = run.status === "em_execucao"
    ? "Em execução"
    : run.status === "interrompido"
      ? "Interrompida"
      : success ? "Concluída" : "Concluída";

  return (
    <tr className="transition-colors hover:bg-[#111E31]">
      <td className="px-5 py-4 font-medium text-[#00D3F3]"># {run.id}</td>
      <td className="px-5 py-4">{run.labirinto_nome} ({run.labirinto_tipo})</td>
      <td className="whitespace-nowrap px-5 py-4 text-[#8A99AD]">{date}</td>
      <td className="px-5 py-4">{duration}</td>
      <td className="px-5 py-4">—</td>
      <td className="px-5 py-4">{run.quantidade_celulas_visitadas ?? "—"}</td>
      <td className="px-5 py-4">
        <span className={`rounded-md px-3 py-1 ${run.status === "interrompido" ? "bg-white/10 text-white/70" : success ? "bg-[#00BC7D]/10 text-[#00BC7D]" : "bg-[#FF6467]/10 text-[#FF6467]"}`}>
          {status}
        </span>
      </td>
      <td className="px-5 py-4 text-[#8A99AD]">
        {run.resultado === "sucesso" ? "Sucesso" : run.resultado === "falha" ? "Falha" : "—"}
      </td>
    </tr>
  );
}
