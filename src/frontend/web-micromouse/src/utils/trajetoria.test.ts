import { describe, expect, it } from 'vitest'
import type { Posicao } from '../types'
import { acrescentarPosicao, percursoDasAmostras } from './trajetoria'

const percorrer = (pontos: [number, number][]) =>
  pontos.reduce<Posicao[]>((percurso, [x, y]) => acrescentarPosicao(percurso, x, y), [])

describe('acrescentarPosicao', () => {
  it('liga células vizinhas com a direção do passo, com y crescendo para o norte', () => {
    expect(percorrer([[0, 0], [0, 1], [1, 1], [1, 0], [0, 0]])).toEqual([
      { x: 0, y: 0, direcao: 'N' },
      { x: 0, y: 1, direcao: 'N' },
      { x: 1, y: 1, direcao: 'L' },
      { x: 1, y: 0, direcao: 'S' },
      { x: 0, y: 0, direcao: 'O' },
    ])
  })

  it('ignora a repetição da mesma célula', () => {
    expect(percorrer([[0, 0], [0, 0], [1, 0], [1, 0]])).toHaveLength(2)
  })

  it('recomeça do início quando a pose antiga dá lugar à célula (0,0)', () => {
    expect(percorrer([[3, 3], [0, 0], [1, 0]])).toEqual([
      { x: 0, y: 0, direcao: 'N' },
      { x: 1, y: 0, direcao: 'L' },
    ])
  })

  it('mantém o percurso quando uma leitura pula uma célula no meio do caminho', () => {
    expect(percorrer([[0, 0], [0, 1], [0, 3]]).map(({ x, y }) => [x, y]))
      .toEqual([[0, 0], [0, 1], [0, 3]])
  })
})

describe('percursoDasAmostras', () => {
  it('monta o percurso gravado, sem amostras sem posição e sem a pose antiga', () => {
    const amostras = [
      { pos_x: 3, pos_y: 7 }, // pose da corrida anterior, antes da missão começar
      { pos_x: 0, pos_y: 0 },
      { pos_x: null, pos_y: null },
      { pos_x: 0, pos_y: 0 },
      { pos_x: 1, pos_y: 0 },
      { pos_x: 1, pos_y: 1 },
    ]
    expect(percursoDasAmostras(amostras).map(({ x, y }) => [x, y]))
      .toEqual([[0, 0], [1, 0], [1, 1]])
  })
})
