#!/usr/bin/env python3
"""Recebe pelo WiFi os dados do firmware de teste de energia (docs/7.2).

Conecte o computador na rede "Xaropinho-Teste" e rode:

    python3 src/firmware/testes/receptor_energia.py

O que for digitado é enviado ao robô como comando (motores 150, parar, perfil,
corte 8.0, relatorio...). Ctrl+C encerra e mostra o resumo do ensaio.
As amostras ficam em ensaio-AAAAMMDD-HHMMSS.csv e o relatório em relatorio-energia.md.
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


def resumo(amostras, reinicios):
    if not amostras:
        print("\nNenhuma amostra recebida.")
        return
    primeira, ultima = amostras[0], amostras[-1]
    duracao_s = ultima["recebido"] - primeira["recebido"]
    correntes = [float(a["corrente_ma"]) for a in amostras]
    # Se o ESP reiniciar, os contadores voltam a zero: soma o último valor de cada boot
    ultima_por_boot = {}
    for a in amostras:
        ultima_por_boot[a["boot"]] = a
    carga = sum(float(a["carga_mah"]) for a in ultima_por_boot.values())
    energia_wh = sum(float(a["energia_mwh"]) for a in ultima_por_boot.values()) / 1000
    corrente_max = max(float(a["corrente_max_ma"]) for a in ultima_por_boot.values())
    print("\n=== Resumo do ensaio ===")
    print(f"Duracao:          {duracao_s / 60:.1f} min ({len(amostras)} amostras)")
    print(f"Tensao:           {float(primeira['tensao_v']):.2f} V -> {float(ultima['tensao_v']):.2f} V")
    print(f"Corrente media:   {sum(correntes) / len(correntes):.1f} mA")
    print(f"Corrente maxima:  {corrente_max:.1f} mA")
    print(f"Carga consumida:  {carga:.1f} mAh ({100 * carga / CAPACIDADE_MAH:.1f}% de {CAPACIDADE_MAH} mAh)")
    print(f"Energia:          {energia_wh:.3f} Wh")
    print(f"Reinicios do ESP: {len(reinicios)}" + (f" ({', '.join(reinicios)})" if reinicios else ""))
    print("Carga e energia sao contadas pelo robo desde o ultimo 'perfil' ou 'zerar'.")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
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

    nome_csv = datetime.now().strftime("ensaio-%Y%m%d-%H%M%S.csv")
    amostras, reinicios = [], []
    boot = None
    ultimo_print = 0.0
    ultima_recepcao = time.time()
    print(f"Aguardando o robo... (comandos: motores <pwm>, parar, perfil, zerar, corte <V>, "
          f"sobrecorrente <mA>, armar, relatorio)")

    with open(nome_csv, "w", newline="") as f_csv, open("relatorio-energia.md", "a") as f_rel:
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

    resumo(amostras, reinicios)
    print(f"Amostras salvas em {nome_csv}")


if __name__ == "__main__":
    main()
