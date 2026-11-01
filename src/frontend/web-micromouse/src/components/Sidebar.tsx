import LogoIconSVG from "../../assets/Logo.svg";
import BurguerSVG from "../../assets/burguer.svg";
import { NavLinkBar } from "./NavLinkBar";
import { useEffect, useState } from "react";
import { RxDashboard } from "react-icons/rx";
import { GiMaze } from "react-icons/gi";
import { GoClock } from "react-icons/go";
import { LuBookText } from "react-icons/lu";
type StatusConexao =  true | false

export function Sidebar() {
  const [IsOpen, setIsOpen] = useState(false);
  const [status, setStatus] = useState<StatusConexao>(false)

   useEffect(() => {
    let ws: WebSocket | undefined
    let disposed = false

    const timer = window.setTimeout(() => {
      if (disposed) return
      ws = new WebSocket('ws://localhost:8000/ws/telemetry/current')

      ws.onmessage = (event) => {
        try {
          const dados = JSON.parse(event.data)
          console.log(dados)
          setStatus(Boolean(dados.connected))
        } catch {
          setStatus(false)
        }
      }

      ws.onclose = () => setStatus(false)
      ws.onerror = () => ws?.close()
    }, 0)

    return () => {
      disposed = true
      window.clearTimeout(timer)
      ws?.close()
    }
  }, [])

  function handleClick() {
    console.log(IsOpen);
    setIsOpen(!IsOpen);
  }
  
  return (
    <nav className="flex flex-col w-full h-30 sm:h-full sm:w-75  bg-[#08111F] border-r-[0.5px] border-[#FFFFFF1A]  sm:pt-9 pt-8 justify-between sm:static fixed z-10">
      <div className="flex sm:flex-col items-center sm:justify-start justify-center gap-10  border-b-[0.5px]  border-[#FFFFFF1A] pb-10">
        <div className="  flex items-center justify-center  sm:pr-15   gap-5 ">
          <img
            src={LogoIconSVG}
            alt="Logo"
            className="sm:w-10 sm:h-10  -red-600"
          />

          <div className="">
            <p className="text-[#F1F5F9] font-bold text-sm sm:text-base">
              MicroNav
            </p>
            <span className="text-[#62748E] text-sm sm:text-base">V2.4.1</span>
          </div>
        </div>

        {status ? (
          <div className="bg-[#00BC7D]/10 sm:w-50 w-30 h-7 sm:h-6.5 justify-center gap-2 sm:justify-start sm:gap-6 flex items-center sm:py-4 sm:pl-6  rounded-[20px] border border-[#00BC7D]/20">
            <div className="w-1.5 h-1.5 rounded-full bg-[#00BC7D]" />
            <p className="text-[#00BC7D] sm:text-base text-sm">Conectado</p>
          </div>
        ) : (
          <div className="bg-[#FF6467]/10 w-50 h-6.5 gap-6 flex items-center py-4 pl-6  rounded-[20px] border border-[#FF6467]/20">
            <div className="w-1.5 h-1.5 rounded-full bg-[#FF6467]" />
            <p className="text-[#FF6467]">Desconectado</p>
          </div>
        )}

        <button className="cursor-pointer" onClick={handleClick}>
          <img
            src={BurguerSVG}
            alt="Logo"
            className="w-6 h-6  -red-600 sm:hidden"
          />
        </button>
      </div>

      {IsOpen ? (
        <div className=" flex   flex-col flex-1 items-center gap-5   py-10 sm:hidden     border-b-[0.5px]  border-[#FFFFFF1A]    bg-[#08111F]   animate-[slideDown_0.3s_ease-out] ">
          <NavLinkBar to="/" text="Dashboard">
          <RxDashboard size={20} className="text-inherit"/>
        </NavLinkBar>

        <NavLinkBar to="/labirinto" text="Labirinto">
          <GiMaze size={20} className="text-inherit"/>
        </NavLinkBar>


           <NavLinkBar to="/historico" text="Histórico">
           <GoClock size={20} className="text-inherit"/>
        </NavLinkBar>


           <NavLinkBar to="/relatorio" text="Relatórios">
          <LuBookText size={20} className="text-inherit"/>
        </NavLinkBar>
        </div>
      ) : null}

      <div className="  flex-col flex-1 items-center gap-5 pt-10  sm:flex hidden">
        <NavLinkBar to="/" text="Dashboard">
          <RxDashboard size={20} className="text-inherit"/>
        </NavLinkBar>

        <NavLinkBar to="/labirinto" text="Labirinto">
          <GiMaze size={20} className="text-inherit"/>
        </NavLinkBar>


           <NavLinkBar to="/historico" text="Histórico">
           <GoClock size={20} className="text-inherit"/>
        </NavLinkBar>


           <NavLinkBar to="/relatorio" text="Relatórios">
          <LuBookText size={20} className="text-inherit"/>
        </NavLinkBar>
      </div>

      <div className="  items-center justify-center w-full py-9 border-t-[0.5px]  border-[#FFFFFF1A]  sm:flex hidden">
        <p className="text-[#FFFFFF]/30">MICROMOUSE · SYS</p>
      </div>
    </nav>
  );
}
