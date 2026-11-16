# Micromouse V4.8 — Integração de navegação + firmware + telemetria

**Estado: CANDIDATO PARA COMPILAÇÃO E VALIDAÇÃO FÍSICA. NÃO É UM BINÁRIO HOMOLOGADO PARA CORRER NO LABIRINTO.**

Arquivos de origem fornecidos:
- `2026.2_PI1_Grupo2_Diogo-main.zip`: backend/frontend e documentação mais recentes.
- `2026.2_PI1_Grupo2_Diogo-integracao-firmware(1)(1).zip`: firmware de controle e sensores V4.7.

**Observação importante:** o ZIP `main` não continha um módulo de navegação embarcada (`Flood Fill`, A* ou exploração). Continha um *mock* de labirinto no backend. Nesta entrega, foi criado um **planejador BFS embarcado com exploração de arestas conhecidas**, separado dos controladores.

## 1. O que funciona no código

- Nova eletrônica preservada: canal A=esquerdo, B=direito; encoders E=32/33 D=14/13; lasers E=18, frontal=19, D=23.
- Drivers dos motores, controle PI, giro de 90° com sincronismo suave, IMU, lasers, bateria, Wi-Fi e telemetria preservados da integração enviada.
- Mapa em SRAM de até 16×16 células; máscaras de paredes conhecidas/presentes e visitadas; paredes recíprocas e bordas fechadas.
- Planejador em largura (BFS): busca objetivo por passagens CONFIRMADAS, caso contrário avança pela próxima célula não visitada acessível. Nunca atravessa aresta desconhecida.
- Máquina de estados não bloqueante: observar → decidir → girar → observar → avançar → confirmar o encoder → atualizar posição → repetir.
- Falha de laser, bateria, controle, timeout ou contradição de paredes interrompe a missão.
- API da missão e mapa no robô: `/missao`, `/api/nav/status`, `/api/maze`, `/api/nav/start` e `/api/nav/stop`.
- Backend opcional: `GET /api/telemetry/maze/live` e `/api/telemetry/mission/live` fazem proxy para a ESP32. A página **Labirinto** do React mostra o mapa ao vivo quando o robô está conectado; o histórico antigo continua separado.
- Telemetria remota de RPM/bateria/velocidade agora inclui `row` e `col` quando a pose é conhecida. **Ainda não persiste as paredes na tabela de paredes do PostgreSQL**; o mapa ao vivo é consultado diretamente na ESP32.

## 2. Antes de habilitar AUTONOMIA — obrigatório

No arquivo `src/firmware/config.h`, a autonomia é **propositalmente desabilitada**:

```cpp
constexpr bool NAV_AUTONOMIA_HABILITADA = false;
constexpr bool NAV_LATERAIS_CALIBRADOS = false;
constexpr float NAV_U3_ATE_NARIZ_MM = -1.0f;
constexpr float NAV_CELULA_MM = 180.0f;
```

Não troque os valores até validar o conjunto.

1. **Conferir eletrônica:** PWM A/B, encoders, XSHUT e sentido dos motores. Teste o botão de PARADA e o comando serial `s`, com rodas suspensas.
2. **Calibrar laser U2/U3/U4** na posição definitiva, medindo com régua 40, 60, 90, 120, 160 e 250 mm. Ajustar `LASER_OFFSET_U2_MM`, `_U3_`, `_U4_` e os limites `LASER_PAREDE_PRESENTE_MM`/`AUSENTE_MM`. O valor deve distinguir parede lateral a 1 célula de passagem aberta no local real.
3. **Medir `NAV_U3_ATE_NARIZ_MM`:** distância entre a janela óptica do U3 e o ponto mais avançado do chassi. U3 recuado aumenta o valor. Nunca usar valor estimado. O código exige valor ≥0 e soma margem + distância estimada de reação/frenagem.
4. **Medir distância real entre centros das células:** `NAV_CELULA_MM` vem por padrão 180 mm, mas só deve ser mantido se for o passo certo do labirinto.
5. **Confirmar bateria:** LiPo 2S e limites 7,0V alerta / 6,6V corte. Compare INA219 com multímetro; não permita movimento sem leitura válida.
6. **Verificar CPRs salvos** na NVS. Referência anterior ~815,55 E e 815,48 D, apenas se a mecânica for a mesma.
7. **Validar giro direita/esquerda** e movimento de um passo real no piso, sem paredes. Medir deslocamento, derrapagem e distância de coast.
8. **Checar folga mecânica nas curvas:** nos dados anteriores o eixo até a frente media ~85mm. Uma célula de 180mm dá apenas ~5mm entre frente do robô e parede na posição central ideal; pode não haver espaço para girar. O robô deve estar fisicamente apto a girar sem encostar. Não confiar apenas no planejador de grade.

