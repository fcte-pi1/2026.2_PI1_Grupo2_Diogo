import { useState } from "react";

export function HistoryPage() {
  return (
    <div className="flex-1 overflow-y-auto px-4 pb-4 pt-[136px] text-slate-200 sm:p-12">
      <header>
        <h1 className="text-white text-[26px]">Histórico</h1>
        <span className="text-[12px] text-white/30">
          8 execuções registradas
        </span>
      </header>

      <main>
        <div className=" flex justify-between items-center mt-15.25 ">
          <div className="flex  gap-[20px]">
            <button className="text-[#00D3F3] bg-[#00D3F3]/10 border border-[#00D3F3]/20 px-2.5 py-1.5 rounded-md cursor-pointer ">
              Todos
            </button>

            <button className="text-[#00D3F3] bg-[#00D3F3]/10 border border-[#00D3F3]/20 px-2.5 py-1.5 rounded-md  ">
              Todos
            </button>

            <button className="text-[#00D3F3] bg-[#00D3F3]/10 border border-[#00D3F3]/20 px-2.5 py-1.5 rounded-md  ">
              Todos
            </button>
          </div>

          <span className="text-[12px] text-white/30">
            8 execuções registradas
          </span>
        </div>


<div className="mt-15   max-h-[500px]  overflow-x-auto rounded-xl border border-[#1D293D]/60 ">
      <table className="w-full bg-[#0B1628] text-sm text-white ">
          <thead className=" text-[#8A99AD] border-b border-[#1D293D] ">
            <tr className="text-left">
              <th className="px-5 py-4 font-medium">ID</th>
              <th className="px-5 py-4 font-medium">LABIRINTO</th>
              <th className="px-5 py-4 font-medium">DATA</th>
              <th className="px-5 py-4 font-medium">TEMPO</th>
              <th className="px-5 py-4 font-medium">ALGORITMO</th>
              <th className="px-5 py-4 font-medium">CÉLULAS</th>
              <th className="px-5 py-4 font-medium">STATUS</th>
              <th className="px-5 py-4 font-medium">RESULTADO</th>
            </tr>
          </thead>

          <tbody>

            <tr className="transition-colors hover:bg-[#111E31]">
              <td className="px-5 py-4 font-medium">#0047</td>
              <td className="px-5 py-4">Labirinto A</td>
              <td className="px-5 py-4 text-[#8A99AD]">18 set 2026</td>
              <td className="px-5 py-4">2:30</td>
              <td className="px-5 py-4">Flood Fill v2</td>
              <td className="px-5 py-4">123</td>
              <td className="px-5 py-4">
                <span className="rounded-md bg-[#FF6467]/10 px-3 py-1 text-[#FF6467]">
                  Falhou
                </span>
              </td>
              <td className="px-5 py-4 text-[#8A99AD]">Tempo esgotado</td>
            </tr>

            <tr className="transition-colors hover:bg-[#111E31]">
              <td className="px-5 py-4 font-medium">#0047</td>
              <td className="px-5 py-4">Labirinto A</td>
              <td className="px-5 py-4 text-[#8A99AD]">18 set 2026</td>
              <td className="px-5 py-4">2:30</td>
              <td className="px-5 py-4">Flood Fill v2</td>
              <td className="px-5 py-4">123</td>
              <td className="px-5 py-4">
                <span className="rounded-md bg-[#00BC7D]/10 px-3 py-1 text-[#00BC7D]">
                  Sucesso
                </span>
              </td>
              <td className="px-5 py-4 text-[#8A99AD]">Meta atingida</td>
            </tr>
            <tr className="transition-colors hover:bg-[#111E31]">
              <td className="px-5 py-4 font-medium">#0047</td>
              <td className="px-5 py-4">Labirinto A</td>
              <td className="px-5 py-4 text-[#8A99AD]">18 set 2026</td>
              <td className="px-5 py-4">2:30</td>
              <td className="px-5 py-4">Flood Fill v2</td>
              <td className="px-5 py-4">123</td>
              <td className="px-5 py-4">
                <span className="rounded-md bg-[#00BC7D]/10 px-3 py-1 text-[#00BC7D]">
                  Sucesso
                </span>
              </td>
              <td className="px-5 py-4 text-[#8A99AD]">Meta atingida</td>
            </tr>

            <tr className="transition-colors hover:bg-[#111E31]">
              <td className="px-5 py-4 font-medium">#0047</td>
              <td className="px-5 py-4">Labirinto A</td>
              <td className="px-5 py-4 text-[#8A99AD]">18 set 2026</td>
              <td className="px-5 py-4">2:30</td>
              <td className="px-5 py-4">Flood Fill v2</td>
              <td className="px-5 py-4">123</td>
              <td className="px-5 py-4">
                <span className="rounded-md bg-[#00BC7D]/10 px-3 py-1 text-[#00BC7D]">
                  Sucesso
                </span>
              </td>
              <td className="px-5 py-4 text-[#8A99AD]">Meta atingida</td>
            </tr>


            <tr className="transition-colors hover:bg-[#111E31]">
              <td className="px-5 py-4 font-medium">#0047</td>
              <td className="px-5 py-4">Labirinto A</td>
              <td className="px-5 py-4 text-[#8A99AD]">18 set 2026</td>
              <td className="px-5 py-4">2:30</td>
              <td className="px-5 py-4">Flood Fill v2</td>
              <td className="px-5 py-4">123</td>
              <td className="px-5 py-4">
                <span className="rounded-md bg-[#00BC7D]/10 px-3 py-1 text-[#00BC7D]">
                  Sucesso
                </span>
              </td>
              <td className="px-5 py-4 text-[#8A99AD]">Meta atingida</td>
            </tr>


            <tr className="transition-colors hover:bg-[#111E31]">
              <td className="px-5 py-4 font-medium">#0047</td>
              <td className="px-5 py-4">Labirinto A</td>
              <td className="px-5 py-4 text-[#8A99AD]">18 set 2026</td>
              <td className="px-5 py-4">2:30</td>
              <td className="px-5 py-4">Flood Fill v2</td>
              <td className="px-5 py-4">123</td>
              <td className="px-5 py-4">
                <span className="rounded-md bg-[#00BC7D]/10 px-3 py-1 text-[#00BC7D]">
                  Sucesso
                </span>
              </td>
              <td className="px-5 py-4 text-[#8A99AD]">Meta atingida</td>
            </tr>


            <tr className="transition-colors hover:bg-[#111E31]">
              <td className="px-5 py-4 font-medium">#0047</td>
              <td className="px-5 py-4">Labirinto A</td>
              <td className="px-5 py-4 text-[#8A99AD]">18 set 2026</td>
              <td className="px-5 py-4">2:30</td>
              <td className="px-5 py-4">Flood Fill v2</td>
              <td className="px-5 py-4">123</td>
              <td className="px-5 py-4">
                <span className="rounded-md bg-[#00BC7D]/10 px-3 py-1 text-[#00BC7D]">
                  Sucesso
                </span>
              </td>
              <td className="px-5 py-4 text-[#8A99AD]">Meta atingida</td>
            </tr>


            <tr className="transition-colors hover:bg-[#111E31]">
              <td className="px-5 py-4 font-medium">#0047</td>
              <td className="px-5 py-4">Labirinto A</td>
              <td className="px-5 py-4 text-[#8A99AD]">18 set 2026</td>
              <td className="px-5 py-4">2:30</td>
              <td className="px-5 py-4">Flood Fill v2</td>
              <td className="px-5 py-4">123</td>
              <td className="px-5 py-4">
                <span className="rounded-md bg-[#00BC7D]/10 px-3 py-1 text-[#00BC7D]">
                  Sucesso
                </span>
              </td>
              <td className="px-5 py-4 text-[#8A99AD]">Meta atingida</td>
            </tr>


            <tr className="transition-colors hover:bg-[#111E31]">
              <td className="px-5 py-4 font-medium">#0047</td>
              <td className="px-5 py-4">Labirinto A</td>
              <td className="px-5 py-4 text-[#8A99AD]">18 set 2026</td>
              <td className="px-5 py-4">2:30</td>
              <td className="px-5 py-4">Flood Fill v2</td>
              <td className="px-5 py-4">123</td>
              <td className="px-5 py-4">
                <span className="rounded-md bg-[#00BC7D]/10 px-3 py-1 text-[#00BC7D]">
                  Sucesso
                </span>
              </td>
              <td className="px-5 py-4 text-[#8A99AD]">Meta atingida</td>
            </tr>


            <tr className="transition-colors hover:bg-[#111E31]">
              <td className="px-5 py-4 font-medium">#0047</td>
              <td className="px-5 py-4">Labirinto A</td>
              <td className="px-5 py-4 text-[#8A99AD]">18 set 2026</td>
              <td className="px-5 py-4">2:30</td>
              <td className="px-5 py-4">Flood Fill v2</td>
              <td className="px-5 py-4">123</td>
              <td className="px-5 py-4">
                <span className="rounded-md bg-[#00BC7D]/10 px-3 py-1 text-[#00BC7D]">
                  Sucesso
                </span>
              </td>
              <td className="px-5 py-4 text-[#8A99AD]">Meta atingida</td>
            </tr>


            <tr className="transition-colors hover:bg-[#111E31]">
              <td className="px-5 py-4 font-medium">#0047</td>
              <td className="px-5 py-4">Labirinto A</td>
              <td className="px-5 py-4 text-[#8A99AD]">18 set 2026</td>
              <td className="px-5 py-4">2:30</td>
              <td className="px-5 py-4">Flood Fill v2</td>
              <td className="px-5 py-4">123</td>
              <td className="px-5 py-4">
                <span className="rounded-md bg-[#00BC7D]/10 px-3 py-1 text-[#00BC7D]">
                  Sucesso
                </span>
              </td>
              <td className="px-5 py-4 text-[#8A99AD]">Meta atingida</td>
            </tr>


          </tbody>
        </table>
        </div>


        <div  className="mt-5" >
            <div className="w-75.5 h-22 bg-[#0B1628] border border-[#1D293D]/60 rounded-xl flex flex-col items-center justify-center">
                <p className="text-[16px] font-bold text-[#00D492]">75%</p>
                <span className="text-white/60 text-[12px] ">Taxa de sucesso</span>
            </div>
        </div>
      </main>
    </div>
  );
}
