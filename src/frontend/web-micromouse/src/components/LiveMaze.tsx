import { useEffect, useState, type CSSProperties } from 'react'

type LiveMazeData = {
  largura: number; altura: number; revisao: number
  pose: { x: number; y: number; direcao: 'N' | 'L' | 'S' | 'O' } | null
  objetivo: { x: number; y: number }
  celulas: [number, number, number][][]
}
type MissionData = { estado: string; ativo: boolean; motivo: string; passos: number }

export function LiveMaze() {
  const [map, setMap] = useState<LiveMazeData | null>(null)
  const [mission, setMission] = useState<MissionData | null>(null)
  const [disconnected, setDisconnected] = useState(false)
  useEffect(() => {
    let mounted = true
    async function refresh() {
      try {
        const [a, b] = await Promise.all([
          fetch('/api/telemetry/maze/live', { cache: 'no-store' }),
          fetch('/api/telemetry/mission/live', { cache: 'no-store' }),
        ])
        if (!a.ok || !b.ok) throw new Error('ESP32 desconectada')
        const maze = (await a.json()) as LiveMazeData
        const nav = (await b.json()) as MissionData
        if (mounted) { setMap(maze); setMission(nav); setDisconnected(false) }
      } catch {
        if (mounted) { setDisconnected(true); setMap(null); setMission(null) }
      }
    }
    void refresh()
    const id = window.setInterval(() => void refresh(), 1000)
    return () => { mounted = false; window.clearInterval(id) }
  }, [])
  return (
    <section className="rounded-xl border border-white/5 bg-card p-4">
      <div className="mb-3 flex flex-wrap items-center justify-between gap-3">
        <h2 className="text-sm font-semibold text-slate-100">Mapa ao vivo da ESP32</h2>
        <a href="http://192.168.4.1/missao" target="_blank" rel="noopener noreferrer" className="text-xs text-cyan-400 underline">Abrir painel da missão</a>
      </div>
      {disconnected || !map ? (
        <p className="text-sm text-slate-400">ESP32 ainda não disponível. Conecte o computador ao Wi-Fi do robô; o histórico continua disponível abaixo.</p>
      ) : (
        <>
          <p className="mb-3 text-xs text-slate-300">
            {mission?.estado ?? '—'} · {mission?.passos ?? 0} células percorridas · objetivo ({map.objetivo.x},{map.objetivo.y})
            {mission?.motivo ? ` · ${mission.motivo}` : ''}
          </p>
          <div className="mx-auto grid max-w-md" style={{ gridTemplateColumns: `repeat(${map.largura},minmax(0,1fr))` }}>
            {Array.from({ length: map.altura }, (_, row) => map.altura - 1 - row).flatMap((y) =>
              Array.from({ length: map.largura }, (_, x) => {
                const c = map.celulas[y]?.[x] ?? [0,0,0]
                const border = (n: number): string => !(c[0] & (1 << n))
                  ? '2px dotted #64748b'
                  : c[1] & (1 << n) ? '4px solid #ef7676' : '2px solid transparent'
                const style: CSSProperties = {
                  borderTop: border(0), borderRight: border(1),
                  borderBottom: border(2), borderLeft: border(3),
                }
                const current = map.pose?.x === x && map.pose.y === y
                const goal = map.objetivo.x === x && map.objetivo.y === y
                const arrow = ({ N: '↑', L: '→', S: '↓', O: '←' })[map.pose?.direcao ?? 'N']
                return <div key={`${x}-${y}`} title={`(${x},${y})`} style={style} className={`flex aspect-square items-center justify-center text-sm ${current ? 'bg-emerald-800 text-white' : goal ? 'bg-amber-900/70' : 'bg-slate-800/60'}`}>
                  {current ? arrow : goal ? '◎' : c[2] ? '·' : ''}
                </div>
              }),
            )}
          </div>
          <p className="mt-3 text-xs text-slate-400">Vermelho: parede · pontilhado: desconhecido · verde: robô · amarelo: objetivo</p>
        </>
      )}
    </section>
  )
}