Somente depois, defina `NAV_AUTONOMIA_HABILITADA=true`, `NAV_LATERAIS_CALIBRADOS=true` e o valor REAL de `NAV_U3_ATE_NARIZ_MM`. Esta liberação é para **teste supervisionado**, não certificação automática.

**Durante a autonomia, a centralização pelas paredes laterais continua DESLIGADA** até ser novamente calibrada no firmware. O avanço é sincronizado por encoders, não corrige escorregamento lateral; verifique a folga antes de tentar várias células.

## 3. Compilar e gravar na ESP32

Na raiz do projeto extraído (onde está `platformio.ini`):

```bash
pio run -e esp32dev
pio run -e esp32dev -t upload
pio device monitor -b 115200
```

Recomendado: PlatformIO no VS Code, placa conectada por USB. Não habilite OTA na primeira instalação. Monitor serial informa erros de PCNT/I²C/PWM.

Para testes puros de lógica, com g++ no Linux/macOS:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -Itests/host -Isrc/firmware src/firmware/maze_state.cpp tests/host/test_maze.cpp -o /tmp/test_maze && /tmp/test_maze
g++ -std=c++17 -Wall -Wextra -Werror -DNAV_HOST_SIM -Itests/host -Isrc/firmware src/firmware/maze_state.cpp src/firmware/navigation.cpp tests/host/test_navigation_v48.cpp -o /tmp/test_nav && /tmp/test_nav
```

`NAV_HOST_SIM` libera a navegação **somente no executável de teste simulado**, não é configuração para o ESP32 real.

## 4. Teste gradual do labirinto

**Primeira sessão sem motores:** confirmar no painel local `http://192.168.4.1/` leituras recentes de E/U2, frente/U3, D/U4 e IMU e bateria. Confira o mapa vazio em `/api/maze`.

**Primeiro teste com controle:** com a área aberta, faça PWM baixo E/D, giros ±90°, `reto` 50 mm e 180 mm por encoders. Registre os motivos de parada. Rode com rodas suspensas antes de tocar o chão.

**Primeira missão pequena:** monte um labirinto 2×2 com paredes estáveis, passagem livre do início até o objetivo e **um observador ao lado do robô com corte físico de energia dos motores**. Coloque o robô na célula **(0,0)**, apontando para **Norte**, com o centro do eixo na posição calibrada. Marque largura=2, altura=2, objetivo=(0,1), e abra `http://192.168.4.1/missao` para clicar **Iniciar exploração**. Não haverá movimento antes de terminar o checklist e habilitar autonomia no código.

**Depois:** testar 2×2 com giro direito para `(1,0)`, depois 4×4 com caminhos simples e por último labirinto completo. Avaliar se o eixo e o corpo ficam alinhados com cada centro. **Não comece pelo labirinto desconhecido completo.**

A origem `(0,0)` é canto sudoeste; o eixo +Y aponta Norte, +X Leste. Para outros inícios, esta versão exige ajuste do código (não aceita configuração arbitrária de pose inicial).

### Falhas esperadas que exigem parada

- `sensores_laterais_nao_calibrados` / `medir_NAV_U3_ATE_NARIZ_MM`: preflight não concluído.
- `CPR_driver_ou_freio_indisponivel`: aguardando controlador pronto e NVS válida.
- `laser_indisponivel_ou_leitura_invalida`, `leitura_de_parede_invalida_ou_contraditoria`: não confia nas paredes.
- `sem_caminho_conhecido_seguro`: passagem não comprovada; a navegação não arrisca atravessar desconhecido.
- `obstaculo_frontal_limite_dinamico`: interrompeu antes de bater, revalidar distância laser→nariz e freio.
- `reto_dessincronia_limite`, `giro_fim_com_dessincronia`, `atraso_loop_controle`: movimento real não correspondeu ao esperado. **Não atualizar pose** nesse caso.

