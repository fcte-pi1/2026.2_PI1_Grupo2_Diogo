import { BatteryCard } from "../components/telemetria/BatteryCard";
import { GyroscopeCard } from "../components/telemetria/GyroscopeCard";
import { MotorRPMCard } from "../components/telemetria/MotorRPMCard";
import { SpeedCard } from "../components/telemetria/SpeedCard";

import { Percurso } from "../components/Percurso";
import { useCallback, useEffect, useState } from "react";
import { LABIRINTOS, type Posicao, type TipoLabirinto } from "../types";
import { TbRestore } from "react-icons/tb";

import { toast } from "sonner";

import { api } from "../services/api";
import axios from "axios";
import { useTelemetry } from "../hooks/useTelemetry";
interface PropsExec {
  id: string | number;
  algorithm: string;
  iniciada_em?: string | Date;
  labirinto_id: number;
  status: string;
}

function segundosDesdeInicio(iniciadaEm?: string | Date): number {
  if (!iniciadaEm) return 0;
  const valor =
    typeof iniciadaEm === "string" && !/(Z|[+-]\d{2}:?\d{2})$/i.test(iniciadaEm)
      ? `${iniciadaEm}Z`
      : iniciadaEm;
  const inicioMs = new Date(valor).getTime();
  if (!Number.isFinite(inicioMs)) return 0;
  return Math.max(0, Math.floor((Date.now() - inicioMs) / 1000));
}

