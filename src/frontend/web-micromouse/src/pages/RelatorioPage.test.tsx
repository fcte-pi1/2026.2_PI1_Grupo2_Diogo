import { fireEvent, render, screen } from "@testing-library/react";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { api } from "../services/api";
import { createReportPdf } from "../utils/create_pdf";
import { RelatorioPage } from "./RelatorioPage";

vi.mock("../services/api", () => ({ api: { get: vi.fn() } }));
vi.mock("../utils/create_pdf", () => ({ createReportPdf: vi.fn(() => new Blob(["pdf"], { type: "application/pdf" })) }));

const item = {
  id: "42",
  algoritmo: "Flood Fill",
  labirinto_nome: "Labirinto 4x4",
  iniciada_em: "2026-10-25T14:30:00Z",
  finalizada_em: "2026-10-25T14:32:15Z",
  tempo_total_ms: 135000,
  status: "concluido",
  resultado: "sucesso",
  velocidade_media_cm_s: 8.4,
  quantidade_celulas_visitadas: 12,
  rpm: 320,
  bateria_consumida_pct: 19,
};

describe("RelatorioPage", () => {
  const getHistory = vi.mocked(api.get);

  beforeEach(() => {
    vi.clearAllMocks();
    vi.stubGlobal("URL", {
      ...URL,
      createObjectURL: vi.fn(() => "blob:report"),
      revokeObjectURL: vi.fn(),
    });
  });

  afterEach(() => vi.unstubAllGlobals());

  it("loads history and shows the selected execution metrics", async () => {
    getHistory.mockResolvedValue({ data: { items: [item] } });

    render(<RelatorioPage />);

    expect(await screen.findByText("Labirinto 4x4 · Flood Fill")).toBeInTheDocument();
    expect(screen.getByText("Execução #42")).toBeInTheDocument();
    expect(screen.getByText("02:15")).toBeInTheDocument();
    expect(screen.getByText("8.4 cm/s")).toBeInTheDocument();
    expect(screen.getByText("19%")).toBeInTheDocument();
    expect(getHistory).toHaveBeenCalledWith("/api/sessions/history");
  });

  it("changes the preview when another execution is selected", async () => {
    getHistory.mockResolvedValue({ data: { items: [item, { ...item, id: "43", labirinto_nome: "Labirinto 8x8" }] } });
    render(<RelatorioPage />);

    const option = await screen.findByRole("button", { name: /#43/ });
    fireEvent.click(option);

    expect(screen.getByText("Execução #43")).toBeInTheDocument();
    expect(option).toHaveAttribute("aria-pressed", "true");
  });

  it("shows loading and failed request states", async () => {
    let rejectRequest!: (reason: Error) => void;
    getHistory.mockReturnValue(new Promise((_, reject) => { rejectRequest = reject; }));
    render(<RelatorioPage />);
    expect(screen.getByText("Carregando execuções…")).toBeInTheDocument();

    rejectRequest(new Error("offline"));
    expect(await screen.findByText("Não foi possível carregar o histórico.")).toBeInTheDocument();
  });

  it("shows an empty state when there are no executions", async () => {
    getHistory.mockResolvedValue({ data: { items: [] } });
    render(<RelatorioPage />);

    expect(await screen.findByText("Nenhuma execução disponível.")).toBeInTheDocument();
    expect(screen.getByRole("button", { name: /Gerar Relatório PDF/ })).toBeDisabled();
  });

  it("generates a PDF and exposes its download link", async () => {
    getHistory.mockResolvedValue({ data: { items: [item] } });
    render(<RelatorioPage />);
    await screen.findByText("Execução #42");

    fireEvent.click(screen.getByRole("button", { name: /Gerar Relatório PDF/ }));

    const download = await screen.findByRole("link", { name: /Exportar PDF/ }, { timeout: 5000 });
    expect(createReportPdf).toHaveBeenCalledWith(expect.objectContaining({ id: "42", cellsVisited: 12 }));
    expect(download).toHaveAttribute("href", "blob:report");
    expect(download).toHaveAttribute("download", "micronav-relatorio-42.pdf");
    expect(screen.getByText(/0.00 KB/)).toBeInTheDocument();
  });
});
