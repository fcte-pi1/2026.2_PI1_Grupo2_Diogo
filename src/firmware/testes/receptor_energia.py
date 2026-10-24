#!/usr/bin/env python3
"""Recebe pelo WiFi os dados do firmware de teste de energia (docs/7.2).

Conecte o computador na rede "Xaropinho-Teste" e rode:

    python3 src/firmware/testes/receptor_energia.py 2.3

O número do teste é opcional e entra no nome do CSV e no relatório.
O que for digitado é enviado ao robô como comando (motores 150, parar, perfil,
corte 8.0, relatorio...). Ctrl+C encerra e mostra o resumo do ensaio.
As amostras ficam em ensaio-<teste>-AAAAMMDD-HHMMSS.csv. O resumo, a corrente
por PWM e os eventos são acrescentados ao relatorio-energia.md.
"""
import argparse
import csv
import socket
import sys
import threading
import time
from datetime import datetime

PORTA_DADOS = 4210
PORTA_COMANDOS = 4211
CAPACIDADE_MAH = 1500  # Leão LiPo 2S 1500 mAh (docs/4.2)
CAMPOS = ["boot", "motivo_reset", "t_ms", "tensao_v", "corrente_ma", "potencia_mw", "carga_mah",
          "energia_mwh", "corrente_max_ma", "estado", "sobrecorrente", "pwm", "perfil"]


def ler_teclado(enviar):
    for linha in sys.stdin:
        if linha.strip():
            enviar(linha.strip())


def consumido(amostras, campo):
    # Soma só o que foi gasto durante este ensaio. Se o contador voltar
    # (zerar, perfil ou reinício do ESP), recomeça a partir do novo valor.
    total, anterior = 0.0, None
    for a in amostras:
        valor = float(a[campo])
        if anterior is None:
            pass
        elif a["boot"] == anterior[0] and valor >= anterior[1]:
            total += valor - anterior[1]
        else:
            total += valor
        anterior = (a["boot"], valor)
    return total


def corrente_por_pwm(amostras):
    # Média em cada PWM, sem o primeiro segundo depois de cada troca (partida dos motores)
    grupos, pwm, inicio = {}, None, 0.0
    for a in amostras:
        if a["pwm"] != pwm:
            pwm, inicio = a["pwm"], a["recebido"]
        if a["recebido"] - inicio >= 1.0:
            grupos.setdefault(pwm, []).append(float(a["corrente_ma"]))
    return {p: sum(c) / len(c) for p, c in grupos.items()}


def resumo(amostras, reinicios):
    primeira, ultima = amostras[0], amostras[-1]
    duracao_s = ultima["recebido"] - primeira["recebido"]
    correntes = [float(a["corrente_ma"]) for a in amostras]
    carga = consumido(amostras, "carga_mah")
    energia_wh = consumido(amostras, "energia_mwh") / 1000
    return [
        ("Duracao", f"{duracao_s / 60:.1f} min ({len(amostras)} amostras)"),
        ("Tensao", f"{float(primeira['tensao_v']):.2f} V -> {float(ultima['tensao_v']):.2f} V"),
        ("Corrente media", f"{sum(correntes) / len(correntes):.1f} mA"),
        ("Corrente maxima", f"{max(float(a['corrente_max_ma']) for a in amostras):.1f} mA"),
        ("Carga consumida", f"{carga:.1f} mAh ({100 * carga / CAPACIDADE_MAH:.1f}% de {CAPACIDADE_MAH} mAh)"),
        ("Energia", f"{energia_wh:.3f} Wh"),
        ("Reinicios do ESP", f"{len(reinicios)}" + (f" ({', '.join(reinicios)})" if reinicios else "")),
    ]


