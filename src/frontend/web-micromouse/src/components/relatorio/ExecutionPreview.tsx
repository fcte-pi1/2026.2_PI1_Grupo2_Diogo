import { IoMdClock } from "react-icons/io";
import { MdBatteryChargingFull, MdGridOn, MdRotateRight, MdSpeed, MdStraighten } from "react-icons/md";
import type { ReportExecution } from "../../types/report";
import { REPORT_CONTENT } from "../../utils/report";

const METRIC_ICONS = [IoMdClock, MdSpeed, MdGridOn, MdRotateRight, MdBatteryChargingFull, MdStraighten];

export function ExecutionPreview({ execution }: { execution: ReportExecution }) {
  const metrics = [
    { label: "Tempo de execução", value: execution.duration },
    { label: "Velocidade média", value: execution.averageSpeed },
    { label: "Células visitadas", value: String(execution.cellsVisited) },
    { label: "RPM médio", value: String(execution.averageRpm) },
    { label: "Bateria consumida", value: execution.batteryConsumed },
    { label: "Distância percorrida", value: execution.distance },
  ];

  return (
    <section className="min-h-[390px] w-full rounded-xl border border-[#1D293D]/60 bg-[#0B1628] p-4">
      <div className="flex justify-between gap-3">
        <div className="min-w-0">
          <h2 className="text-sm">Execução #{execution.id}</h2>
          <span className="text-xs text-[#62748E]">{execution.maze} · {execution.algorithm} · {execution.date}</span>
        </div>
        <span className={`shrink-0 text-sm ${execution.status === "Concluída" ? "text-[#00D492]" : "text-[#FF6467]"}`}>{execution.status}</span>
      </div>
      <div className="mt-4 grid grid-cols-2 gap-4 md:grid-cols-3">
        {metrics.map(({ label, value }, index) => {
          const Icon = METRIC_ICONS[index];
          return <div key={label} className="flex min-h-[92px] flex-col justify-center rounded-lg bg-[#0F172B] p-4">
            <Icon className="h-5 w-5 text-[#00B8DB]" aria-hidden="true" />
            <p className="mt-1 text-lg text-slate-100">{value}</p>
            <span className="text-xs text-[#62748E]">{label}</span>
          </div>;
        })}
        <div className="col-span-2 min-w-0 md:col-span-3">
          <h3 className="mb-2 text-xs uppercase text-[#62748E]">Conteúdo do Relatório</h3>
          <ul className="grid grid-cols-1 gap-x-6 gap-y-2 sm:grid-cols-2">
            {REPORT_CONTENT.map((item) => <li key={item} className="flex min-w-0 items-center gap-2 text-xs text-slate-400">
              <span className="shrink-0 text-emerald-500" aria-hidden="true">✓</span><span className="min-w-0">{item}</span>
            </li>)}
          </ul>
        </div>
      </div>
    </section>
  );
}
