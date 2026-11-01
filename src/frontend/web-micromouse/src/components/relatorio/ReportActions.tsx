import { FaCircleCheck } from "react-icons/fa6";
import { FiDownload } from "react-icons/fi";
import { TbReportMedical } from "react-icons/tb";
import type { ReportExecution } from "../../types/report";

type Props = {
  execution: ReportExecution | null;
  generating: boolean;
  progress: number;
  pdfUrl: string;
  fileSize: number;
  onGenerate: () => void;
};

export function ReportActions({ execution, generating, progress, pdfUrl, fileSize, onGenerate }: Props) {
  return (
    <section className="flex min-h-[126px] w-full flex-col gap-4 rounded-xl border border-[#1D293D]/60 bg-[#0B1628] p-4">
      {generating ? (
        <div className="flex flex-col gap-2" aria-live="polite">
          <div className="flex justify-between text-xs text-[#62748E]"><span>Gerando relatório…</span><span>{progress}%</span></div>
          <div className="h-2 w-full overflow-hidden rounded-full bg-[#1D293D]" role="progressbar" aria-valuenow={progress} aria-valuemin={0} aria-valuemax={100}>
            <div className="h-full rounded-full bg-[#00B8DB] transition-all duration-200" style={{ width: `${progress}%` }} />
          </div>
        </div>
      ) : pdfUrl && execution ? (
        <div className="flex w-full flex-col gap-3" aria-live="polite">
          <div className="flex w-full items-center gap-3 rounded-lg border border-[#00D492]/30 bg-[#00D492]/10 p-3">
            <FaCircleCheck className="h-5 w-5 shrink-0 text-[#00D492]" aria-hidden="true" />
            <div className="min-w-0">
              <p className="text-sm font-medium text-[#00D492]">Relatório gerado com sucesso!</p>
              <p className="break-all text-xs text-[#62748E]">micronav-relatorio-{execution.id}.pdf · {(fileSize / 1024).toFixed(2)} KB</p>
            </div>
          </div>
          <div className="flex w-full items-center gap-3">
            <a href={pdfUrl} download={`micronav-relatorio-${execution.id}.pdf`} className="flex min-w-0 flex-[4] items-center justify-center gap-2 rounded-lg bg-[#00B8DB] px-3 py-2 text-sm text-white">
              <FiDownload className="h-4 w-4" aria-hidden="true" />Exportar PDF
            </a>
            <button type="button" onClick={onGenerate} className="flex-[1] cursor-pointer whitespace-nowrap rounded-lg border border-[#1D293D] px-3 py-2 text-sm">Novo relatório</button>
          </div>
        </div>
      ) : (
        <>
          <span className="text-xs text-[#62748E]">Clique em <span className="text-white">Gerar Relatório PDF</span> para compilar os dados desta execução.</span>
          <button type="button" disabled={!execution} onClick={onGenerate} className="flex cursor-pointer items-center justify-center gap-2 rounded-lg bg-[#00B8DB] px-2 py-2 text-[15px] text-white disabled:cursor-not-allowed disabled:opacity-50">
            <TbReportMedical className="h-5 w-5" aria-hidden="true" />Gerar Relatório PDF
          </button>
        </>
      )}
    </section>
  );
}
