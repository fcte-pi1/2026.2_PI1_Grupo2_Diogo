# Testes de energia (docs/7.2)

Firmware de bancada e roteiro para os testes de energia. O objetivo é conferir se as demandas energéticas previstas no [docs/4.2](../../../docs/4.2%20-%20Projeto%20conceitual%20de%20energia.md) foram atendidas e preencher a tabela do [docs/7.2](../../../docs/7.2%20-%20Testes%20de%20energia.md).

Quase tudo é medido pelo INA219, que fica na entrada da bateria e mede a tensão e a corrente de todo o robô. O multímetro só é indispensável para a saída dos reguladores (teste 2.4).

## Arquivos

| Arquivo | O que é |
|---|---|
| `energia.cpp` | Firmware de teste. Não é o firmware do robô. |
| `energia_logic.h` | Contas do teste (média, carga, energia e conversão entre trilhos). Testes em `test/test_energia_logic`. |
| `receptor_energia.py` | Script que roda no computador: recebe os dados pelo WiFi, salva em CSV e manda comandos ao robô. Só usa a biblioteca padrão do Python. |

## Preparação

1. Bateria carregada (acima de 7,8 V) e robô **apoiado com as rodas no ar**. Os motores giram em vários testes.
2. Gravar o firmware de teste **pelo USB** (ele não tem OTA):
   ```
   pio run -e teste-energia -t upload
   ```
3. Conectar o computador na rede WiFi **Xaropinho-Teste** (aberta, sem senha) e rodar:
   ```
   python3 src/firmware/testes/receptor_energia.py
   ```
   O receptor mostra a tensão, a corrente, a potência, a carga e a energia a cada segundo. O que for digitado nele vai para o robô como comando. **Ctrl+C** encerra e mostra o resumo. As amostras ficam em `ensaio-*.csv` e o relatório em `relatorio-energia.md`, na pasta onde o script rodou. Se nenhum dado chegar, verifique o firewall (`sudo ufw allow 4210/udp`).
4. Ao terminar, gravar de novo o firmware do robô: `pio run -e esp32dev -t upload`. Para gravar pelo USB, comente as linhas `espota` do `platformio.ini`.

**Durante o ensaio, o cabo USB fica desligado.** Com ele ligado, parte do ESP32 é alimentada pelo computador e a corrente medida fica menor que a real. Os dados chegam pelo WiFi.

**Nunca descarregue a LiPo para testar o corte.** O teste 2.7 sobe o limite por comando, sem gastar a bateria.

## Comandos

| Comando | O que faz |
|---|---|
| `motores <pwm>` | Liga os dois motores para frente (PWM de 0 a 255) |
| `parar` | Para os motores |
| `perfil` | Zera os contadores e roda o lançamento do docs/4.2: 5 min parado e 10 min com motores em PWM 200 |
| `zerar` | Zera a carga (mAh) e a energia (mWh) acumuladas |
| `corte <V>` | Muda o limite de corte por subtensão. `corte` sem valor volta para 6,6 V |
| `sobrecorrente <mA>` | Muda o limite do aviso de sobrecorrente. Sem valor volta para 2000 mA |
| `armar` | Arma o consumo por componente para o próximo boot |
| `relatorio` | Mostra a última tabela do consumo por componente |
| `manual` | Consumo por componente avançando pelo teclado (só pela serial, com USB) |

Ao ligar, o robô entra no ensaio contínuo, como no uso real: IMU e lasers ligados e WiFi transmitindo a 10 Hz. Os motores só giram por comando.

## Consumo por componente

O firmware liga os componentes um de cada vez (E0 a E9: base, MPU6050, cada laser, WiFi, WiFi transmitindo e cada motor). Cada etapa estabiliza por 2 s e mede por 5 s. **O consumo de cada componente é a diferença entre uma etapa e a anterior.** O relatório mostra o valor puxado da bateria e o valor no trilho do componente (3,3, 5 ou 6 V, convertido pelo rendimento dos reguladores do docs/4.2), ao lado do esperado.

1. No receptor (ou na serial), mande `armar`.
2. Desligue o robô, confira que o USB está desconectado e ligue de novo pela bateria, com as rodas no ar.
3. Em 10 s o robô roda as etapas sozinho (cerca de 1,5 min). O WiFi fica desligado na maior parte do tempo, então o receptor fica sem dados. Isso é esperado.
4. Quando o receptor voltar a mostrar dados, mande `relatorio`. A tabela sai pronta para colar no docs/7.2 e também fica em `relatorio-energia.md`.

O software não consegue desligar os encoders nem o LED do MPU6050. Esses entram na etapa base. Para separá-los, tire os jumpers deles na protoboard e compare a corrente da base.

## Roteiro por teste

Valores esperados do docs/4.2. As correntes dependem da tensão da bateria (seção 6.1): anote a tensão junto.

| ID | Teste | Como fazer | Esperado | Precisa de |
|:---:|---|---|---|---|
| 2.1 | Tensão medida pelo INA219 | Comparar a tensão do receptor com uma referência: multímetro nos bornes da bateria ou o visor do carregador IMAX B6 | Diferença ≤ 0,05 V | Multímetro ou IMAX B6 |
| 2.2 | Consumo por componente e corrente parado | Consumo por componente (acima). A corrente parado é a etapa E6 | Total parado: ~162 mA a 7,4 V (142 mA a 8,4 V). Por componente, ver a tabela do relatório | INA219 |
| 2.3 | Corrente com os motores girando | `motores 200` e anotar a corrente estável | ~866 mA a 7,4 V **com carga**. Com as rodas no ar sai menor, registrar em Observações | INA219 |
| 2.4 | Saída dos reguladores | Multímetro em V na saída e GND de cada regulador, com os motores parados e com `motores 255` | 6 V: 5,70 a 6,30 · 5 V: 4,75 a 5,25 · 3,3 V: 3,14 a 3,47 | **Multímetro** |
| 2.5 | ESP32 não reinicia na partida dos motores | Alternar `motores 255` e `parar` umas 10 vezes | Nenhum "ESP32 REINICIOU" no receptor (um brownout aparece com o motivo) | INA219 |
| 2.6 | Autonomia de um lançamento | `perfil` e esperar 15 min. Ctrl+C no fim e anotar o resumo | ~1,18 Wh (~160 mAh) por lançamento, sem corte nem reinício. Com as rodas no ar sai menor | INA219 |
| 2.7 | Corte por subtensão | Com `motores 150` ligado, mandar `corte 8.0` (acima da tensão atual). Depois `corte` para voltar ao padrão | "Bateria critica" depois de 2 s, motores param e não religam com `motores` | INA219 |
| 2.8 | Queda rápida não corta | Com o limite padrão, partidas bruscas (`motores 255` / `parar`) | Estado continua "normal" | INA219 |
| 2.9 | Aviso de sobrecorrente | Ver a corrente com `motores 255` e mandar `sobrecorrente` com um valor abaixo dela (ex.: metade). Depois `sobrecorrente` para voltar | "Sobrecorrente" depois de ~500 ms | INA219 |

Para cada teste, registrar na tabela do docs/7.2 a data, o responsável, o esperado, o obtido e a aprovação interna, e tirar uma foto ou print (receptor, multímetro, montagem). Um teste que não puder ser feito continua na tabela, com o motivo em Observações.
