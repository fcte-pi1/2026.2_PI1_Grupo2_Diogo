import { fireEvent, render, screen, waitFor, act } from '@testing-library/react'
import { afterEach, beforeEach, describe, expect, test, vi } from 'vitest'
import axios from 'axios'
import { TelemetryPage } from './TelemetryPage'
import { api } from '../services/api'
import { toast } from 'sonner'
import '../test/setup'
import { TelemetryProvider } from '../contexts/TelemetryProvider'
import type { TelemetrySnapshot } from '../types/index'

vi.mock('../services/api', () => ({
  api: { get: vi.fn(), post: vi.fn() },
}))

vi.mock('sonner', () => ({
  toast: Object.assign(vi.fn(), { error: vi.fn() }),
}))

const mockApi = vi.mocked(api)
const mockToast = vi.mocked(toast)

class MockWebSocket {
  static instances: MockWebSocket[] = []
  onmessage: ((event: MessageEvent) => void) | null = null
  onclose: (() => void) | null = null
  onerror: (() => void) | null = null
  close = vi.fn(() => this.onclose?.())

  url: string
  constructor(url: string) {
    this.url = url
    MockWebSocket.instances.push(this)
  }

  send(data: Partial<TelemetrySnapshot>) {
    this.onmessage?.({ data: JSON.stringify({
      has_data: true, connected: true, stale: false,
      generated_at: new Date().toISOString(), age_ms: 0, connection_age_ms: 0,
      connection_timeout_ms: 5000, stale_after_ms: 1250, execucao_id: null,
      metrics: activeRun.metrics, movement: activeRun.movement, position: activeRun.position,
      ...data,
    }) } as MessageEvent)
  }
}

const activeRun = {
  id: '42',
  connected: true,
  execucao_id: 42,
  algorithm: 'BFS',
  labirinto_id: 1,
  status: 'em_execucao',
  iniciada_em: new Date().toISOString(),
  position: { col: 1, row: 2 },
  metrics: { battery_pct: 80, rpm: 120, speed_cm_s: 2.5, rpmLeft: 100, rpmRight: 110 },
  movement: { giro_x_deg_s: 1, giro_y_deg_s: 2, giro_z_deg_s: 3 },
}

function renderPage() {
  return render(<TelemetryProvider><TelemetryPage /></TelemetryProvider>)
}

async function connectEsp(data: Partial<TelemetrySnapshot> = { connected: true }) {
  await act(async () => {
    await new Promise((resolve) => setTimeout(resolve, 0))
  })
  const socket = MockWebSocket.instances.at(-1)!
  act(() => socket.send(data))
  return socket
}

async function renderReady(active: typeof activeRun | null = null) {
  mockApi.get.mockResolvedValue({ data: active ? { ...active, iniciada_em: new Date().toISOString() } : null })
  renderPage()
  await screen.findByText(active ? 'Em execução' : 'Aguardando início')
  const socket = await connectEsp()
  await screen.findByRole('button', { name: active ? 'Execução iniciada' : 'Iniciar' })
  return socket
}

