<!--
Instruções do template da disciplina para esta pasta:

# _Firmware_
Esta pasta deverá armazenar arquivos referentes a:

- Código-fonte para microcontroladores e SOCs: [Arduino](https://www.arduino.cc/), [ESP32](https://www.espressif.com/), [Raspberry Pi](https://www.raspberrypi.com/), dentre outros.
- Projetos de organização destes códigos, caso utilize IDEs: [Arduino IDE](https://www.arduino.cc/en/software/), [Visual Studio](https://visualstudio.microsoft.com/pt-br/downloads/) etc.
- Arquivos de configuração: ```CMakeLists.txt``` se for usar [CMake](https://cmake.org/download/), ```Makefile``` se for usar [make](https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/) etc.

Evite incluir:
- Artefatos de compilação: os arquivos de saída compilados (binários, arquivos objeto, .hex, .elf) devem ser gerados pelo processo de compilação e não incluídos no repositório.
- Arquivos temporários/específicos do sistema operacional: Arquivos como .DS_Store (macOS) ou arquivos de configuração do editor (a menos que esteja usando uma configuração compartilhada como .editorconfig).
- Configurações pessoais: Arquivos de configuração locais específicos da máquina do desenvolvedor.

> [!WARNING]
> **Não acrescente arquivos referentes a _hardware_ nesta pasta.** Eles deverão ser armazenados na pasta [hw](https://github.com/fcte-pi1/template/tree/main/hw) deste repositório.
-->

# Firmware do Micromouse

Firmware do ESP32 DevKit, feito com PlatformIO e o framework Arduino. Este documento registra o que foi implementado, como funciona, os problemas encontrados e o estado de cada parte.

> [!IMPORTANT]
> **Estado atual (integração da V4.7):** o firmware compilado hoje é a versão 4.7 de bancada, descrita em [README_V47.md](README_V47.md), com o roteiro de validação em [VALIDACAO_INTEGRACAO.md](VALIDACAO_INTEGRACAO.md). Ela ainda não foi testada na placa.
>
> As seções abaixo descrevem o firmware anterior, exceto as de energia e do giroscópio, que já valem para a V4.7. Os módulos `telemetry.cpp` e `i2c_check.cpp` continuam na pasta, mas estão fora da compilação (`build_src_filter` no `platformio.ini`), porque a V4.7 traz os seus equivalentes (`telemetry_remote.cpp` e `i2c_diagnostics.cpp`).

## Como compilar e testar

| Ação | Comando |
|---|---|
| Compilar | `pio run` |
| Gravar por USB | comentar as linhas `upload_protocol`/`upload_port` do `platformio.ini` e rodar `pio run -t upload` |
| Gravar por WiFi (OTA) | conectar na [rede do robô](#rede-wifi-do-robô) e rodar `pio run -t upload` |
| Monitor serial | `pio device monitor` (115200) |
| Testes no computador | `pio test -e native` |

O projeto usa a plataforma **pioarduino** (Arduino-ESP32 3.x), definida no `platformio.ini`.

### Rede WiFi do robô

Ao ligar, o ESP32 cria a própria rede WiFi (modo Access Point), configurada em `ota_manager.cpp`:

| | |
|---|---|
| Rede | `Xaropinho-WiFi` |
| Senha | `pi12pi34` |
| IP do robô | `192.168.4.1` |

1. Ligue o robô e espere alguns segundos (a calibração do giroscópio leva ~2,4 s).
2. No computador ou no celular, conecte na rede `Xaropinho-WiFi` com a senha acima. O computador recebe um IP da faixa 192.168.4.x, normalmente 192.168.4.2.
3. Com a conexão feita, dá para:
   - gravar o firmware por OTA (`pio run -t upload`);
   - comandar o robô pelo navegador: `http://192.168.4.1/direita`, `/esquerda` (giro de 90°) e `/parar`;
   - receber a telemetria: o robô envia os dados para `http://192.168.4.2:8000/api/telemetry`, então o backend precisa rodar no computador conectado com esse IP e aceitando conexões de fora (`uvicorn ... --host 0.0.0.0`; o padrão só aceita o próprio computador).

Enquanto estiver na rede do robô, o computador fica sem internet.

As regras de cada módulo que não dependem do hardware (contas, filtros e máquinas de estado) ficam em headers sem Arduino (`*_logic.h`) e são testadas no computador com Unity, na pasta `test/`.

## Energia: INA219 e proteção da bateria

Issue #121 (parte do INA219), PR #123. Religada no firmware integrado (V4.7) na issue #150.

### O que faz

O INA219 (endereço 0x40) fica na entrada da bateria LiPo 2S e mede a tensão e a corrente de todo o robô. A cada ciclo de 100 ms do loop, o firmware lê o sensor e:

- calcula a **tensão da bateria** como tensão do barramento + queda no shunt, que é a tensão nos terminais da bateria;
- suaviza tensão, corrente e potência com uma **média móvel exponencial** (α = 0,2);
- estima o **percentual de carga** por uma tabela de tensão por célula (3,30 V = 0% a 4,20 V = 100%), com interpolação entre os pontos;
- **corta os motores** se a tensão ficar abaixo de 6,6 V por 2 s seguidos, para proteger a LiPo contra descarga profunda. O corte vale em qualquer teste com motor e bloqueia o início de novos testes;
- avisa **bateria baixa** abaixo de 7,0 V e **sobrecorrente** acima de 2 A por 500 ms.

O corte fica **travado**: depois de acionado, não volta mesmo que a tensão suba quando os motores param. A exigência de 2 s evita cortes falsos pelas quedas rápidas de tensão na partida dos motores.

No `setup()`, a função `verificarI2C()` confere se os cinco dispositivos do barramento respondem (INA219, MPU6050 e os três lasers) e avisa se algum laser ficou no endereço padrão 0x29, o que indica falha na troca de endereço pelo XSHUT.

### Arquivos

| Arquivo | Conteúdo |
|---|---|
| `battery.h` / `battery.cpp` | Leitura do INA219, filtro e funções `getVoltage()`, `getCurrent()`, `getPowerMilliwatt()`, `getBatteryPercent()`, `bateriaCritica()`, `bateriaBaixa()` e `sobrecorrente()` |
| `power_logic.h` | Percentual de carga, máquina de estados da bateria (`MonitorBateria`) e regras dos limites de ensaio, sem Arduino |
| `i2c_check.h` / `i2c_check.cpp` | Verificação dos dispositivos I2C no boot (firmware anterior, fora da compilação) |
| `config.h` | Endereço do INA219 e limites: `BAT_V_ALERTA`, `BAT_V_CORTE`, `BAT_TEMPO_CONFIRMACAO_MS`, `I_SOBRECORRENTE_MA`, `I_SOBRECORRENTE_TEMPO_MS` |

No `main.cpp`, o loop chama `updateBattery()`. O `web_telemetry.cpp` para o teste em andamento quando `bateriaCritica()` fica verdadeiro e recusa novos testes com motor. A serial avisa com a linha `[ENERGIA]` quando o estado muda. O `/api/status` traz `bateriaEstado` (`NORMAL`, `BAIXA`, `CRITICA` ou `SEM_LEITURA`), `bateriaPct`, `sobrecorrente` e os limites em uso, e o painel mostra o estado no topo da página.

### Limites de ensaio

Para testar o corte e o aviso de sobrecorrente sem descarregar a bateria, os limites podem ser trocados com o robô parado:

| Ação | Comando (conectado na rede do robô) |
|---|---|
| Subir o corte para 8,0 V | `curl -X POST -d "corte=8.0" http://192.168.4.1/api/bateria/limites` |
| Baixar o aviso de sobrecorrente para 100 mA | `curl -X POST -d "sobrecorrente=100" http://192.168.4.1/api/bateria/limites` |
| Voltar aos limites padrão | `curl -X POST http://192.168.4.1/api/bateria/limites` |

Os limites de ensaio só deixam a proteção mais sensível: o corte aceita de 6,6 a 9,0 V e a sobrecorrente, de 50 a 2000 mA. Trocar os limites libera o corte travado, mas só se a tensão em repouso estiver em 7,0 V ou mais. Abaixo disso a bateria está descarregada de verdade, e o corte só sai depois da recarga e de reiniciar o robô.

### Testes

Doze testes no computador (`test/test_power_logic`): percentual nos limites, interpolação e monotonicidade da tabela; queda curta que não corta; subtensão sustentada que corta; corte travado; alerta antes do corte; sobrecorrente só depois do tempo limite; corte funcionando quando o `millis()` estoura; limites de ensaio que só deixam a proteção mais sensível; e corte real que não é liberado com a bateria descarregada.

### Problemas e soluções

| Problema | Solução |
|---|---|
| O `main` não compilava: `ledcAttach` não existe no core 2 do Arduino-ESP32 | Troca para a plataforma pioarduino (core 3) |
| Usar -1 como "sem leitura" no filtro falharia com corrente negativa, que aparece por ruído perto de zero | Indicador separado (`temLeitura`) para a primeira leitura |
| Uma leitura com falha no I2C entraria no filtro como zero | Cada leitura confere `success()`; se falhar, mantém a última leitura válida |
| A partida dos motores derruba a tensão por instantes e cortaria a bateria cheia | O corte exige 2 s abaixo do limite |
| O teste do estouro do `millis()` não detectava um erro inserido de propósito | Teste reforçado com uma verificação antes do tempo limite |

### Estado

- **Implementado e testes passando.**
- **Falta a bancada:** repetir na placa, com o firmware integrado, os testes 2.7, 2.8 e 2.9 do docs/7.2.

## Giroscópio: calibração, zona morta e correção do giro

Issue #119 (PR #124) no firmware anterior. Refeito sobre a IMU da V4.7 na issue #153.

### O que faz

A IMU (MPU-6050 ou MPU-6515, endereço 0x68 ou 0x69) mede a velocidade de giro no eixo Z, que é integrada para obter o ângulo do robô (yaw). Todo giroscópio tem um desvio (bias): mesmo parado, mede uma pequena rotação, e na integração isso vira um ângulo que cresce sem parar.

**Calibração.** Ao ligar, e pelo botão do painel, o firmware:

1. Lê **800 amostras** do eixo Z com o robô parado, a 200 Hz (~4 s). A coleta acontece dentro do `loop()`, sem travar o resto.
2. Calcula a **média** (o bias) e o **desvio padrão** pelo método de Welford, que não guarda as amostras na memória.
3. Se o desvio passar de 1,4 °/s, o robô foi mexido: a coleta é **repetida sozinha, até 3 tentativas**. Se todas falharem, a IMU fica sem calibração até alguém calibrar pelo painel.

**Zona morta.** Na integração, taxas abaixo de 0,25 °/s contam como zero, o que impede o ruído de acumular no ângulo com o robô parado.

**Ângulo confiável.** Se o loop atrasar mais de 100 ms ou houver 3 falhas seguidas de I2C, o ângulo perde a continuidade e deixa de ser confiável (`imuOk` falso). O bias continua válido. O ângulo volta a valer quando o yaw é zerado, o que acontece no início de todo giro e pelo botão "Zerar Yaw". Para zerar, é preciso ter calibração, leitura recente e o robô parado.

### Giro de 90° com correção

O giro de 90° termina um pouco antes do alvo (cerca de 89° na bancada). Com a opção **"Correção pelo giroscópio"** ligada no painel (parâmetro `corrigir=1` em `POST /api/giro/start`), o firmware continua depois do giro:

1. Corta os motores e espera o robô parar: no mínimo 300 ms e até a taxa de giro ficar abaixo de 1 °/s por 100 ms.
2. Mede o erro em relação aos 90° pelo yaw.
3. Se o erro for de até 1°, termina. Se passar de 15°, termina sem corrigir, porque algo deu errado no giro.
4. Caso contrário, faz um **giro curto pela IMU** do tamanho do erro, a 20 RPM, cortando o PWM quando faltam 0,4°. Depois volta ao passo 1. São no máximo 2 correções.

O modo continua `GIRO` durante todas as fases, então o heartbeat, o limite de tempo e o corte de bateria seguem valendo. O `/api/status` e o painel mostram a fase (`giroFase`), o número de correções, o erro antes da correção (`giroErroAntesCorrecao`), a última medição (`giroErroMedido`) e o erro total em relação aos 90° (`giroErroTotal`). O motivo final indica o desfecho: `giro_dentro_da_tolerancia`, `giro_corrigido_dentro_da_tolerancia`, `giro_correcoes_esgotadas`, `giro_erro_grande_demais_para_corrigir`, `giro_sem_correcao_robo_nao_parou` ou `giro_sem_correcao_imu_indisponivel`.

A correção vem **desligada por padrão**, porque os valores ainda não foram ajustados na placa.

### Arquivos

| Arquivo | Conteúdo |
|---|---|
| `imu.h` / `imu.cpp` | Identificação da IMU, calibração do bias, integração do yaw e `zerarYawIMU()` |
| `imu_logic.h` | `EstatisticaGiro` (média e desvio), `aplicarZonaMorta()`, `calibracaoValida()` e `yawPodeSerZerado()`, sem Arduino |
| `giro_logic.h` | `DetectorParado` e `decidirCorrecaoGiro()`, sem Arduino |
| `motion_control.h` / `.cpp` | `iniciarControleGiroAngulo()`: giro de ângulo qualquer por encoder ou por IMU |
| `web_telemetry.cpp` | Sequência do giro com correção (fases `PRINCIPAL`, `ASSENTANDO` e `CORRIGINDO`) |
| `config.h` | `IMU_AMOSTRAS_BIAS`, `IMU_TENTATIVAS_CALIBRACAO`, `IMU_RUIDO_MAX_DPS`, `IMU_ZONA_MORTA_DPS` e as constantes `GIRO_CORRECAO_*` |

### Testes

- `test/test_imu_logic` (11 testes): média e desvio, calibração aceita e recusada, zona morta e as condições para zerar o yaw.
- `test/test_giro_logic` (10 testes): decisão da correção e detecção de robô parado, inclusive no estouro do `millis()`.
- `tests/host/test_giro.cpp`: simulação com rodas ideais do giro de 90° por encoder e por IMU, de correções curtas e da sequência completa (giro que para em 86°, 84,5° ou 93° e é corrigido para dentro de 1°). A simulação não tem atrito nem inércia.

### Estado

- **Implementado e testado no computador.**
- **Falta a bancada:**
  - deriva menor que 1° em 60 s com o robô parado;
  - 10 giros para cada lado com a correção ligada, conferindo o ângulo com um gabarito externo (critério: erro final de até 1°);
  - ajustar as constantes `GIRO_CORRECAO_*`. Um giro de 1° corresponde a menos de 1 mm de curso na roda, então o atrito e a parada por inércia podem fazer a correção passar do ponto.

## Partida conjunta das duas rodas

Issue #152.

### O que faz

Os comandos das duas rodas já saíam juntos, mas a **ajuda de partida** era decidida por roda: quando uma roda ficava 140 ms sem pulsos, recebia sozinha um PWM mínimo (65) para vencer o atrito estático, e a ajuda acabava no primeiro pulso. Assim, uma roda saía antes da outra e o robô descompensava logo no começo do movimento.

Agora, enquanto as duas rodas comandadas não tiverem contado 3 pulsos cada (`PARTIDA_PULSOS_MIN`), o controle está na **fase de partida conjunta**:

1. **Ajuda nas duas.** Quando uma roda precisa da ajuda, a outra recebe também. A roda que já está mais de 1 mm à frente (`PARTIDA_ADIANTAMENTO_MAX_MM`) não recebe a ajuda da outra. Sem esse limite, uma roda presa fazia a solta disparar com o PWM da ajuda.
2. **Rampa segurada.** A rampa de RPM sobe só até 18 RPM (`PARTIDA_RPM_PISO`) e fica parada até as duas rodas saírem. Quando uma já saiu e a outra não, só a que saiu fica segurada: a roda presa continua com a referência subindo para ganhar torque. O piso fica acima dos 12 RPM que a ajuda exige, para a roda presa ainda pedir ajuda.
3. Quando as duas saem, a fase termina e o controle volta ao normal, com a ajuda decidida por roda (perto do alvo, uma roda que já chegou não pode receber ajuda da outra).

A fase só vale quando as **duas rodas estão comandadas**. Movimentos com uma roda parada de propósito (posição com alvo zero numa roda, RPM com um lado zero, pausa de sincronismo do giro antigo) seguem como antes. O diagnóstico de roda travada (1,2 s sem pulsos) continua valendo.

**Registro.** Em todo movimento, o firmware registra quanto tempo cada roda levou para sair e a diferença de percurso entre elas (E − D) enquanto o percurso médio estiver nos primeiros 50 mm (`PARTIDA_JANELA_MM`). O `/api/status` mostra `partidaEMs`, `partidaDMs`, `partidaAtrasoMs` (positivo quando a esquerda sai antes), `partidaDifMm` (diferença no fim da janela), `partidaDifMaxMm` (maior diferença dentro dela), `partidaJanelaCompleta` e `partidaConjunta` (fase ativa agora). Os campos também entram no CSV do painel. No giro com correção, o registro é o do giro principal, não o da correção.

### Arquivos

| Arquivo | Conteúdo |
|---|---|
| `partida_logic.h` | `rodaPartiu()`, `segurarRampaPartida()`, `ajudaPartidaConjunta()` e `RegistroPartida`, sem Arduino |
| `motion_control.cpp` | Cálculo do PI separado do acionamento (`calcularRoda()` e `aplicarRoda()`), para decidir a ajuda olhando as duas rodas |
| `web_telemetry.cpp` / `web_page.h` | Campos `partida*` no status, na aba de posição e no CSV |
| `config.h` | `PARTIDA_PULSOS_MIN`, `PARTIDA_RPM_PISO`, `PARTIDA_ADIANTAMENTO_MAX_MM` e `PARTIDA_JANELA_MM` |

### Testes

- `test/test_partida_logic` (10 testes): saída da roda, rampa segurada, ajuda conjunta com o limite de adiantamento e registro da janela de 50 mm.
- `tests/host/test_partida.cpp`: simulação com atrito estático diferente em cada roda. Confere a diferença nos primeiros 50 mm do reto, a roda travada caindo no diagnóstico (reto e RPM) e o movimento de posição com uma roda só.

A simulação **não reproduz o problema visto na bancada**: no reto, com uma roda que precisa de até 60 de PWM para sair, o firmware anterior e o novo dão a mesma diferença (menos de 0,3 mm). A comparação com o firmware anterior, roda esquerda leve e direita mais presa:

| Movimento | Direita precisa de | Antes | Agora |
|---|---|---|---|
| Reto 200 mm | 70 de PWM | 9,2 mm, chega | 5,2 mm, chega |
| Reto 200 mm | 90 de PWM | 17 mm, para por dessincronia | 10 mm, para por dessincronia |
| Giro 90° (encoder ou IMU) | 60 de PWM | para por dessincronia | 6 mm, conclui |
| Correção curta de 1,5° | qualquer | igual | igual |
| Posição, duas rodas | 70 / 90 de PWM | 7 / 21 mm | 10 / 14 mm |

Quando uma roda precisa de mais PWM que a ajuda, o problema é a ajuda fraca demais para aquela roda, e não a partida conjunta.

### Estado

- **Implementado e testado no computador.**
- **Falta a bancada:** 10 partidas no chão, conferindo `partidaDifMaxMm` (critério: até 2 mm nos primeiros 50 mm). Se uma roda precisar de mais PWM que a ajuda para sair, ajustar `PWM_AJUDA_PARTIDA` antes de mexer nas constantes `PARTIDA_*`.
