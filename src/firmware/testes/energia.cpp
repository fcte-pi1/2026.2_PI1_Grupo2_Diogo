// Firmware de bancada para os testes de energia (docs/7.2). Instruções no README.md desta pasta.
//
// Dois modos, ambos medindo com o INA219, que fica na entrada da bateria:
// - Ensaio contínuo (padrão ao ligar): envia tensão, corrente, carga (mAh) e energia (mWh)
//   pelo WiFi para o receptor_energia.py e aceita comandos (motores, perfil, corte...).
// - Consumo por componente: liga os componentes um de cada vez e mede o total;
//   o consumo de cada um é a diferença entre uma etapa e a anterior.
//
// Gravar: pio run -e teste-energia -t upload (só por USB, este firmware não tem OTA)

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <Adafruit_INA219.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_VL53L0X.h>
#include "config.h"
#include "power_logic.h"
#include "testes/energia_logic.h"

static const char *SSID_TESTE = "Xaropinho-Teste";
static const uint16_t PORTA_DADOS = 4210;      // Robô -> computador
static const uint16_t PORTA_COMANDOS = 4211;   // Computador -> robô
static const uint16_t PORTA_CARGA_WIFI = 4212; // Pacotes de carga da etapa E6

static const uint32_t TEMPO_ESTABILIZAR_MS = 2000;
static const uint32_t TEMPO_MEDIR_MS = 5000;
static const uint32_t INTERVALO_AMOSTRA_MS = 10;
static const uint32_t INTERVALO_ENVIO_MS = 100;
static const uint32_t CONTAGEM_REGRESSIVA_MS = 10000;
static const uint32_t PERFIL_PARADO_MS = 5UL * 60 * 1000;   // docs/4.2: 300 s de preparação
static const uint32_t PERFIL_MOTORES_MS = 10UL * 60 * 1000; // docs/4.2: 600 s em movimento
static const int PWM_PERFIL = 200;
static const int PWM_MEIO = 128;
static const int PWM_MAX = 255;

// Rendimentos dos reguladores adotados no docs/4.2 (seção 3.2)
static const float ETA_6V = 0.80f;   // LM2596, motores
static const float ETA_5V = 0.85f;   // MP1584, ESP32
static const float ETA_3V3 = 0.80f;  // MP1584, sensores

struct Etapa {
  const char *nome;
  const char *componente;  // O que a diferença para a etapa anterior representa
  float vTrilho;
  float rendimento;
  float esperadoMA;        // Corrente no trilho segundo o docs/4.2 (0 = sem valor de referência)
};

static const Etapa ETAPAS[] = {
  {"E0 base",                "ESP32 sem WiFi + reguladores + encoders + INA219", 0.0f, 0.0f, 0.0f},
  {"E1 + MPU6050",           "MPU6050",                3.3f, ETA_3V3, 5.0f},
  {"E2 + laser esquerdo",    "VL53L0X esquerdo",       3.3f, ETA_3V3, 20.0f},
  {"E3 + laser frontal",     "VL53L0X frontal",        3.3f, ETA_3V3, 20.0f},
  {"E4 + laser direito",     "VL53L0X direito",        3.3f, ETA_3V3, 20.0f},
  {"E5 + WiFi ligado",       "WiFi (AP sem transmitir)", 5.0f, ETA_5V, 0.0f},
  {"E6 + WiFi transmitindo", "WiFi transmitindo 10 Hz", 5.0f, ETA_5V, 0.0f},
  {"E7 + motor A 50%",       "Motor A a 50%",          6.0f, ETA_6V, 0.0f},
  {"E8 motor A 100%",        "Motor A de 50% para 100%", 6.0f, ETA_6V, 0.0f},
  {"E9 + motor B 100%",      "Motor B a 100%",         6.0f, ETA_6V, 350.0f},
};
static const int NUM_ETAPAS = sizeof(ETAPAS) / sizeof(ETAPAS[0]);

struct Resultado {
  float tensaoV;
  float correnteMA;
  float minMA;
  float maxMA;
  uint32_t amostras;
};

static Adafruit_INA219 ina219(INA219_ADDR);
static Adafruit_MPU6050 mpu;
static Adafruit_VL53L0X lasers[3];
static const int XSHUT[3] = {XSHUT_ESQ, XSHUT_FTE, XSHUT_DIR};
static const uint8_t ENDERECOS[3] = {ADDR_LASER_ESQ, ADDR_LASER_FTE, ADDR_LASER_DIR};
static bool laserAtivo[3] = {false, false, false};
static bool imuAtiva = false;
static bool cargaWifi = false;
static WiFiUDP udpDados;
static WiFiUDP udpComandos;
static bool wifiLigado = false;
static IPAddress ipComputador(192, 168, 4, 255);  // Broadcast até o computador mandar um comando
static Preferences prefs;