export function TelemetryPage() {
  const [dataExec, setDataExec] = useState<PropsExec>();
  const { data: telemetry, connected: conectada, streamStatus, stale, ageMs } = useTelemetry();
  const [tipo, setTipo] = useState<TipoLabirinto>("4x4");

  const [iniciada, setIniciada] = useState(false);
  const [inicioExecucao, setInicioExecucao] = useState<string | Date>();
  const [verificandoExecucao, setVerificandoExecucao] = useState(true);
  const [tempoSegundos, setTempoSegundos] = useState(0);
  const [trajetoria] = useState<Posicao[]>([]);
  const { linhas, colunas } = LABIRINTOS[tipo];

  const handleVerificar = useCallback(async (): Promise<PropsExec | null | undefined> => {
    try {
      const { data } = await api.get<PropsExec | null>("/api/sessions/active");
      return data;
    } catch {
      const mensagem = "Backend indisponível. Verifique se o servidor está ativo.";
      toast.error(mensagem);
      return undefined;
    } finally {
      setVerificandoExecucao(false);
    }
  }, []);

  useEffect(() => {
    if (streamStatus === "reconnecting") return;
    let mounted = true;

    const atualizarExecucaoAtiva = async () => {
      const execucaoAtiva = await handleVerificar();
      if (!mounted || execucaoAtiva === undefined) return;
      setIniciada(Boolean(execucaoAtiva));
      setDataExec(execucaoAtiva ?? undefined);
      if (execucaoAtiva) {
        setDataExec(execucaoAtiva);
        setInicioExecucao(execucaoAtiva.iniciada_em);
        setTempoSegundos(segundosDesdeInicio(execucaoAtiva.iniciada_em));
      } else {
        setInicioExecucao(undefined);
        setTempoSegundos(0);
      }
    };

    void atualizarExecucaoAtiva();
    return () => {
      mounted = false;
    };
  }, [handleVerificar, streamStatus]);
  async function handleIniciarExecucao() {
    if (!conectada) {
      toast("Conecte a ESP antes de iniciar a execução.");
      return false;
    }
    try {
    
      const { data } = await api.post<PropsExec>("/api/sessions", {
        tipo,
      });
      setDataExec(data);
      setInicioExecucao(data.iniciada_em);
      setTempoSegundos(segundosDesdeInicio(data.iniciada_em));
      return true;
    } catch (err) {
      if (axios.isAxiosError(err)) {
        const mensagem = err.response
          ? err.response.data.detail ?? "Não foi possível iniciar a execução."
          : "Backend indisponível. Verifique se o servidor está ativo.";
        toast.error(mensagem);
      } else {
        toast.error("Erro inesperado ao iniciar a execução.");
      }

      return false;
    }
  }

  async function handleStopExecucao() {
    // # verificiar se chegou no destino final
    // falha  sucesso
    const status = "interrompido";
    // em_execucao, concluido, interrompido
    const resultado = "falha";

    try {
      await api.post(
        `/api/sessions/${dataExec?.id}/finish`,
        {
          status: status,
          resultado: resultado,
        },
      );
      return true;
    } catch (err) {
      if (axios.isAxiosError(err)) {
        const mensagem = err.response
          ? err.response.data.detail ?? "Não foi possível parar a execução."
          : "Backend indisponível. Verifique se o servidor está ativo.";
        toast.error(mensagem);
      } else {
        toast.error("Erro inesperado ao parar a execução.");
      }

      return false;
    }
  }

  useEffect(() => {
    if (!iniciada) return;
    const atualizarTempo = () => {
      setTempoSegundos(segundosDesdeInicio(inicioExecucao));
    };
    atualizarTempo();
    const id = window.setInterval(atualizarTempo, 1000);
    return () => window.clearInterval(id);
  }, [iniciada, inicioExecucao]);

  const tempo = `${String(Math.floor(tempoSegundos / 60)).padStart(2, "0")}:${String(tempoSegundos % 60).padStart(2, "0")}`;
  const posicaoAtual = trajetoria.at(-1);
  const metric = (value: unknown) => {
    return typeof value === "number" && Number.isFinite(value) ? value : null;
  };
  const mensagemConexao = streamStatus === "connecting"
    ? "Conectando à telemetria..."
    : streamStatus === "reconnecting"
      ? "Conexão interrompida. Tentando reconectar..."
      : !telemetry?.has_data
        ? "Aguardando dados do Micromouse."
        : !conectada
          ? "Micromouse desconectado. Exibindo os últimos dados recebidos."
          : stale
            ? "Telemetria atrasada. Exibindo os últimos dados recebidos."
            : "Telemetria atualizada.";

  return (
    <main className="flex-1 overflow-y-auto px-4 pb-4 pt-34 text-slate-200 sm:p-12">
      <header className="mb-6 flex flex-wrap items-center justify-between gap-3">
        <h1 className="text-2xl font-semibold text-slate-100">Telemetria</h1>
        <div role="status" aria-live="polite" className={`w-full rounded-md border px-4 py-3 text-sm ${stale ? "border-amber-500/30 bg-amber-500/10 text-amber-200" : "border-emerald-500/30 bg-emerald-500/10 text-emerald-200"}`}>
          <p>{mensagemConexao}</p>
          {ageMs !== null && <p className="mt-1 text-xs">Última atualização há {Math.floor(ageMs / 1000)} s.</p>}
        </div>
        <div
          className="
          grid w-full gap-4 sm:gap-5
          grid-cols-[repeat(auto-fit,minmax(240px,1fr))]
        "
        >
          {/* BATERIA  quando a bateria esta no medio da a messagem*/}

          <BatteryCard battery={metric(telemetry?.metrics?.battery_pct)} />

          {/* VELOCIDADE */}
          <SpeedCard speed={metric(telemetry?.metrics?.speed_cm_s)} />

          {/* MOTORES */}
          <MotorRPMCard
            leftRpm={metric(telemetry?.metrics?.rpmLeft)}
            rightRpm={metric(telemetry?.metrics?.rpmRight)}
          />

          {/* GIROSCÓPIO */}
          <GyroscopeCard
            x={metric(telemetry?.movement?.giro_x_deg_s)}
            y={metric(telemetry?.movement?.giro_y_deg_s)}
            z={metric(telemetry?.movement?.giro_z_deg_s)}
          />
        </div>
      </header>
      <div className="mt-6 grid items-start gap-4 lg:grid-cols-2">
        <section className="rounded-xl border border-white/5 bg-card p-5 lg:col-start-2 lg:row-start-1">
          <h2 className="mb-5 text-sm font-semibold text-slate-100">
            Configurar execução
          </h2>
          <label
            htmlFor="tipo-labirinto"
            className="mb-2 block text-xs text-slate-400"
          >
            Tipo do labirinto
          </label>
          <select
            id="tipo-labirinto"
            value={tipo}
            onChange={(event) => setTipo(event.target.value as TipoLabirinto)}
            disabled={iniciada}
            className="w-full rounded-md border border-slate-700 bg-[#08111F] px-3 py-2.5 text-sm text-slate-200 outline-none focus:border-cyan-500 disabled:opacity-60"
          >
            {Object.keys(LABIRINTOS).map((opcao) => (
              <option key={opcao} value={opcao}>
                {opcao}
              </option>
            ))}
          </select>
          <div className="flex items-center gap-20">
            <button
              type="button"
              onClick={async () => {
                const execucaoAtiva = await handleVerificar();
                if (execucaoAtiva === undefined) return;
                if (execucaoAtiva) {
                  setDataExec(execucaoAtiva);
                  setInicioExecucao(execucaoAtiva.iniciada_em);
                  setTempoSegundos(segundosDesdeInicio(execucaoAtiva.iniciada_em));
                  setIniciada(true);
                  toast("Já existe uma execução ativa.");
                  return;
                }

                const ok = await handleIniciarExecucao();
                if (ok) {
                  setTempoSegundos(0);
                  setIniciada(true);
                  toast("Execução Iniciada");
                } else {
                  toast("A Execução Não Foi Iniciada");
                }
              }}
              disabled={!conectada || iniciada || verificandoExecucao}
              className="mt-5 w-full rounded-md bg-cyan-600 px-4 py-2.5 text-sm font-semibold text-white transition-colors hover:bg-cyan-500 disabled:cursor-not-allowed disabled:opacity-50"
            >
              {verificandoExecucao ? "Verificando..." : !conectada ? "ESP desconectada" : iniciada ? "Execução iniciada" : "Iniciar"}
            </button>

            <button
              disabled={!iniciada}
              aria-label="Interromper execução"
              onClick={async () => {

                const ok = await handleStopExecucao();
                if (ok) {
                  setTempoSegundos(0);
                  setIniciada(false);
                  setDataExec(undefined);
                  setInicioExecucao(undefined);
                  setTempoSegundos(0);
                  toast("Execução Parada");
                } else {
                  toast("Não Foi Possível Parar a Execução");
                }
              }}
              className="flex w-10  h-10 cursor-pointer  items-center justify-center rounded-md  mt-5  bg-cyan-600/10 border border-cyan-600/20 disabled:cursor-not-allowed disabled:opacity-50 "
            >
              <TbRestore className="w-5  h-5 text-cyan-600" />
            </button>
          </div>
        </section>

        <section className="rounded-xl border border-white/5 bg-card p-5 lg:col-start-1 lg:row-span-2 lg:row-start-1">
          <h2 className="mb-5 text-sm font-semibold text-slate-100">
            Percurso
          </h2>
          <Percurso
            key={tipo}
            linhas={linhas}
            colunas={colunas}
            trajetoria={trajetoria}
            mostrarControles={false}
          />
        </section>

        <section className="rounded-xl border border-white/5 bg-card p-5 lg:col-start-2 lg:row-start-2">
          <div className="mb-5 flex items-center justify-between gap-3">
            <h2 className="text-sm font-semibold text-slate-100">
              Execução atual
            </h2>
            <span className="text-xs text-cyan-400">
              {iniciada ? "Em execução" : "Aguardando início"}
            </span>
          </div>
          <div className="grid grid-cols-3 gap-3">
            <Resumo nome="Tempo" valor={tempo} />
            <Resumo
              nome="Posição"
              valor={
                posicaoAtual ? `(${posicaoAtual.x}, ${posicaoAtual.y})` : "—"
              }
            />
            <Resumo nome="Células visitadas" valor={trajetoria.length || "—"} />
          </div>
        </section>
      </div>
    </main>
  );
}

function Resumo({ nome, valor }: { nome: string; valor: string | number }) {
  return (
    <div className="rounded-md bg-bloco p-3">
      <div className="text-xs text-slate-400">{nome}</div>
      <div className="mt-2 text-sm font-semibold text-slate-100">{valor}</div>
    </div>
  );
}
