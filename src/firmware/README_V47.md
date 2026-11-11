# Micromouse — firmware integrado V4.7 (CANDIDATO DE PRE-VALIDACAO)

**Nao foi testado na placa fisica.** Preservado mapeamento TB6612 da V4.4.2 corrigida, encoders e gyro V4.3. Inclui port do reto V4.5, ensaio U3 V4.6, unica rotina INA219 com filtro, esqueleto de mapa 16x16 e telemetria HTTP opcional. **Ainda nao ha exploracao/algoritmo de resolucao.**

## Compilar

Abra a pasta raiz no VSCode/PlatformIO, e execute `pio run -e esp32dev`. Antes da primeira energizacao, veja `VALIDACAO_INTEGRACAO.md`. Não habilite motores com a placa sem inspeção.

## Pinagem inalterada — NOVA ELETRONICA

Canal A / motor ESQUERDO: PWMA 15, AIN1 2, AIN2 4; encoder 32,33.
Canal B / motor DIREITO: PWMB 25, BIN1 26, BIN2 27; encoder 14,13.
STBY 5. I2C SDA21 SCL22. U2 ESQUERDO XSHUT18/0x30, U3 FRONTAL XSHUT19/0x31, U4 DIREITO XSHUT23/0x32. IMU 0x68. INA219 0x40.

## Painel local

Conecte ao AP existente `Xaropinho-WiFi`, abra `http://192.168.4.1`. Abas: Encoders/calibracao; Movimento/giro; Reto V4.5; Lasers. Comandos dos testes motorizados exigem *heartbeat* do navegador; isso **nao** e a logica de navegacao autonoma futura.

## Endpoints

- `GET /api/status`: bancada e estados de movimento, inclui progresso reto, U3 e heap.
- `GET /api/lasers/status`: validacao dos lasers.
- `GET /api/maze`: mapa RAM 16x16, inicialmente paredes desconhecidas e pose `null`.
- `GET /api/telemetria/remota`: estatisticas HTTP.
- `POST /api/reto/start`: teste curto, `distancia`, `rpm`, `duracao`, `sync` e opcional `frontal=1`, `limiar`.
- `POST /api/pwm/stop`: parada segura.
- `POST /api/bateria/limites`: limites temporarios de corte e sobrecorrente para ensaio, `corte` (V) e `sobrecorrente` (mA); sem parametros volta ao padrao.

## Telemetria externa

Por padrao **DESATIVADA** para evitar tentativas de rede sem backend. Edite `TELEMETRIA_REMOTA_HABILITADA` e `TELEMETRIA_HOST` em `config.h` e configure `TELEMETRIA_BATERIA_CELULAS_LIPO` conforme a bateria real. Sem celulas e leitura de tensao validas, o POST nao sera enviado. O computador roda o FastAPI em `0.0.0.0:8000` e pode se conectar ao AP da ESP32. Em cada POST `/api/telemetry` sao enviados campos planos aceitos pelo `EspTelemetryInput` da branch da equipe: `bateria`, `velocidade`, `rpm_motor`, `connected`. Metadados extras sao diagnosticos, podem nao ser persistidos pelo backend. O percentual, quando habilitado, e somente uma **estimativa por tensao de LiPo** e nao SOC preciso; valide a curva com o pacote real antes de usar no site. `posicao_estimada:null` e `mapa_disponivel:false` enquanto algoritmo de navegacao nao existir.

## Mapa

`maze_state.h/.cpp` define o contrato de escrita da navegacao: `mazeObserve`, `mazeMarkVisited`, `mazeSetPose`. Cada celula usa tres bytes (mascara de paredes conhecidas, mascara de paredes presentes, visitado). Parede desconhecida nao e considerada livre. 16x16 = 768 bytes para celulas, mais variaveis de controle. O mapa ainda nao e persistido em flash e ainda nao e atualizado automaticamente.

## O que continua pendente

- Medir distancia janela U3 -> frente fisica do chassi; calibrar U2/U4; testar distancia de parada com coast.
- Validar feedback e CPU/heap com Wi-Fi durante marcha. A leitura de lasers durante movimento so acontece com U3 frontal explicitamente armado.
- Testar corrente com o shunt/circuito concreto. A protecao de bateria (corte travado em 6,6 V) foi religada na issue #150; falta repetir na placa os testes 2.7, 2.8 e 2.9 do docs/7.2.
- Separar missao autonoma de testes com heartbeat, integrar controlador de missao/maze feito pela equipe, definir checkpoints em flash.
- Executar compilacao PlatformIO e ensaios no ESP32; nao declarar sistema homologado sem isso.

## Testes host incluidos

Veja `tests/host/README.md` para validar o estado do labirinto e o PI de movimento numa simulacao idealizada, sem placa.
