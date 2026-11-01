import { fireEvent, render, screen, waitFor, within } from "@testing-library/react";
import { beforeEach, describe, expect, it, vi } from "vitest";
import { api } from "../services/api";
import { HistoryPage } from "./HistoryPage";

vi.mock("../services/api", () => ({
  api: { get: vi.fn() },
}));

const execution = {
  id: "42",
  labirinto_id: "3",
  labirinto_nome: "Labirinto 4x4",
  labirinto_tipo: "4x4",
  iniciada_em: "2026-10-25T14:30:00",
  finalizada_em: "2026-10-25T14:32:15",
  tempo_total_ms: 135000,
  status: "concluido",
  resultado: "sucesso",
  velocidade_media_cm_s: 8.4,
  quantidade_celulas_visitadas: 12,
};

function historyResponse(items = [execution], total = items.length) {
  return { data: { total, limit: 20, offset: 0, items } };
}

describe("HistoryPage", () => {
  const getHistory = vi.mocked(api.get);

  beforeEach(() => {
    vi.clearAllMocks();
  });

  it("loads and displays executions from the history endpoint", async () => {
    getHistory.mockResolvedValue(historyResponse());

    render(<HistoryPage />);

    expect(await screen.findByText("# 42")).toBeInTheDocument();
    expect(screen.getByText("Labirinto 4x4 (4x4)")).toBeInTheDocument();
    expect(within(screen.getByText("# 42").closest("tr")!).getByText("02:15")).toBeInTheDocument();
    expect(within(screen.getByText("# 42").closest("tr")!).getByText("12")).toBeInTheDocument();
    expect(within(screen.getByText("# 42").closest("tr")!).getByText("Sucesso")).toBeInTheDocument();
    expect(getHistory).toHaveBeenCalledWith("/api/sessions/history", {
      params: { limit: 20, offset: 0 },
    });
  });

  it("sends the selected result as a backend filter", async () => {
    getHistory.mockResolvedValue(historyResponse());

    render(<HistoryPage />);
    await screen.findByText("# 42");
    fireEvent.click(screen.getByRole("radio", { name: "Falhou" }));

    await waitFor(() => expect(getHistory).toHaveBeenLastCalledWith(
      "/api/sessions/history",
      { params: { limit: 20, offset: 0, resultado: "falha" } },
    ));
  });

  it("requests the next page using the backend offset", async () => {
    getHistory.mockResolvedValue(historyResponse([execution], 21));

    render(<HistoryPage />);
    await screen.findByText("# 42");
    fireEvent.click(screen.getByRole("button", { name: "Próxima" }));

    await waitFor(() => expect(getHistory).toHaveBeenLastCalledWith(
      "/api/sessions/history",
      { params: { limit: 20, offset: 20 } },
    ));
  });

  it("shows an empty state when the API has no executions", async () => {
    getHistory.mockResolvedValue(historyResponse([], 0));

    render(<HistoryPage />);

    expect(await screen.findByText("Nenhuma execução encontrada.")).toBeInTheDocument();
  });

  it("shows a helpful message when the history request fails", async () => {
    getHistory.mockRejectedValue(new Error("Backend unavailable"));

    render(<HistoryPage />);

    expect(await screen.findByText(
      "Não foi possível carregar o histórico. Verifique a conexão com o backend.",
    )).toBeInTheDocument();
  });
});
