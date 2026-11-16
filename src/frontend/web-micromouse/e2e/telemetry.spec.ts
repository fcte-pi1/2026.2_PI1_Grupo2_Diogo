import { expect, test, type WebSocketRoute } from "@playwright/test";

function snapshot(battery: number | null = null, connected = false) {
  return {
    has_data: battery !== null, connected, stale: !connected,
    generated_at: battery === null ? null : new Date().toISOString(),
    age_ms: battery === null ? null : 0,
    connection_age_ms: connected ? 0 : 6000,
    connection_timeout_ms: 5000, stale_after_ms: 1250, execucao_id: null,
    metrics: battery === null ? null : {
      battery_pct: battery, speed_cm_s: 18, rpm: 130, rpmLeft: 120, rpmRight: 140,
    },
    movement: null,
    position: battery === null ? null : { row: 0, col: 0 },
  };
}

test("RNF04: aguarda, inicia sem histórico e retoma a telemetria após queda", async ({ page }) => {
  const sockets: WebSocketRoute[] = [];
  let active = false;
  await page.route("**/api/sessions/active", route => route.fulfill({
    json: active ? { id: "2", iniciada_em: new Date().toISOString(), status: "em_execucao" } : null,
  }));
  await page.route("**/api/sessions", route => {
    active = true;
    return route.fulfill({ status: 201, json: { id: "2", iniciada_em: new Date().toISOString() } });
  });
  await page.routeWebSocket("**/ws/telemetry/current", socket => {
    sockets.push(socket);
    socket.send(JSON.stringify(snapshot()));
  });

  await page.goto("/");
  await expect(page.getByText("Aguardando dados do Micromouse.")).toBeVisible();
  await expect(page.getByRole("button", { name: "ESP desconectada" })).toBeDisabled();
  expect(sockets).toHaveLength(1);

  sockets[0].send(JSON.stringify(snapshot(87, true)));
  await expect(page.getByText("87%")).toBeVisible();
  await page.getByRole("button", { name: "Iniciar", exact: true }).click();
  await expect(page.getByRole("button", { name: "Execução iniciada" })).toBeDisabled();

  sockets[0].send(JSON.stringify(snapshot(87, false)));
  await expect(page.getByText("Micromouse desconectado. Exibindo os últimos dados recebidos.")).toBeVisible();
  await expect(page.getByText("87%")).toBeVisible();

  await sockets[0].close({ code: 1012, reason: "Reinício simulado" });
  await expect(page.getByText("Conexão interrompida. Tentando reconectar...")).toBeVisible();
  await expect(page.getByText("87%")).toBeVisible();
  await expect.poll(() => sockets.length).toBe(2);
  sockets[1].send(JSON.stringify(snapshot(86, true)));
  await expect(page.getByText("86%")).toBeVisible();
  await expect(page.getByText("Telemetria atualizada.")).toBeVisible();
  await expect(page.getByRole("button", { name: "Execução iniciada" })).toBeDisabled();
  await page.screenshot({ path: "test-results/telemetry-reconnected.png", fullPage: true });
  await page.setViewportSize({ width: 390, height: 844 });
  await expect(page.getByText("Telemetria atualizada.")).toBeVisible();
  await expect(page.getByText("86%")).toBeVisible();
  await page.screenshot({ path: "test-results/telemetry-mobile.png", fullPage: true });
});
