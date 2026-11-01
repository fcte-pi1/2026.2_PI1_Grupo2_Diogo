// aqui segue somente os formatos de dados devolvidos pela api
export type ApiSession ={
    id: string,
    status: string,
    algorithm: string,
    tempo_total_ms: number | null,
}

export type ApiPosition = {
    row: number;
    col: number
}

export type ApiMaze = {
    size: number
    grid: {
        parede_norte: boolean;
        parede_sul: boolean;
        parede_leste: boolean;
        parede_oeste: boolean;
    }[][]
    start: ApiPosition
    goal: ApiPosition
}

export type ApiMetrics = {
    speed_cm_s: number
    rpm: number
    battery_pct: number
}

export type ApiEvent = {
    timestamp: string
    type: string
    message: string
}

export type ApiHistoryItem = {
    id: string;
    status: string;
    algoritmo: string;
    labirinto_nome: string;
    labirinto_tipo: string;
    tempo_total_ms: number | null;
    velocidade_media_cm_s: number | null;
    quantidade_celulas_visitadas: number | null;
    rpm: number | null;
    bateria_consumida_pct: number | null;
}

export type ApiTelemetrySample = {
    pos_x: number | null;
    pos_y: number | null;
    velocidade_instantanea_cm_s: number | null;
    rpm_motor_esq: number | null;
    rpm_motor_dir: number | null;
    nivel_bateria_pct: number | null;
}

// <---> Faz um GET na API. O <T> é o tipo do que vai voltar, quem chama é que diz.
async function get<T>(url: string): Promise<T> {
    const response = await fetch(`/api/${url}`);
    if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
    }
    return response.json()
}

export const api = {
    sessaoAtiva: ()=> get<ApiSession | null>('sessions/active'),
    labirinto: (id: string) => get<ApiMaze>(`sessions/${id}/maze`),
    trajetoria: (id: string) => get<ApiPosition[]>(`sessions/${id}/path`),
    metricas: (id: string) => get<ApiMetrics>(`sessions/${id}/metrics`),
    eventos: (id: string) => get<ApiEvent[]>(`sessions/${id}/events`),
    historico: () => get<{ items: ApiHistoryItem[] }>('sessions/history?limit=1&offset=0'),
    telemetriaExecucao: (id: string) => get<{ samples: ApiTelemetrySample[] }>(`sessions/${id}/telemetry?limit=1000&offset=0`),
}