static float vCorte = BAT_V_CORTE;
static float iSobrecorrente = I_SOBRECORRENTE_MA;
static MonitorBateria monitor({BAT_V_ALERTA, BAT_V_CORTE, BAT_TEMPO_CONFIRMACAO_MS,
                               I_SOBRECORRENTE_MA, I_SOBRECORRENTE_TEMPO_MS});

static Resultado resultados[NUM_ETAPAS];
static int etapasConcluidas = 0;
static bool modoAutomatico = false;
static bool abortado = false;

// Estado do ensaio contínuo
static IntegradorEnergia integrador;
static uint32_t ultimaLeituraOk = 0;
static float ultimaTensao = 0.0f;
static float ultimaCorrente = 0.0f;
static float correnteMaxMA = 0.0f;
static int pwmAtual = 0;
static bool perfilAtivo = false;
static uint32_t inicioPerfil = 0;
static uint32_t numBoot = 0;
static const char *motivoReset = "";

// ---------------------------------------------------------------------------
// Atuação
// ---------------------------------------------------------------------------

static void pararMotores() {
  ledcWrite(PIN_PWMA, 0);
  ledcWrite(PIN_PWMB, 0);
  digitalWrite(PIN_STBY, LOW);
}

static void motorFrente(int pinPwm, int pin1, int pin2, int pwm) {
  digitalWrite(PIN_STBY, HIGH);
  digitalWrite(pin1, HIGH);
  digitalWrite(pin2, LOW);
  ledcWrite(pinPwm, pwm);
}

static void ligarLaser(int i) {
  digitalWrite(XSHUT[i], HIGH);
  delay(15);
  if (lasers[i].begin(ENDERECOS[i])) {
    lasers[i].startRangeContinuous();
    laserAtivo[i] = true;
  } else {
    Serial.printf("[TESTE] Laser %d nao respondeu em 0x%02X\n", i, ENDERECOS[i]);
  }
}

static void ligarWifi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SSID_TESTE);
  udpDados.begin(PORTA_DADOS + 10);
  udpComandos.begin(PORTA_COMANDOS);
  wifiLigado = true;
}

static void desligarWifi() {
  udpDados.stop();
  udpComandos.stop();
  WiFi.mode(WIFI_OFF);
  wifiLigado = false;
  cargaWifi = false;
}

static void prepararBase() {
  pinMode(PIN_STBY, OUTPUT);
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_BIN2, OUTPUT);
  static bool pwmPronto = false;
  if (!pwmPronto) {
    ledcAttach(PIN_PWMA, 10000, 8);
    ledcAttach(PIN_PWMB, 10000, 8);
    pwmPronto = true;
  }
  pararMotores();
  pwmAtual = 0;

  desligarWifi();

  for (int i = 0; i < 3; i++) {
    pinMode(XSHUT[i], OUTPUT);
    digitalWrite(XSHUT[i], LOW);  // Laser em shutdown
    laserAtivo[i] = false;
  }

  imuAtiva = false;
  if (mpu.begin(ADDR_IMU)) {
    mpu.enableSleep(true);
  } else {
    Serial.println("[TESTE] MPU6050 nao respondeu");
  }
}

static void aplicarEtapa(int k) {
  switch (k) {
    case 0: prepararBase(); break;
    case 1: mpu.enableSleep(false); imuAtiva = true; break;
    case 2: ligarLaser(0); break;
    case 3: ligarLaser(1); break;
    case 4: ligarLaser(2); break;
    case 5: ligarWifi(); break;
    case 6: cargaWifi = true; break;
    case 7: motorFrente(PIN_PWMA, PIN_AIN1, PIN_AIN2, PWM_MEIO); break;
    case 8: motorFrente(PIN_PWMA, PIN_AIN1, PIN_AIN2, PWM_MAX); break;
    case 9: motorFrente(PIN_PWMB, PIN_BIN1, PIN_BIN2, PWM_MAX); break;
  }
}