Para emergências: desligue a alimentação dos motores no corte físico. Os comandos `/api/nav/stop`, `/api/pwm/stop` ou `s` no serial são secundários, pois falham se a ESP32 travar.

## 5. Site externo e backend

O robô funciona sozinho, com AP Wi-Fi `Xaropinho-WiFi`, página em `192.168.4.1/missao` e o mapa em RAM. O backend é OPCIONAL para o controle da missão.

Para acompanhar pelo site React:

1. Conecte o computador ao AP do robô, confira se consegue abrir `http://192.168.4.1/api/maze` no browser. Identifique o IP do PC; **192.168.4.2 é apenas valor padrão**.
2. Ajuste `TELEMETRIA_HOST` em `src/firmware/config.h` para o IP do PC. Regrave se esse IP não for `192.168.4.2`.
3. Na pasta `src/backend`, `cp .env.example .env` e configure credenciais PostgreSQL de desenvolvimento. Há `docker-compose.yml` para subir o backend/banco juntos (`docker compose up -d --build`). Acesso ao IP da ESP a partir de dentro do container depende do roteamento do PC; se o proxy retornar 503, valide com um backend rodando diretamente no host e `MICROMOUSE_ESP_BASE_URL=http://192.168.4.1`.
4. Para rodar backend no host, instale `requirements.txt` em um venv e use `uvicorn app.main:app --host 0.0.0.0 --port 8000`, com PostgreSQL ativo e variáveis de ambiente corretas.
5. Na pasta `src/frontend/web-micromouse`, use `npm ci` seguido de `npm run dev` e abra `http://localhost:5173/labirinto`. A primeira seção exibe o mapa em tempo real; a seção de baixo mantém o histórico.
6. Para gravar telemetria no PostgreSQL, **inicie uma execução no site**; sem execução ativa, o backend atual responde HTTP 409 a `/api/telemetry`. O proxy de mapa ao vivo independe dessa sessão.

O mapa não é gravado automaticamente na flash/SQL. Desligar a ESP32 reinicia a exploração. Uma solução robusta de checkpoint e uma rota de velocidade/otimização ainda são evoluções futuras, não foram prometidas como prontas nesta versão.

## 6. Evidências desta entrega

- Passou: testes de `maze_planner` (bordas, reciprocidade, BFS e bloqueio de caminhos desconhecidos).
- Passou: testes de `navigation.cpp` com motores/sensores falsos (1 célula reta, 90° direita + 1 célula, bloqueio de sensor frontal inválido).
- Passaram anteriormente no mesmo projeto: testes host de giro e controle de movimento com hardware simulado.
- Passou: `python -m compileall` do backend (sintaxe). Não foi executado backend completo com PostgreSQL porque não há `psycopg2` instalado neste ambiente.
- Não foi possível executar `pio run` / build ESP32: PlatformIO e toolchain Arduino não estavam disponíveis e não houve acesso à internet para instalá-los.
- Não foi possível executar `npm run build`: dependências Node/React não estão instaladas neste ambiente.
- **NÃO TESTADO NO HARDWARE**: calibração dos lasers atuais, giro real, colisão, acesso do backend ao AP, precisão de passo de 180 mm no chão e navegação integral.

## 7. Arquivos novos/alterados

Novos:
- `src/firmware/maze_planner.h`, `maze_state.cpp/.h` — mapa e busca por caminhos conhecidos.
- `src/firmware/navigation.cpp/.h` — máquina de estados e pré-checagens.
- `src/firmware/mission_web_page.h` — painel embarcado.
- `tests/host/test_navigation_v48.cpp`, `test_maze_planner_v48.cpp` — testes.
- `src/frontend/web-micromouse/src/components/LiveMaze.tsx` — visualização ao vivo.

Alterados: `src/firmware/config.h`, `main.cpp`, `web_telemetry.cpp/.h`, `telemetry_remote.cpp`, backend `app/routes/telemetry.py`, frontend `pages/LabirintoPage.tsx`, `src/backend/.env.example`.

**Não** mexemos nos pinos de controle elétrico, algoritmo PI nem saídas físicas do driver de motores.
