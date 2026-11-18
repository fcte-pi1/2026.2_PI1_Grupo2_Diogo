import type { ApiTelemetrySample } from "../api";
import type { Posicao } from "../types";

function direcao(anterior: Posicao | undefined, x: number, y: number): Posicao["direcao"] {
  if (!anterior) return "N";
  // y cresce para o norte, como no mapa da ESP
  if (y > anterior.y) return "N";
  if (y < anterior.y) return "S";
  if (x > anterior.x) return "L";
  return "O";
}

/**
 * Acrescenta uma posição recebida ao percurso, ignorando a repetição da última.
 *
 * O robô anda uma célula por vez. Chegar à célula de início (0,0) vindo de uma
 * célula que não é vizinha não é um passo: é a missão (re)começando. Até a
 * missão começar, a ESP ainda informa a pose antiga (por exemplo, a meta da
 * corrida anterior), então o percurso recomeça do início em vez de ligar as
 * duas posições.
 */
export function acrescentarPosicao(percurso: Posicao[], x: number, y: number): Posicao[] {
  const ultimo = percurso.at(-1);
  if (ultimo?.x === x && ultimo.y === y) return percurso;
  const recomecou = x === 0 && y === 0 && ultimo !== undefined
    && Math.abs(ultimo.x) + Math.abs(ultimo.y) > 1;
  if (recomecou) return [{ x, y, direcao: "N" }];
  return [...percurso, { x, y, direcao: direcao(ultimo, x, y) }];
}

/** Percurso gravado de uma execução, a partir das amostras do backend. */
export function percursoDasAmostras(amostras: Pick<ApiTelemetrySample, "pos_x" | "pos_y">[]): Posicao[] {
  return amostras.reduce<Posicao[]>(
    (percurso, { pos_x: x, pos_y: y }) =>
      Number.isInteger(x) && Number.isInteger(y)
        ? acrescentarPosicao(percurso, x as number, y as number)
        : percurso,
    [],
  );
}