// Mantém os componentes ativos trabalhando como no uso real
static void exercitarComponentes() {
  if (imuAtiva) {
    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);
  }
  for (int i = 0; i < 3; i++) {
    if (laserAtivo[i] && lasers[i].isRangeComplete()) lasers[i].readRangeResult();
  }
  static uint32_t ultimoPacote = 0;
  if (cargaWifi && millis() - ultimoPacote >= 100) {
    ultimoPacote = millis();
    static uint8_t pacote[512];
    udpDados.beginPacket(IPAddress(192, 168, 4, 255), PORTA_CARGA_WIFI);
    udpDados.write(pacote, sizeof(pacote));
    udpDados.endPacket();
  }
}

// ---------------------------------------------------------------------------
// Saída: serial e, com o WiFi ligado, também o computador
// ---------------------------------------------------------------------------

static void enviarLinha(char tipo, const char *texto) {
  if (!wifiLigado) return;
  udpDados.beginPacket(ipComputador, PORTA_DADOS);
  udpDados.printf("%c,%s", tipo, texto);
  udpDados.endPacket();
}

static void mensagem(const char *formato, ...) {
  char texto[160];
  va_list args;
  va_start(args, formato);
  vsnprintf(texto, sizeof(texto), formato, args);
  va_end(args);
  Serial.printf("[TESTE] %s\n", texto);
  enviarLinha('M', texto);
}

// Print que manda cada linha do relatório para a serial e para o computador
class SaidaRelatorio : public Print {
public:
  size_t write(uint8_t c) override {
    Serial.write(c);
    if (c == '\n' || n == sizeof(linha) - 1) {
      linha[n] = '\0';
      enviarLinha('R', linha);
      n = 0;
    } else {
      linha[n++] = c;
    }
    return 1;
  }

private:
  char linha[200];
  size_t n = 0;
};

// ---------------------------------------------------------------------------
// Medição
// ---------------------------------------------------------------------------

static bool lerINA(float &tensaoV, float &correnteMA) {
  float vBus = ina219.getBusVoltage_V();
  bool ok = ina219.success();
  float vShunt = ina219.getShuntVoltage_mV();
  ok = ok && ina219.success();
  float i = ina219.getCurrent_mA();
  ok = ok && ina219.success();
  if (!ok) return false;
  tensaoV = vBus + vShunt / 1000.0f;
  correnteMA = i;
  return true;
}

// Retorna false se o corte da bateria foi acionado
static bool aguardar(uint32_t duracaoMs, EstatisticaEnergia *estatistica) {
  uint32_t inicio = millis();
  uint32_t ultimoPrint = inicio;
  EstatisticaEnergia janela;
  while (millis() - inicio < duracaoMs) {
    exercitarComponentes();
    float v, i;
    if (lerINA(v, i)) {
      monitor.atualizar(v, i, millis());
      if (estatistica) estatistica->adicionar(v, i);
      janela.adicionar(v, i);
    }
    if (monitor.estado() == BATERIA_CRITICA) {
      pararMotores();
      Serial.printf("[TESTE] Bateria abaixo de %.1f V, teste interrompido\n", vCorte);
      return false;
    }
    if (!modoAutomatico && millis() - ultimoPrint >= 1000) {
      ultimoPrint = millis();
      Serial.printf("   %.2f V | %7.1f mA (min %.1f, max %.1f)\n",
                    janela.tensaoMedia(), janela.correnteMedia(), janela.minI, janela.maxI);
      janela = EstatisticaEnergia();
    }
    delay(INTERVALO_AMOSTRA_MS);
  }
  return true;
}

static bool medirEtapa(int k) {
  Serial.printf("[TESTE] %s\n", ETAPAS[k].nome);
  aplicarEtapa(k);
  if (!aguardar(TEMPO_ESTABILIZAR_MS, nullptr)) return false;

  EstatisticaEnergia e;
  if (!aguardar(TEMPO_MEDIR_MS, &e)) return false;

  resultados[k] = {e.tensaoMedia(), e.correnteMedia(), e.minI, e.maxI, e.n};
  etapasConcluidas = k + 1;
  Serial.printf("[TESTE] %s: %.2f V | %.1f mA (%lu amostras)\n", ETAPAS[k].nome,
                resultados[k].tensaoV, resultados[k].correnteMA, (unsigned long)e.n);
  return true;
}

// ---------------------------------------------------------------------------
// Relatório do consumo por componente (fica salvo na flash)
// ---------------------------------------------------------------------------

static void salvarResultados() {
  prefs.putBytes("res", resultados, sizeof(resultados));
  prefs.putInt("n", etapasConcluidas);
  prefs.putBool("auto", modoAutomatico);
  prefs.putBool("abort", abortado);
}

