import LogoIconSVG from "../../assets/Logo.svg";
import BurguerSVG from "../../assets/burguer.svg";
import { NavLinkBar } from "./NavLinkBar";
import { useState } from "react";
import { useTelemetry } from "../hooks/useTelemetry";
import { RxDashboard } from "react-icons/rx";
import { GiMaze } from "react-icons/gi";
import { GoClock } from "react-icons/go";
import { LuBookText } from "react-icons/lu";

export function Sidebar() {
  const [IsOpen, setIsOpen] = useState(false);
  const { connected, stale, streamStatus } = useTelemetry();
  const connectionLabel = streamStatus === "connecting" ? "Conectando..."
    : streamStatus === "reconnecting" ? "Reconectando..."
      : !connected ? "Desconectado" : stale ? "Dados atrasados" : "Conectado";

  function handleClick() {
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

        <div className={`flex h-7 items-center justify-center gap-2 rounded-full border px-4 text-sm sm:w-50 ${
          connected && !stale ? "border-emerald-500/20 bg-emerald-500/10 text-emerald-400"
            : streamStatus === "open" && !connected ? "border-red-400/20 bg-red-400/10 text-red-400"
              : "border-amber-500/20 bg-amber-500/10 text-amber-300"
        }`}>
          <span className="h-1.5 w-1.5 rounded-full bg-current" />
          <p>{connectionLabel}</p>
        </div>

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
