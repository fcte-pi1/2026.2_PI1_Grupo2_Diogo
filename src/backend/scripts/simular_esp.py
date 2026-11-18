#!/usr/bin/env python3
r"""Simula a ESP do Micromouse para testar o mapa de paredes sem o carrinho.

Faz com o backend o mesmo que o firmware V4.8:
- serve o mapa interno em GET /api/maze, no formato da ESP;
- envia telemetria para POST /api/telemetry a cada 0,5 s, no formato da ESP.

O robô fica parado até existir uma execução ativa (botão Iniciar no site).
Então explora um labirinto aleatório do tamanho do tipo escolhido no site
(8x4 = 4 de largura x 8 de altura), uma célula por passo, descobrindo as
paredes de cada célula visitada, até o canto oposto. O labirinto verdadeiro é
impresso a cada execução para comparar com o desenho do site. Ao finalizar a
execução, o simulador volta ao estado do boot e espera a próxima.

Em src/backend, com o backend apontando para http://127.0.0.1:8081 (ou
host.docker.internal:8081, no Docker):

    .venv\Scripts\python scripts\simular_esp.py
    .venv\Scripts\python scripts\simular_esp.py --seed 3 --passo 0.5
"""

import argparse
import json
import random
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.request import Request, urlopen

# Máscaras de parede do firmware (maze_state.h). Origem (0,0) no canto
# sudoeste, x cresce para leste e y para o norte.
N, L, S, O = 1, 2, 4, 8
DESLOCAMENTO = {N: (0, 1), L: (1, 0), S: (0, -1), O: (-1, 0)}
OPOSTA = {N: S, S: N, L: O, O: L}
LETRA = {N: "N", L: "L", S: "S", O: "O"}
PERIODO_TELEMETRIA_S = 0.5  # TELEMETRIA_PERIODO_MS do firmware


def gerar_labirinto(largura, altura, rng):
    """Labirinto perfeito (backtracking aleatório): há caminho entre quaisquer células."""
    paredes = [[N | L | S | O] * largura for _ in range(altura)]
    visitadas = {(0, 0)}
    pilha = [(0, 0)]
    while pilha:
        x, y = pilha[-1]
        livres = [
            (lado, x + dx, y + dy)
            for lado, (dx, dy) in DESLOCAMENTO.items()
            if 0 <= x + dx < largura and 0 <= y + dy < altura
            and (x + dx, y + dy) not in visitadas
        ]
        if not livres:
            pilha.pop()
            continue
        lado, nx, ny = rng.choice(livres)
        paredes[y][x] &= ~lado
        paredes[ny][nx] &= ~OPOSTA[lado]
        visitadas.add((nx, ny))
        pilha.append((nx, ny))
    return paredes


def explorar(paredes, objetivo):
    """Busca em profundidade que prefere se aproximar do objetivo e volta dos becos."""
    passos = [(0, 0)]
    visitadas = {(0, 0)}
    pilha = [(0, 0)]
    while pilha and pilha[-1] != objetivo:
        x, y = pilha[-1]
        abertas = [
            (x + dx, y + dy)
            for lado, (dx, dy) in DESLOCAMENTO.items()
            if not paredes[y][x] & lado and (x + dx, y + dy) not in visitadas
        ]
        if abertas:
            proxima = min(abertas, key=lambda c: abs(objetivo[0] - c[0]) + abs(objetivo[1] - c[1]))
            visitadas.add(proxima)
            pilha.append(proxima)
            passos.append(proxima)
        else:
            pilha.pop()
            if pilha:
                passos.append(pilha[-1])
    return passos


def desenhar(paredes, largura, altura):
    """Labirinto em texto, com o norte em cima (como no site)."""
    linhas = []
    for y in range(altura - 1, -1, -1):
        linhas.append("+" + "".join(
            ("---" if paredes[y][x] & N else "   ") + "+" for x in range(largura)
        ))
        meio = "|" if paredes[y][0] & O else " "
        for x in range(largura):
            marca = " S " if (x, y) == (0, 0) else " F " if (x, y) == (largura - 1, altura - 1) else "   "
            meio += marca + ("|" if paredes[y][x] & L else " ")
        linhas.append(meio)
    linhas.append("+" + "".join(
        ("---" if paredes[0][x] & S else "   ") + "+" for x in range(largura)
    ))
    return "\n".join(linhas)