static void imprimirRelatorio(Print &out) {
  Resultado r[NUM_ETAPAS];
  int n = prefs.getInt("n", 0);
  if (n == 0 || prefs.getBytes("res", r, sizeof(r)) != sizeof(r)) {
    out.println("Nenhum resultado salvo");
    return;
  }
  bool automatico = prefs.getBool("auto", false);
  out.printf("\n## Consumo por componente (%s%s)\n\n",
             automatico ? "modo automatico, sem USB" : "modo manual, USB ligado",
             prefs.getBool("abort", false) ? ", INTERROMPIDO pelo corte da bateria" : "");
  out.println("| Etapa | Bateria (V) | Corrente media (mA) | Min (mA) | Max (mA) |");
  out.println("|---|:---:|:---:|:---:|:---:|");
  for (int k = 0; k < n; k++) {
    out.printf("| %s | %.2f | %.1f | %.1f | %.1f |\n",
               ETAPAS[k].nome, r[k].tensaoV, r[k].correnteMA, r[k].minMA, r[k].maxMA);
  }

  out.println("\n| Componente | Na bateria (mA) | No trilho (mA) | Trilho | Esperado docs/4.2 (mA) |");
  out.println("|---|:---:|:---:|:---:|:---:|");
  out.printf("| %s | %.1f | - | - | - |\n", ETAPAS[0].componente, r[0].correnteMA);
  for (int k = 1; k < n; k++) {
    float delta = r[k].correnteMA - r[k - 1].correnteMA;
    float trilho = correnteNoTrilho(delta, r[k].tensaoV, ETAPAS[k].vTrilho, ETAPAS[k].rendimento);
    char esperado[12] = "-";
    if (ETAPAS[k].esperadoMA > 0.0f) snprintf(esperado, sizeof(esperado), "%.0f", ETAPAS[k].esperadoMA);
    out.printf("| %s | %.1f | %.1f | %.1f V | %s |\n",
               ETAPAS[k].componente, delta, trilho, ETAPAS[k].vTrilho, esperado);
  }
  if (n > 8) {
    float motorA = r[8].correnteMA - r[6].correnteMA;
    out.printf("| Motor A a 100%% (E8 - E6) | %.1f | %.1f | 6.0 V | 350 |\n", motorA,
               correnteNoTrilho(motorA, r[8].tensaoV, 6.0f, ETA_6V));
  }
  out.printf("| **Total (ultima etapa)** | %.1f | - | - | - |\n\n", r[n - 1].correnteMA);
  out.println("Obs.: motores medidos sem carga (rodas no ar); os 350 mA do docs/4.2 sao com carga.");
  if (!automatico) {
    out.println("Obs.: com o USB ligado, parte do ESP32 e alimentada pelo computador.");
  }
}

// ---------------------------------------------------------------------------
// Ensaio contínuo pelo WiFi
// ---------------------------------------------------------------------------

static const char *nomeMotivoReset(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON: return "ligado";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "erro";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_BROWNOUT: return "BROWNOUT";
    case ESP_RST_EXT: return "externo";
    default: return "outro";
  }
}

static const char *nomeEstado(EstadoBateria e) {
  switch (e) {
    case BATERIA_BAIXA: return "baixa";
    case BATERIA_CRITICA: return "CRITICA";
    default: return "normal";
  }
}

static void ligarMotores(int pwm) {
  pwm = constrain(pwm, 0, 255);
  if (pwm == 0 || monitor.estado() == BATERIA_CRITICA) {
    pararMotores();
    pwmAtual = 0;
    return;
  }
  motorFrente(PIN_PWMA, PIN_AIN1, PIN_AIN2, pwm);
  motorFrente(PIN_PWMB, PIN_BIN1, PIN_BIN2, pwm);
  pwmAtual = pwm;
}

// Recria o monitor com os limites atuais (e zera o estado, para repetir o teste de corte)
static void aplicarLimites() {
  float vAlerta = BAT_V_ALERTA > vCorte ? BAT_V_ALERTA : vCorte;
  monitor = MonitorBateria({vAlerta, vCorte, BAT_TEMPO_CONFIRMACAO_MS,
                            iSobrecorrente, I_SOBRECORRENTE_TEMPO_MS});
}

static void zerarEnsaio() {
  integrador = IntegradorEnergia();
  correnteMaxMA = 0.0f;
  ultimaLeituraOk = millis();  // Não conta o tempo em que o ensaio não estava rodando
}