describe('TelemetryPage', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    mockApi.get.mockReset()
    mockApi.post.mockReset()
    MockWebSocket.instances = []
    vi.stubGlobal('WebSocket', MockWebSocket)
  })

  afterEach(() => {
    vi.unstubAllGlobals()
    vi.useRealTimers()
  })

  test('renders the telemetry cards, maze selector, route and execution summary', async () => {
    await renderReady()

    expect(screen.getByRole('heading', { name: 'Telemetria' })).toBeInTheDocument()
    expect(screen.getByText('Bateria')).toBeInTheDocument()
    expect(screen.getByText('Velocidade instantânea')).toBeInTheDocument()
    expect(screen.getByText('Configurar execução')).toBeInTheDocument()
    expect(screen.getByText('Percurso')).toBeInTheDocument()
    expect(screen.getByText('Execução atual')).toBeInTheDocument()
    expect(screen.getByText('Aguardando início')).toBeInTheDocument()
  })

  test('loads an active run and displays its metrics and elapsed time', async () => {
    await renderReady(activeRun)

    expect(screen.getByText('Execução iniciada')).toBeInTheDocument()
    expect(screen.getByText('Em execução')).toBeInTheDocument()
    expect(screen.getByText('80%')).toBeInTheDocument()
    expect(screen.getByText('2.500')).toBeInTheDocument()
    expect(screen.getByText('00:00')).toBeInTheDocument()
  })

  test('shows backend unavailable notification when loading fails', async () => {
    mockApi.get.mockRejectedValue(new Error('offline'))
    renderPage()

    await waitFor(() => expect(mockToast.error).toHaveBeenCalledWith(
      'Backend indisponível. Verifique se o servidor está ativo.',
    ))
    expect(screen.getByText('ESP desconectada')).toBeDisabled()
  })

  test('selects a maze size and keeps the selector disabled during an active run', async () => {
    await renderReady()
    const selector = screen.getByLabelText('Tipo do labirinto')
    fireEvent.change(selector, { target: { value: '8x4' } })
    expect(selector).toHaveValue('8x4')

    const socket = MockWebSocket.instances.at(-1)!
    act(() => socket.send({ connected: true, metrics: activeRun.metrics }))
    mockApi.get.mockResolvedValueOnce({ data: activeRun })
    fireEvent.click(screen.getByRole('button', { name: 'Iniciar' }))
    await waitFor(() => expect(selector).toBeDisabled())
  })

  test('starts a run with the selected maze and updates the execution state', async () => {
    await renderReady()
    fireEvent.change(screen.getByLabelText('Tipo do labirinto'), { target: { value: '12x4' } })
    mockApi.get.mockResolvedValueOnce({ data: null })
    mockApi.post.mockResolvedValueOnce({ data: activeRun })

    fireEvent.click(screen.getByRole('button', { name: 'Iniciar' }))

    await waitFor(() => expect(mockApi.post).toHaveBeenCalledWith('/api/sessions', { tipo: '12x4' }))
    expect(await screen.findByText('Em execução')).toBeInTheDocument()
    expect(mockToast).toHaveBeenCalledWith('Execução Iniciada')
  })

  test('reports an active run instead of starting another one', async () => {
    await renderReady()
    mockApi.get.mockResolvedValueOnce({ data: activeRun })
    fireEvent.click(screen.getByRole('button', { name: 'Iniciar' }))

    await waitFor(() => expect(mockToast).toHaveBeenCalledWith('Já existe uma execução ativa.'))
    expect(mockApi.post).not.toHaveBeenCalled()
    expect(screen.getByLabelText('Tipo do labirinto')).toBeDisabled()
  })

  test('reports backend failure when starting a run', async () => {
    await renderReady()
    mockApi.get.mockResolvedValueOnce({ data: null })
    mockApi.post.mockRejectedValueOnce(new axios.AxiosError('offline'))
    fireEvent.click(screen.getByRole('button', { name: 'Iniciar' }))

    await waitFor(() => expect(mockToast.error).toHaveBeenCalledWith(
      'Backend indisponível. Verifique se o servidor está ativo.',
    ))
    expect(mockToast).toHaveBeenCalledWith('A Execução Não Foi Iniciada')
  })

  test('stops an active run and clears its summary', async () => {
    await renderReady(activeRun)
    mockApi.post.mockResolvedValueOnce({ data: {} })
    const stopButton = screen.getByRole('button', { name: 'Finalizar execução' })
    fireEvent.click(stopButton)

    await waitFor(() => expect(mockApi.post).toHaveBeenCalledWith('/api/sessions/42/finish', {
      status: 'concluido', resultado: 'sucesso',
    }))
    expect(await screen.findByText('Aguardando início')).toBeInTheDocument()
    expect(mockToast).toHaveBeenCalledWith('Execução Parada')
  })

  test('reports backend failure when stopping a run', async () => {
    await renderReady(activeRun)
    mockApi.post.mockRejectedValueOnce(new axios.AxiosError('offline'))
    fireEvent.click(screen.getByRole('button', { name: 'Finalizar execução' }))

    await waitFor(() => expect(mockToast.error).toHaveBeenCalledWith(
      'Backend indisponível. Verifique se o servidor está ativo.',
    ))
    expect(mockToast).toHaveBeenCalledWith('Não Foi Possível Parar a Execução')
  })

  test('updates connection state from websocket messages and closes the socket on unmount', async () => {
    const socket = await renderReady()
    act(() => socket.send({ connected: true }))
    expect(screen.getByRole('button', { name: 'Iniciar' })).toBeEnabled()

    act(() => socket.onclose?.())
    expect(screen.getByRole('button', { name: 'ESP desconectada' })).toBeDisabled()
  })

  test('shows waiting state without inventing readings before the first packet', async () => {
    mockApi.get.mockResolvedValue({ data: null })
    renderPage()
    await connectEsp({
      has_data: false, connected: false, stale: true, generated_at: null,
      age_ms: null, metrics: null, movement: null, position: null,
    })
    expect(screen.getByText('Aguardando dados do Micromouse.')).toBeInTheDocument()
    expect(screen.getByText('Sem dados')).toBeInTheDocument()
    expect(screen.queryByText('0%')).not.toBeInTheDocument()
  })

  test('preserves readings and displays their age after losing the ESP signal', async () => {
    const socket = await renderReady()
    act(() => socket.send({ connected: false, stale: true, age_ms: 6000 }))
    expect(screen.getByText('80%')).toBeInTheDocument()
    expect(screen.getByText('Micromouse desconectado. Exibindo os últimos dados recebidos.')).toBeInTheDocument()
    expect(screen.getByText('Última atualização há 6 s.')).toBeInTheDocument()
  })

  test('tracks row/col updates during the run without duplicating stationary positions', async () => {
    const socket = await renderReady(activeRun)
    act(() => socket.send({ position: { row: 2, col: 2 } }))
    act(() => socket.send({ position: { row: 3, col: 2 } }))
    act(() => socket.send({ position: { row: 3, col: 2 } }))
    expect(screen.getByText('(2, 3)')).toBeInTheDocument()
    expect(screen.getByText('Células visitadas').parentElement).toHaveTextContent('3')
    mockApi.post.mockResolvedValueOnce({ data: {} })
    fireEvent.click(screen.getByRole('button', { name: 'Finalizar execução' }))
    await screen.findByText('Aguardando início')
    act(() => socket.send({ position: { row: 3, col: 3 } }))
    expect(document.querySelector('polyline')).toHaveAttribute('points', '')
  })
})