def registrar(amostras, reinicios, eventos):
    md = []
    if not amostras:
        print("\nNenhuma amostra recebida.")
        md.append("Nenhuma amostra recebida.")
    else:
        linhas = resumo(amostras, reinicios)
        por_pwm = " · ".join(f"{p} -> {c:.1f} mA" for p, c in corrente_por_pwm(amostras).items())
        print("\n=== Resumo do ensaio ===")
        for rotulo, valor in linhas:
            print(f"{rotulo + ':':18}{valor}")
        print(f"{'Corrente por PWM:':18}{por_pwm}")
        print("A corrente maxima e contada pelo robo desde o ultimo 'perfil' ou 'zerar'.")
        md += ["\n| Medida | Valor |", "|---|---|"] + [f"| {r} | {v} |" for r, v in linhas]
        md.append(f"\nCorrente media por PWM (sem o 1o segundo apos cada troca): {por_pwm}")
    if eventos:
        md.append("\nEventos:")
        md += [f"- {hora} {texto}" for hora, texto in eventos]
    with open("relatorio-energia.md", "a") as f_rel:
        f_rel.write("\n".join(md) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("teste", nargs="?", help="ID do teste no docs/7.2 (ex.: 2.3)")
    parser.add_argument("--ip", default="192.168.4.1", help="IP do robo (padrao 192.168.4.1)")
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", PORTA_DADOS))
    sock.settimeout(1.0)

    def enviar(comando):
        sock.sendto(comando.encode(), (args.ip, PORTA_COMANDOS))

    enviar("ola")  # O robô passa a mandar os dados direto para este computador
    threading.Thread(target=ler_teclado, args=(enviar,), daemon=True).start()

    inicio = datetime.now()
    nome_csv = f"ensaio-{args.teste}-" if args.teste else "ensaio-"
    nome_csv += inicio.strftime("%Y%m%d-%H%M%S.csv")
    amostras, reinicios, eventos = [], [], []
    boot = None
    ultimo_print = 0.0
    ultima_recepcao = time.time()
    print(f"Aguardando o robo... (comandos: motores <pwm>, parar, perfil, zerar, corte <V>, "
          f"sobrecorrente <mA>, armar, relatorio)")

    with open(nome_csv, "w", newline="") as f_csv, open("relatorio-energia.md", "a") as f_rel:
        # O título vai antes, porque a tabela do 'relatorio' entra no meio do ensaio
        titulo = f"Teste {args.teste}" if args.teste else "Ensaio"
        f_rel.write(f"\n## {titulo} ({inicio.strftime('%d/%m/%Y %H:%M')})\n\n")
        f_rel.flush()
        escritor = csv.writer(f_csv)
        escritor.writerow(["hora_pc"] + CAMPOS)
        try:
            while True:
                try:
                    dados, _ = sock.recvfrom(2048)
                except socket.timeout:
                    if time.time() - ultima_recepcao > 3:
                        print("... sem dados do robo (o computador esta na rede Xaropinho-Teste?)")
                        enviar("ola")
                        ultima_recepcao = time.time()
                    continue
                ultima_recepcao = time.time()

                tipo, _, resto = dados.decode(errors="replace").rstrip("\n").partition(",")
                if tipo == "M":
                    print(f"[robo] {resto}")
                    eventos.append((datetime.now().strftime("%H:%M:%S"), resto))
                elif tipo == "R":
                    print(resto)
                    f_rel.write(resto + "\n")
                    f_rel.flush()
                elif tipo == "E":
                    valores = resto.split(",")
                    if len(valores) != len(CAMPOS):
                        continue
                    a = dict(zip(CAMPOS, valores))
                    a["recebido"] = time.time()
                    if boot is not None and a["boot"] != boot:
                        reinicios.append(a["motivo_reset"])
                        print(f"!!! ESP32 REINICIOU (motivo: {a['motivo_reset']})")
                        eventos.append((datetime.now().strftime("%H:%M:%S"),
                                        f"ESP32 reiniciou (motivo: {a['motivo_reset']})"))
                    boot = a["boot"]
                    escritor.writerow([datetime.now().isoformat(timespec="milliseconds")] + valores)
                    amostras.append(a)
                    if time.time() - ultimo_print >= 1:
                        ultimo_print = time.time()
                        print(f"{float(a['tensao_v']):5.2f} V | {float(a['corrente_ma']):7.1f} mA | "
                              f"{float(a['potencia_mw']) / 1000:5.2f} W | {float(a['carga_mah']):7.1f} mAh | "
                              f"{float(a['energia_mwh']):7.1f} mWh | PWM {a['pwm']:>3} | {a['estado']}"
                              + (" | SOBRECORRENTE" if a["sobrecorrente"] == "1" else ""))
        except KeyboardInterrupt:
            pass

    registrar(amostras, reinicios, eventos)
    print(f"Amostras salvas em {nome_csv}; resumo acrescentado em relatorio-energia.md")


if __name__ == "__main__":
    main()