// Robô como no uso real: IMU e lasers ligados, WiFi transmitindo
static void iniciarEnsaio() {
  prepararBase();
  for (int k = 1; k <= 4; k++) aplicarEtapa(k);
  ligarWifi();
  zerarEnsaio();
  Serial.printf("[TESTE] Ensaio continuo: conecte o computador na rede %s\n", SSID_TESTE);
}

static void enviarAmostra() {
  char linha[200];
  snprintf(linha, sizeof(linha), "%lu,%s,%lu,%.3f,%.1f,%.0f,%.2f,%.1f,%.1f,%s,%d,%d,%d",
           (unsigned long)numBoot, motivoReset, (unsigned long)millis(), ultimaTensao, ultimaCorrente,
           ultimaTensao * ultimaCorrente, integrador.mAh, integrador.mWh, correnteMaxMA,
           nomeEstado(monitor.estado()), monitor.sobrecorrente() ? 1 : 0, pwmAtual, perfilAtivo ? 1 : 0);
  enviarLinha('E', linha);
}

static void atualizarEnsaio() {
  static uint32_t ultimaAmostra = millis();
  static uint32_t ultimoEnvio = 0;
  static uint32_t ultimoPrint = 0;
  static EstadoBateria estadoAvisado = BATERIA_NORMAL;
  static bool sobrecorrenteAvisada = false;

  uint32_t agora = millis();
  if (agora - ultimaAmostra < INTERVALO_AMOSTRA_MS) return;
  ultimaAmostra = agora;

  exercitarComponentes();
  float v, i;
  if (lerINA(v, i)) {
    integrador.adicionar(v, i, agora - ultimaLeituraOk);
    ultimaLeituraOk = agora;
    ultimaTensao = v;
    ultimaCorrente = i;
    if (i > correnteMaxMA) correnteMaxMA = i;
    monitor.atualizar(v, i, agora);
  }

  if (monitor.estado() != estadoAvisado) {
    estadoAvisado = monitor.estado();
    if (estadoAvisado == BATERIA_BAIXA) mensagem("Bateria baixa: %.2f V", ultimaTensao);
    if (estadoAvisado == BATERIA_CRITICA) mensagem("Bateria critica: %.2f V, motores cortados", ultimaTensao);
  }
  if (monitor.sobrecorrente() != sobrecorrenteAvisada) {
    sobrecorrenteAvisada = monitor.sobrecorrente();
    if (sobrecorrenteAvisada) mensagem("Sobrecorrente: %.0f mA", ultimaCorrente);
  }
  if (monitor.estado() == BATERIA_CRITICA) {
    if (pwmAtual > 0) ligarMotores(0);
    perfilAtivo = false;
  }

  if (perfilAtivo) {
    uint32_t t = agora - inicioPerfil;
    if (t >= PERFIL_PARADO_MS + PERFIL_MOTORES_MS) {
      ligarMotores(0);
      perfilAtivo = false;
      mensagem("Perfil concluido: %.1f mAh, %.1f mWh", integrador.mAh, integrador.mWh);
    } else if (t >= PERFIL_PARADO_MS && pwmAtual == 0) {
      ligarMotores(PWM_PERFIL);
      mensagem("Perfil: motores ligados (PWM %d) por 10 min", PWM_PERFIL);
    }
  }

  if (agora - ultimoEnvio >= INTERVALO_ENVIO_MS) {
    ultimoEnvio = agora;
    enviarAmostra();
  }
  if (agora - ultimoPrint >= 1000) {
    ultimoPrint = agora;
    Serial.printf("%.2f V | %6.1f mA | %7.1f mAh | %7.1f mWh | PWM %3d | %s\n", ultimaTensao,
                  ultimaCorrente, integrador.mAh, integrador.mWh, pwmAtual, nomeEstado(monitor.estado()));
  }
}

// ---------------------------------------------------------------------------
// Comandos (pela serial ou pelo receptor no computador)
// ---------------------------------------------------------------------------

static void rodarSequencia();

