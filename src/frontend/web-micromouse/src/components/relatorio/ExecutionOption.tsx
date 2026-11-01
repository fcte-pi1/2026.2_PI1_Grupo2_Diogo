import { FaCheck } from "react-icons/fa6";
import { VscErrorCompact } from "react-icons/vsc";
import type { HistoryItem } from "../../types/report";
import { formatReportDate } from "../../utils/report";

type Props = { item: HistoryItem; selected: boolean; onSelect: (id: string) => void };

export function ExecutionOption({ item, selected, onSelect }: Props) {
  const completed = item.status === "concluido" && item.resultado !== "falha";
  return (
    <button type="button" onClick={() => onSelect(item.id)} aria-pressed={selected}
      className={`flex w-full cursor-pointer items-center justify-between gap-2 rounded-lg border p-2 text-left transition hover:bg-[#00B8DB]/10 ${selected ? "border-[#00B8DB]/40 bg-[#00B8DB]/10" : "border-transparent hover:border-[#00B8DB]/20"}`}>
      <span className="flex min-w-0 gap-3">
        <span className={`mt-1 h-2 w-2 shrink-0 rounded-full ${completed ? "bg-[#00D492]" : "bg-[#FF6467]"}`} />
        <span className="flex min-w-0 flex-col gap-1 text-xs">
          <span>#{item.id}</span>
          <span className="truncate">{item.labirinto_nome} · {item.algoritmo}</span>
          <span>{formatReportDate(item.iniciada_em)}{item.finalizada_em ? ` até ${formatReportDate(item.finalizada_em)}` : ""}</span>
        </span>
      </span>
      {completed
        ? <FaCheck className="h-3 w-3 shrink-0 text-[#00D492]" aria-label="Concluída" />
        : <VscErrorCompact className="h-3 w-3 shrink-0 text-[#FF6467]" aria-label="Não concluída" />}
    </button>
  );
}
