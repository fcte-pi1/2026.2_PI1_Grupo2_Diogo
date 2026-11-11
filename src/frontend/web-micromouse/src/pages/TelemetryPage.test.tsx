import { fireEvent, render, screen, waitFor } from '@testing-library/react'
import { beforeEach, test, expect, vi } from 'vitest'
import { TelemetryPage } from './TelemetryPage'
import { TelemetryContext } from '../contexts/telemetry'
import type { TelemetryState } from '../types/telemetry'
import { api } from '../services/api'

vi.mock('../services/api', () => ({ api: { get: vi.fn(), post: vi.fn() } }))

const empty: TelemetryState = { data: null, connected: false, stale: true, ageMs: null, streamStatus: 'open' }

beforeEach(() => {
    vi.mocked(api.get).mockResolvedValue({ data: null })
    vi.mocked(api.post).mockReset()
})

test('mostra espera por dados sem inventar bateria zerada', async () => {
    render(<TelemetryContext.Provider value={empty}><TelemetryPage /></TelemetryContext.Provider>)
    expect(screen.getByText('Aguardando dados do Micromouse.')).toBeInTheDocument()
    expect(screen.getByText('Sem dados')).toBeInTheDocument()
    expect(screen.queryByText('0%')).not.toBeInTheDocument()
    await waitFor(() => expect(screen.getByRole('button', { name: 'ESP desconectada' })).toBeDisabled())
})

function liveState(): TelemetryState {
    return {
        ...empty, connected: true, stale: false, ageMs: 0,
        data: {
            has_data: true, connected: true, stale: false, age_ms: 0, connection_age_ms: 0,
            generated_at: new Date().toISOString(), connection_timeout_ms: 5000, stale_after_ms: 1250,
            execucao_id: null, position: { row: 0, col: 0 }, movement: null,
            metrics: { battery_pct: 87, speed_cm_s: 18, rpm: 130, rpmLeft: 120, rpmRight: 140 },
        },
    }
}

test('mantém os valores e informa a perda de conexão', async () => {
    const state = liveState()
    const view = render(<TelemetryContext.Provider value={state}><TelemetryPage /></TelemetryContext.Provider>)
    expect(screen.getByText('87%')).toBeInTheDocument()
    view.rerender(<TelemetryContext.Provider value={{ ...state, connected: false, stale: true, ageMs: 6000 }}><TelemetryPage /></TelemetryContext.Provider>)
    expect(screen.getByText('87%')).toBeInTheDocument()
    expect(screen.getByText('Micromouse desconectado. Exibindo os últimos dados recebidos.')).toBeInTheDocument()
    expect(screen.getByText('Última atualização há 6 s.')).toBeInTheDocument()
    await waitFor(() => expect(screen.getByRole('button', { name: 'ESP desconectada' })).toBeDisabled())
})

test('permite iniciar a primeira corrida com pacotes recebidos e sem histórico', async () => {
    vi.mocked(api.post).mockResolvedValue({ data: { id: '2', iniciada_em: new Date().toISOString() } })
    render(<TelemetryContext.Provider value={liveState()}><TelemetryPage /></TelemetryContext.Provider>)
    const start = await screen.findByRole('button', { name: 'Iniciar' })
    fireEvent.click(start)
    await waitFor(() => expect(api.post).toHaveBeenCalledWith('/api/sessions', { tipo: '4x4' }))
    expect(await screen.findByRole('button', { name: 'Execução iniciada' })).toBeDisabled()
    expect(screen.getByText('87%')).toBeInTheDocument()
})
