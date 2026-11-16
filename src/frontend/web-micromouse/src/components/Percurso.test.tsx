import { act, fireEvent, render, screen } from '@testing-library/react'
import { beforeEach, describe, expect, it, vi } from 'vitest'
import type { Posicao } from '../types'
import { Percurso } from './Percurso'

const trajectory: Posicao[] = [
  { x: 0, y: 0, direcao: 'S' },
  { x: 0, y: 1, direcao: 'S' },
  { x: 0, y: 2, direcao: 'S' },
  { x: 0, y: 3, direcao: 'S' },
  { x: 1, y: 3, direcao: 'L' },
  { x: 2, y: 3, direcao: 'L' },
  { x: 3, y: 3, direcao: 'L' },
]

const fullPathPoints = '20,20 20,60 20,100 20,140 60,140 100,140 140,140'

describe('Percurso', () => {
  beforeEach(() => {
    vi.useFakeTimers()
  })

  it.each([
    { rows: 4, columns: 4, viewBox: '-2 -2 164 164' },
    { rows: 8, columns: 4, viewBox: '-2 -2 164 324' },
    { rows: 12, columns: 4, viewBox: '-2 -2 164 484' },
  ])('renders a $rows by $columns grid', ({ rows, columns, viewBox }) => {
    const { container } = render(
      <Percurso linhas={rows} colunas={columns} trajetoria={[]} />,
    )

    expect(container.querySelector('svg')).toHaveAttribute('viewBox', viewBox)
    expect(container.querySelector('svg > rect')).toHaveAttribute('width', '160')
    expect(container.querySelector('svg > rect')).toHaveAttribute('height', String(rows * 40))
  })

  it('places the start and goal markers at the grid corners', () => {
    render(<Percurso linhas={8} colunas={4} trajetoria={[]} />)

    expect(screen.getByText('S')).toHaveAttribute('x', '20')
    expect(screen.getByText('S')).toHaveAttribute('y', '20')
    expect(screen.getByText('F')).toHaveAttribute('x', '140')
    expect(screen.getByText('F')).toHaveAttribute('y', '300')
  })

  it('draws the seed trajectory through cell centers and places the robot at its end', () => {
    const { container } = render(
      <Percurso linhas={4} colunas={4} trajetoria={trajectory} />,
    )

    expect(container.querySelector('polyline')).toHaveAttribute('points', fullPathPoints)
    const robot = container.querySelector<SVGGElement>('svg > g[style]')
    expect(robot).not.toBeNull()
    expect(robot?.style.transform).toContain('translate(140px, 140px)')
    expect(screen.getByRole('slider', { name: 'Passo do replay' })).toHaveValue('6')
  })

  it.each([
    { direction: 'N', angle: 0 },
    { direction: 'L', angle: 90 },
    { direction: 'S', angle: -180 },
    { direction: 'O', angle: -90 },
  ] as const)('orients the robot towards $direction', ({ direction, angle }) => {
    const { container } = render(
      <Percurso
        linhas={4}
        colunas={4}
        trajetoria={[{ x: 2, y: 1, direcao: direction }]}
      />,
    )

    expect(container.querySelector('svg > g[style]')).toHaveStyle({
      transform: `translate(100px, 60px) rotate(${angle}deg)`,
    })
  })

  it('shows the robot at the start with replay disabled when the trajectory is empty', () => {
    const { container } = render(<Percurso linhas={4} colunas={4} trajetoria={[]} />)

    expect(container.querySelector('polyline')).toHaveAttribute('points', '')
    expect(container.querySelector('svg > g[style]')).toHaveStyle({
      transform: 'translate(20px, 20px) rotate(0deg)',
    })
    expect(screen.getByRole('button', { name: /Reproduzir/ })).toBeDisabled()
    expect(screen.getByRole('slider', { name: 'Passo do replay' })).toBeDisabled()
    expect(screen.getByRole('slider', { name: 'Passo do replay' })).toHaveValue('0')
  })

  it('shows a single position while keeping replay disabled', () => {
    const { container } = render(
      <Percurso linhas={4} colunas={4} trajetoria={trajectory.slice(0, 1)} />,
    )

    expect(container.querySelector('polyline')).toHaveAttribute('points', '20,20')
    expect(container.querySelector('svg > g[style]')).not.toBeNull()
    expect(screen.getByRole('button', { name: /Reproduzir/ })).toBeDisabled()
    expect(screen.getByRole('slider', { name: 'Passo do replay' })).toBeDisabled()
  })

  it('can hide the replay controls while displaying the trajectory', () => {
    const { container } = render(
      <Percurso linhas={4} colunas={4} trajetoria={trajectory} mostrarControles={false} />,
    )

    expect(screen.queryByRole('button')).not.toBeInTheDocument()
    expect(screen.queryByRole('slider')).not.toBeInTheDocument()
    expect(container.querySelector('polyline')).toHaveAttribute('points', fullPathPoints)
  })

  it('starts replay at the first position and advances after 400 ms', () => {
    const { container } = render(
      <Percurso linhas={4} colunas={4} trajetoria={trajectory} />,
    )
    const slider = screen.getByRole('slider', { name: 'Passo do replay' })

    fireEvent.click(screen.getByRole('button', { name: /Reproduzir/ }))

    expect(slider).toHaveValue('0')
    expect(container.querySelector('polyline')).toHaveAttribute('points', '20,20')
    expect(screen.getByRole('button', { name: /Pausar/ })).toBeEnabled()

    act(() => { vi.advanceTimersByTime(399) })
    expect(slider).toHaveValue('0')

    act(() => { vi.advanceTimersByTime(1) })
    expect(slider).toHaveValue('1')
    expect(container.querySelector('polyline')).toHaveAttribute('points', '20,20 20,60')
  })

  it('pauses replay and resumes from the same position', () => {
    render(<Percurso linhas={4} colunas={4} trajetoria={trajectory} />)
    const slider = screen.getByRole('slider', { name: 'Passo do replay' })
    fireEvent.click(screen.getByRole('button', { name: /Reproduzir/ }))
    act(() => { vi.advanceTimersByTime(400) })

    fireEvent.click(screen.getByRole('button', { name: /Pausar/ }))
    act(() => { vi.advanceTimersByTime(2000) })
    expect(slider).toHaveValue('1')
    expect(vi.getTimerCount()).toBe(0)

    fireEvent.click(screen.getByRole('button', { name: /Reproduzir/ }))
    act(() => { vi.advanceTimersByTime(400) })
    expect(slider).toHaveValue('2')
  })

  it('finishes at the last position and can restart', () => {
    const { container } = render(
      <Percurso linhas={4} colunas={4} trajetoria={trajectory} />,
    )
    const slider = screen.getByRole('slider', { name: 'Passo do replay' })
    fireEvent.click(screen.getByRole('button', { name: /Reproduzir/ }))

    for (let step = 1; step < trajectory.length; step++) {
      act(() => { vi.advanceTimersByTime(400) })
    }

    expect(slider).toHaveValue('6')
    expect(container.querySelector('polyline')).toHaveAttribute('points', fullPathPoints)
    expect(vi.getTimerCount()).toBe(0)

    fireEvent.click(screen.getByRole('button', { name: /Reproduzir/ }))
    expect(slider).toHaveValue('0')
    expect(container.querySelector('polyline')).toHaveAttribute('points', '20,20')
  })

  it('seeks to a selected position, pauses playback, and can select the end', () => {
    const { container } = render(
      <Percurso linhas={4} colunas={4} trajetoria={trajectory} />,
    )
    const slider = screen.getByRole('slider', { name: 'Passo do replay' })
    fireEvent.click(screen.getByRole('button', { name: /Reproduzir/ }))

    fireEvent.change(slider, { target: { value: '3' } })
    act(() => { vi.advanceTimersByTime(2000) })
    expect(slider).toHaveValue('3')
    expect(container.querySelector('polyline')).toHaveAttribute(
      'points', '20,20 20,60 20,100 20,140',
    )
    expect(screen.getByRole('button', { name: /Reproduzir/ })).toBeEnabled()

    fireEvent.change(slider, { target: { value: '6' } })
    expect(container.querySelector('polyline')).toHaveAttribute('points', fullPathPoints)
    expect(vi.getTimerCount()).toBe(0)
  })

  it('clamps the selected position when the trajectory gets shorter', () => {
    const { container, rerender } = render(
      <Percurso linhas={4} colunas={4} trajetoria={trajectory} />,
    )
    const slider = screen.getByRole('slider', { name: 'Passo do replay' })
    fireEvent.change(slider, { target: { value: '5' } })

    rerender(<Percurso linhas={4} colunas={4} trajetoria={trajectory.slice(0, 2)} />)

    expect(slider).toHaveValue('1')
    expect(container.querySelector('polyline')).toHaveAttribute('points', '20,20 20,60')
  })

  it('follows new positions when the trajectory grows outside replay', () => {
    const { container, rerender } = render(
      <Percurso linhas={4} colunas={4} trajetoria={trajectory.slice(0, 2)} />,
    )

    rerender(<Percurso linhas={4} colunas={4} trajetoria={trajectory} />)

    expect(screen.getByRole('slider', { name: 'Passo do replay' })).toHaveValue('6')
    expect(container.querySelector('polyline')).toHaveAttribute('points', fullPathPoints)
  })

  it('clears the replay timer when unmounted', () => {
    const { unmount } = render(
      <Percurso linhas={4} colunas={4} trajetoria={trajectory} />,
    )
    fireEvent.click(screen.getByRole('button', { name: /Reproduzir/ }))
    expect(vi.getTimerCount()).toBe(1)

    unmount()

    expect(vi.getTimerCount()).toBe(0)
  })

  it('turns from west to north through the short rotation', () => {
    const { container } = render(
      <Percurso
        linhas={4}
        colunas={4}
        trajetoria={[
          { x: 1, y: 1, direcao: 'O' },
          { x: 1, y: 0, direcao: 'N' },
        ]}
      />,
    )
    fireEvent.click(screen.getByRole('button', { name: /Reproduzir/ }))
    expect(container.querySelector<SVGGElement>('svg > g[style]')?.style.transform)
      .toContain('rotate(-90deg)')

    act(() => { vi.advanceTimersByTime(400) })

    expect(container.querySelector<SVGGElement>('svg > g[style]')?.style.transform)
      .toContain('rotate(0deg)')
  })

  it.todo('renders detected walls from maze data')
})
