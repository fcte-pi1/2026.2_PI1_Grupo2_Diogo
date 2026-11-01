import { useEffect, useState } from "react";
import { api, type ApiPosition, type ApiTelemetrySample } from "../api";
import type { Posicao, Telemetria } from "../types";

const STATUS: Record<string, Telemetria["status"]> = {
  em_execucao: "Em execução",
  concluido: "Concluído",
  interrompido: "Interrompido",
};

function direcao(anterior: ApiPosition | undefined, atual: ApiPosition): Posicao["direcao"] {
  if (!anterior) return "N";
  if (atual.row > anterior.row) return "S";
  if (atual.row < anterior.row) return "N";
  if (atual.col > anterior.col) return "L";
  return "O";
}

function dimensoes(tipo: string) {
  const match = tipo.match(/(\d+)\s*x\s*(\d+)/i);
  return match
    ? { linhas: Number(match[1]), colunas: Number(match[2]) }
    : { linhas: 4, colunas: 4 };
}

function caminhoDasAmostras(samples: ApiTelemetrySample[]): Posicao[] {
  const pontos: ApiPosition[] = [];
  for (const sample of samples) {
    if (!Number.isInteger(sample.pos_x) || !Number.isInteger(sample.pos_y)) continue;
    const ponto = { col: sample.pos_x as number, row: sample.pos_y as number };
    const anterior = pontos.at(-1);
    if (!anterior || anterior.row !== ponto.row || anterior.col !== ponto.col) pontos.push(ponto);
  }
  return pontos.map((ponto, index) => ({
    x: ponto.col,
    y: ponto.row,
    direcao: direcao(pontos[index - 1], ponto),
  }));
}

function media(valores: Array<number | null>): number {
  const validos = valores.filter((valor): valor is number => valor != null && Number.isFinite(valor));
  return validos.length ? validos.reduce((soma, valor) => soma + valor, 0) / validos.length : 0;
}

export function useLabirinto() {
  const [telemetria, setTelemetria] = useState<Telemetria | null>(null);
  const [trajetoria, setTrajetoria] = useState<Posicao[]>([]);

  useEffect(() => {
    let mounted = true;

    async function atualizar() {
      try {
        const { items } = await api.historico();
        const ultima = items[0];
        if (!ultima) {
          if (mounted) {
            setTelemetria(null);
            setTrajetoria([]);
          }
          return;
        }

        const { samples } = await api.telemetriaExecucao(ultima.id);
        if (!mounted) return;

        const caminho = caminhoDasAmostras(samples);
        const dims = dimensoes(ultima.labirinto_tipo || ultima.labirinto_nome);
        const ultimoPonto = caminho.at(-1) ?? { x: 0, y: 0, direcao: "N" as const };
        const primeiraBateria = samples.find((sample) => sample.nivel_bateria_pct != null)?.nivel_bateria_pct;
        const ultimaBateria = [...samples].reverse().find((sample) => sample.nivel_bateria_pct != null)?.nivel_bateria_pct;
        const bateriaConsumida = primeiraBateria != null && ultimaBateria != null
          ? Math.max(0, primeiraBateria - ultimaBateria)
          : null;
        setTrajetoria(caminho);
        setTelemetria({
          status: STATUS[ultima.status] ?? "Interrompido",
          algoritmo: ultima.algoritmo,
          tempoMS: ultima.tempo_total_ms ?? 0,
          celulasPercorridas: ultima.quantidade_celulas_visitadas ?? caminho.length,
          posicao: ultimoPonto,
          velocidade: ultima.velocidade_media_cm_s ?? media(samples.map((sample) => sample.velocidade_instantanea_cm_s)),
          rpm: ultima.rpm ?? media(samples.flatMap((sample) => [sample.rpm_motor_esq, sample.rpm_motor_dir])),
          bateriaRestante: ultimaBateria ?? null,
          bateriaConsumida: ultima.bateria_consumida_pct ?? bateriaConsumida,
          labirinto: dims,
        });
      } catch {
        if (mounted) {
          setTelemetria(null);
          setTrajetoria([]);
        }
      }
    }

    void atualizar();
    const relogio = window.setInterval(() => void atualizar(), 1000);
    return () => {
      mounted = false;
      window.clearInterval(relogio);
    };
  }, []);

  return { telemetria, trajetoria };
}