static void executarComando(String cmd, bool pelaSerial) {
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.length() == 0 || cmd == "ola") return;

  if (cmd.startsWith("motores")) {
    perfilAtivo = false;
    ligarMotores(cmd.substring(7).toInt());
    mensagem("Motores em PWM %d", pwmAtual);
  } else if (cmd == "parar") {
    perfilAtivo = false;
    ligarMotores(0);
    mensagem("Motores parados");
  } else if (cmd == "perfil") {
    zerarEnsaio();
    ligarMotores(0);
    perfilAtivo = true;
    inicioPerfil = millis();
    mensagem("Perfil de 15 min iniciado: 5 min parado e 10 min com motores (rodas no ar!)");
  } else if (cmd == "zerar") {
    zerarEnsaio();
    mensagem("Contadores de carga e energia zerados");
  } else if (cmd.startsWith("corte")) {
    float v = cmd.substring(5).toFloat();
    vCorte = v > 0.0f ? v : BAT_V_CORTE;
    aplicarLimites();
    mensagem("Corte em %.2f V (padrao %.1f V)", vCorte, BAT_V_CORTE);
  } else if (cmd.startsWith("sobrecorrente")) {
    float i = cmd.substring(13).toFloat();
    iSobrecorrente = i > 0.0f ? i : I_SOBRECORRENTE_MA;
    aplicarLimites();
    mensagem("Aviso de sobrecorrente em %.0f mA (padrao %.0f mA)", iSobrecorrente, I_SOBRECORRENTE_MA);
  } else if (cmd == "armar" || cmd == "a") {
    prefs.putBool("armado", true);
    mensagem("Consumo por componente armado: desligue, tire o USB, rodas no ar e ligue pela bateria");
  } else if (cmd == "relatorio" || cmd == "r") {
    SaidaRelatorio saida;
    imprimirRelatorio(saida);
  } else if ((cmd == "manual" || cmd == "m") && pelaSerial) {
    rodarSequencia();
    iniciarEnsaio();
  } else {
    mensagem("Comando desconhecido: %s", cmd.c_str());
  }
}

static void lerComandos() {
  static String linhaSerial;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      executarComando(linhaSerial, true);
      linhaSerial = "";
    } else {
      linhaSerial += c;
    }
  }

  if (!wifiLigado) return;
  int tamanho = udpComandos.parsePacket();
  if (tamanho > 0) {
    ipComputador = udpComandos.remoteIP();  // Passa a responder direto para quem mandou
    char buf[64];
    int n = udpComandos.read(buf, sizeof(buf) - 1);
    buf[n > 0 ? n : 0] = '\0';
    executarComando(String(buf), false);
  }
}

// ---------------------------------------------------------------------------
// Consumo por componente
// ---------------------------------------------------------------------------

static void rodarSequencia() {
  etapasConcluidas = 0;
  abortado = false;
  aplicarLimites();
  for (int k = 0; k < NUM_ETAPAS; k++) {
    if (!modoAutomatico && k > 0) {
      Serial.println("[TESTE] Enter para a proxima etapa, 'x' + Enter para parar");
      while (!Serial.available()) aguardar(100, nullptr);
      char c = Serial.read();
      delay(20);
      while (Serial.available()) Serial.read();
      if (c == 'x' || c == 'X') break;
    }
    if (!medirEtapa(k)) {
      abortado = true;
      break;
    }
  }
  pararMotores();
  salvarResultados();
  Serial.println("[TESTE] Fim do consumo por componente, resultados salvos ('relatorio' para ver)");
}

static void imprimirMenu() {
  Serial.println("\n=== Testes de energia (docs/7.2) ===");
  Serial.println("Comandos (serial ou receptor_energia.py):");
  Serial.println("  motores <pwm> | parar | perfil | zerar");
  Serial.println("  corte <V> | sobrecorrente <mA>   (sem valor volta ao padrao)");
  Serial.println("  armar | relatorio | manual (so pela serial)");
  Serial.println("ATENCAO: motores so giram com as rodas no ar.");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Wire.setClock(400000);
  prefs.begin("teste-energia", false);
  numBoot = prefs.getUInt("boots", 0) + 1;
  prefs.putUInt("boots", numBoot);
  motivoReset = nomeMotivoReset(esp_reset_reason());

  prepararBase();
  if (!ina219.begin(&Wire)) {
    Serial.println("[TESTE] INA219 nao encontrado, nada a medir");
    return;
  }
  ina219.setCalibration_32V_2A();

  if (prefs.getBool("armado", false)) {
    prefs.putBool("armado", false);  // Roda uma vez só
    modoAutomatico = true;
    Serial.println("[TESTE] Consumo por componente em 10 s, rodas no ar!");
    delay(CONTAGEM_REGRESSIVA_MS);
    rodarSequencia();
    modoAutomatico = false;
  }

  iniciarEnsaio();
  imprimirMenu();
}

void loop() {
  lerComandos();
  atualizarEnsaio();
}