class EspSimulada:
    def __init__(self, seed):
        self.seed = seed
        self.lock = threading.Lock()
        self.inicio = time.monotonic()
        self.bateria = 100.0
        self.revisao = 0
        self._configurar(4, 4)  # mapa do boot até a primeira execução

    def _configurar(self, largura, altura):
        """Novo labirinto, sem pose e com o mapa vazio, como a ESP ao ligar."""
        self.largura, self.altura = largura, altura
        self.objetivo = (largura - 1, altura - 1)
        self.paredes = gerar_labirinto(largura, altura, random.Random(self.seed))
        self.passos = explorar(self.paredes, self.objetivo)
        self.indice = None  # passo atual; None = sem pose (missão não iniciada)
        self.direcao = "N"
        self.andando = False
        self.celulas = self._mapa_do_boot()
        self.revisao += 1

    def _mapa_do_boot(self):
        """Como o Grid::init do firmware: só as bordas conhecidas, como parede."""
        celulas = []
        for y in range(self.altura):
            linha = []
            for x in range(self.largura):
                borda = (
                    (N if y == self.altura - 1 else 0) | (L if x == self.largura - 1 else 0)
                    | (S if y == 0 else 0) | (O if x == 0 else 0)
                )
                linha.append([borda, borda, 0])
            celulas.append(linha)
        return celulas

    def _observar(self, x, y):
        """Descobre as quatro faces da célula, dos dois lados de cada parede."""
        for lado, (dx, dy) in DESLOCAMENTO.items():
            parede = bool(self.paredes[y][x] & lado)
            faces = [(x, y, lado)]
            if 0 <= x + dx < self.largura and 0 <= y + dy < self.altura:
                faces.append((x + dx, y + dy, OPOSTA[lado]))
            for cx, cy, face in faces:
                celula = self.celulas[cy][cx]
                celula[0] |= face
                celula[1] = celula[1] | face if parede else celula[1] & ~face
        self.celulas[y][x][2] = 1
        self.revisao += 1

    def _paredes_mapeadas(self):
        """Paredes internas já descobertas, cada uma contada uma vez."""
        total = 0
        for y, linha in enumerate(self.celulas):
            for x, celula in enumerate(linha):
                total += bool(celula[1] & N and y + 1 < self.altura)
                total += bool(celula[1] & L and x + 1 < self.largura)
        return total

    def iniciar_missao(self, largura, altura):
        with self.lock:
            self._configurar(largura, altura)
            self.indice, self.andando = 0, True
            self._observar(0, 0)

    def parar(self):
        """Fim da execução: volta ao boot, para a pose antiga não entrar na próxima."""
        with self.lock:
            self._configurar(self.largura, self.altura)

    def avancar(self):
        """Anda uma célula; devolve (x, y, direção, paredes mapeadas) ou None se parado."""
        with self.lock:
            if not self.andando or self.indice >= len(self.passos) - 1:
                self.andando = False
                return None
            (x0, y0), (x, y) = self.passos[self.indice], self.passos[self.indice + 1]
            self.indice += 1
            self.direcao = next(
                LETRA[lado] for lado, d in DESLOCAMENTO.items() if d == (x - x0, y - y0)
            )
            self._observar(x, y)
            if self.indice == len(self.passos) - 1:
                self.andando = False
            return x, y, self.direcao, self._paredes_mapeadas()

    def mapa_json(self):
        """Mesmo formato do mazeEstadoJson() do firmware."""
        with self.lock:
            pose = None
            if self.indice is not None:
                x, y = self.passos[self.indice]
                pose = {"x": x, "y": y, "direcao": self.direcao}
            return {
                "largura": self.largura,
                "altura": self.altura,
                "objetivo": {"x": self.objetivo[0], "y": self.objetivo[1]},
                "revisao": self.revisao,
                "pose": pose,
                "celulas": [[list(celula) for celula in linha] for linha in self.celulas],
            }

    def pacote_telemetria(self):
        """Mesmo formato do telemetry_remote.cpp (V4.8)."""
        with self.lock:
            self.bateria = max(5.0, self.bateria - 0.02)
            rpm = 101 if self.andando else 0
            pacote = {
                "bateria": round(self.bateria, 1),
                "velocidade": 18.0 if self.andando else 0.0,
                "rpm_motor": rpm,
                "connected": True,
                "tempo_relativo_ms": int((time.monotonic() - self.inicio) * 1000),
                "energia": {"tensao_v": 8.1, "consumo_ma": 350.0 if self.andando else 120.0},
                "motores": {"rpm_esq": rpm, "rpm_dir": rpm},
                "imu": {"yaw_deg": 0.0},
                "sensores_distancia_mm": {"esq": 50, "frente": -7, "dir": 50},
                "posicao_estimada": None,
                "mapa_disponivel": self.indice is not None,
                "status_missao": "navegacao",
            }
            if self.indice is not None:
                x, y = self.passos[self.indice]
                pacote.update(posicao_estimada={"x": x, "y": y}, col=x, row=y)
            return pacote


def requisicao(url, corpo=None):
    dados = None if corpo is None else json.dumps(corpo).encode("utf-8")
    pedido = Request(
        url,
        data=dados,
        headers={"Content-Type": "application/json"},
        method="GET" if corpo is None else "POST",
    )
    with urlopen(pedido, timeout=2) as resposta:
        return json.loads(resposta.read().decode("utf-8"))


