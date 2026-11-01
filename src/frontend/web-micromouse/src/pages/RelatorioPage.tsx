import { useEffect, useMemo, useState } from "react";
import { api } from "../services/api";
import { ExecutionOption } from "../components/relatorio/ExecutionOption";
import { ExecutionPreview } from "../components/relatorio/ExecutionPreview";
import { ReportActions } from "../components/relatorio/ReportActions";
import { createReportPdf } from "../utils/create_pdf";
import { toReportExecution } from "../utils/report";
import type { HistoryItem, HistoryResponse } from "../types/report";

export function RelatorioPage() {
  const [history, setHistory] = useState<HistoryItem[]>([]);
  const [selectedId, setSelectedId] = useState<string | null>(null);
  const [loading, setLoading] = useState(true);
  const [loadError, setLoadError] = useState(false);
  const [progress, setProgress] = useState(0);
  const [generating, setGenerating] = useState(false);
  const [pdfUrl, setPdfUrl] = useState("");
  const [fileSize, setFileSize] = useState(0);

  const selectedItem = history.find((item) => item.id === selectedId) ?? history[0];
  const selectedExecution = useMemo(() => selectedItem ? toReportExecution(selectedItem) : null, [selectedItem]);

  useEffect(() => {
    let active = true;
    api.get<HistoryResponse>("/api/sessions/history")
      .then(({ data }) => { if (active) setHistory(data.items ?? []); })
      .catch(() => { if (active) setLoadError(true); })
      .finally(() => { if (active) setLoading(false); });
    return () => { active = false; };
  }, []);

  useEffect(() => {
    if (!generating) return;
    const timer = window.setTimeout(() => setProgress((current) => Math.min(current + 5, 100)), 90);
    return () => window.clearTimeout(timer);
  }, [generating, progress]);

  useEffect(() => {
    if (progress !== 100 || !generating || !selectedExecution) return;
    const pdf = createReportPdf(selectedExecution);
    setPdfUrl(URL.createObjectURL(pdf));
    setFileSize(pdf.size);
    setGenerating(false);
  }, [generating, progress, selectedExecution]);

  useEffect(() => () => {
    if (pdfUrl) URL.revokeObjectURL(pdfUrl);
  }, [pdfUrl]);

  function clearGeneratedPdf() {
    if (pdfUrl) URL.revokeObjectURL(pdfUrl);
    setPdfUrl("");
    setFileSize(0);
    setProgress(0);
    setGenerating(false);
  }

  function startGeneration() {
    clearGeneratedPdf();
    setGenerating(true);
  }

  function selectExecution(id: string) {
    clearGeneratedPdf();
    setSelectedId(id);
  }

  return (
    <main className="flex-1 overflow-y-auto px-4 pb-4 pt-34 text-slate-200 sm:p-12">
      <header>
        <h1 className="text-2xl font-semibold text-slate-100">Relatórios</h1>
        <span className="text-sm text-slate-400">Geração de relatórios em PDF por execução</span>
      </header>
      <div className="mt-5 flex w-full flex-col gap-5 lg:flex-row">
        <section className="flex h-[600px] w-full shrink-0 flex-col gap-2 overflow-y-auto rounded-xl border border-[#1D293D]/60 bg-[#0B1628] px-2.5 pt-3 lg:w-[700px]" aria-label="Selecionar execução">
          <h2 className="text-sm font-medium">Selecionar Execução</h2>
          {loading && <p className="p-3 text-sm text-slate-400">Carregando execuções…</p>}
          {loadError && <p className="p-3 text-sm text-rose-300">Não foi possível carregar o histórico.</p>}
          {!loading && !loadError && history.length === 0 && <p className="p-3 text-sm text-slate-400">Nenhuma execução disponível.</p>}
          {history.map((item) => <ExecutionOption key={item.id} item={item} selected={item.id === (selectedItem?.id ?? null)} onSelect={selectExecution} />)}
        </section>
        <div className="flex min-w-0 w-full flex-col gap-4">
          {selectedExecution ? <ExecutionPreview execution={selectedExecution} /> : (
            <section className="flex min-h-[390px] items-center justify-center rounded-xl border border-[#1D293D]/60 bg-[#0B1628] p-6 text-center text-sm text-slate-400">
              Selecione uma execução para visualizar os dados do relatório.
            </section>
          )}
          <ReportActions execution={selectedExecution} generating={generating} progress={progress} pdfUrl={pdfUrl} fileSize={fileSize} onGenerate={startGeneration} />
        </div>
      </div>
    </main>
  );
}