def dimensoes_do_site(backend):
    """Largura e altura do tipo da execução ativa: 8x4 = 8 linhas x 4 colunas."""
    historico = requisicao(f"{backend}/api/sessions/history?status=em_execucao&limit=1")
    tipo = historico["items"][0]["labirinto_tipo"] if historico["items"] else "4x4"
    linhas, colunas = (int(numero) for numero in tipo.lower().split("x"))
    return colunas, linhas


def servidor_da_esp(esp, porta):
    class Rotas(BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path.split("?")[0] != "/api/maze":
                self.send_error(404)
                return
            corpo = json.dumps(esp.mapa_json()).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(corpo)))
            self.end_headers()
            self.wfile.write(corpo)

        def log_message(self, *args):
            pass

    return ThreadingHTTPServer(("127.0.0.1", porta), Rotas)


def main():
    parser = argparse.ArgumentParser(
        description="Simula a ESP do Micromouse: mapa de paredes em /api/maze e telemetria."
    )
    parser.add_argument("--backend", default="http://127.0.0.1:8000", help="URL base do backend")
    parser.add_argument("--porta", type=int, default=8081, help="porta da ESP simulada")
    parser.add_argument("--largura", type=int, help="células no eixo x (padrão: o tipo do site)")
    parser.add_argument("--altura", type=int, help="células no eixo y (padrão: o tipo do site)")
    parser.add_argument("--seed", type=int, default=7, help="semente do labirinto aleatório")
    # O site recebe a posição 1x por segundo; o carrinho real leva ~2 s por célula.
    parser.add_argument("--passo", type=float, default=2.0, help="segundos por célula")
    args = parser.parse_args()
    fixo = (args.largura, args.altura)
    if (fixo[0] is None) != (fixo[1] is None):
        parser.error("informe --largura e --altura juntos")
    if fixo[0] is not None and not (2 <= fixo[0] <= 16 and 2 <= fixo[1] <= 16):
        parser.error("largura e altura vão de 2 a 16")
    if args.passo <= 0:
        parser.error("o passo deve ser positivo")

    backend = args.backend.rstrip("/")
    esp = EspSimulada(args.seed)
    try:
        servidor = servidor_da_esp(esp, args.porta)
    except OSError as erro:
        parser.error(f"não foi possível abrir a porta {args.porta}: {erro}")
    threading.Thread(target=servidor.serve_forever, daemon=True).start()

    print(f"ESP simulada em http://127.0.0.1:{args.porta}/api/maze")
    print("Aguardando uma execução ativa (clique em Iniciar no site)...")

    ultimo_aviso = {}

    def avisar(chave, mensagem):
        if ultimo_aviso.get(chave) != mensagem:
            ultimo_aviso[chave] = mensagem
            if mensagem:
                print(mensagem)

    execucao = None
    proxima_consulta = proximo_pacote = proximo_passo = time.monotonic()
    try:
        while True:
            agora = time.monotonic()
            if agora >= proxima_consulta:
                proxima_consulta = agora + 1.0
                try:
                    ativa = requisicao(f"{backend}/api/sessions/active")
                except (OSError, ValueError) as erro:
                    avisar("consulta", f"Backend indisponível em {backend}: {erro}")
                else:
                    avisar("consulta", None)
                    ativa_id = ativa["id"] if ativa else None
                    if ativa_id is not None and ativa_id != execucao:
                        largura, altura = fixo
                        if largura is None:
                            try:
                                largura, altura = dimensoes_do_site(backend)
                            except (OSError, ValueError, KeyError, IndexError):
                                largura, altura = 4, 4
                        esp.iniciar_missao(largura, altura)
                        proximo_passo = agora + args.passo
                        print(f"\nExecução {ativa_id}: labirinto {largura}x{altura} "
                              f"(largura x altura), seed {args.seed}:")
                        print(desenhar(esp.paredes, largura, altura))
                        print("Exploração iniciada em (0,0), voltado para N.")
                    elif ativa_id is None and execucao is not None:
                        esp.parar()
                        print("Execução encerrada: aguardando a próxima.")
                    execucao = ativa_id
            if agora >= proximo_passo:
                proximo_passo = agora + args.passo
                movimento = esp.avancar()
                if movimento:
                    x, y, direcao, paredes = movimento
                    print(f"  ({x},{y}) voltado para {direcao} | paredes mapeadas: {paredes}")
                    if (x, y) == esp.objetivo:
                        print("Objetivo atingido.")
            if agora >= proximo_pacote:
                proximo_pacote = agora + PERIODO_TELEMETRIA_S
                try:
                    requisicao(f"{backend}/api/telemetry", esp.pacote_telemetria())
                except (OSError, ValueError) as erro:
                    avisar("telemetria", f"Falha ao enviar telemetria: {erro}")
                else:
                    avisar("telemetria", None)
            time.sleep(0.05)
    except KeyboardInterrupt:
        # O servidor da ESP roda numa thread daemon e termina junto com o processo.
        print("\nSimulação encerrada.")


if __name__ == "__main__":
    main()
