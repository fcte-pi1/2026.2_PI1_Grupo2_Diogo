#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include "lasers.h"
#include "config.h"

// Estado de cada sensor, indexado por SensorLaser
struct Laser {
  Adafruit_VL53L0X sensor;
  bool ativo = false;                      // begin() e início da medição deram certo
  int ultimaLeitura = LASER_SEM_LEITURA;   // última classificação (mm ou código)
  uint32_t ultimaAtualizacaoMs = 0;        // quando chegou o último dado novo
};

static Laser lasers[3];

static const uint8_t PINOS_XSHUT[3]  = {XSHUT_ESQ, XSHUT_FTE, XSHUT_DIR};
static const uint8_t ENDERECOS[3]    = {ADDR_LASER_ESQ, ADDR_LASER_FTE, ADDR_LASER_DIR};
static const char *const NOMES[3]    = {"U2 (Esq)", "U3 (Fte)", "U4 (Dir)"};

static void configLongRange(Adafruit_VL53L0X &sensor) {
  sensor.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE);
  sensor.setMeasurementTimingBudgetMicroSeconds(LASER_ORCAMENTO_US);
}

// true se o sensor respondeu no endereço novo e começou a medir sozinho.
// Sensor que falha volta para XSHUT LOW: senão ele fica ligado no 0x29 de
// fábrica e colide com o próximo sensor liberado.
static bool iniciarLaser(SensorLaser id) {
  Laser &l = lasers[id];

  digitalWrite(PINOS_XSHUT[id], HIGH);
  delay(15);
  if (l.sensor.begin(ENDERECOS[id])) {
    // Ordem importa: o modo e o orçamento são configurados antes de medir
    configLongRange(l.sensor);
    if (l.sensor.startRangeContinuous(LASER_PERIODO_MEDICAO_MS)) {
      Serial.printf("[LASER] %s inicializado em 0x%02X (medição contínua)\n",
                    NOMES[id], ENDERECOS[id]);
      return true;
    }
  }

  digitalWrite(PINOS_XSHUT[id], LOW);
  Serial.printf("[LASER] %s FALHOU (begin/início em 0x%02X); XSHUT desligado\n",
                NOMES[id], ENDERECOS[id]);
  return false;
}

void initLasers() {
  for (int i = 0; i < 3; i++) {
    pinMode(PINOS_XSHUT[i], OUTPUT);
    digitalWrite(PINOS_XSHUT[i], LOW);
  }
  delay(50);

  // A ordem importa: um por vez, cada um já sai do 0x29 antes do próximo ligar.
  for (int i = 0; i < 3; i++) {
    lasers[i].ativo = iniciarLaser((SensorLaser)i);
    lasers[i].ultimaAtualizacaoMs = millis();
  }
}

// Consulta se há dado novo. Sem dado novo, devolve a
// última classificação; se o sensor ficar mudo além da validade, é timeout.
int ler_distancia_mm(SensorLaser id) {
  if (id < LASER_ESQ || id > LASER_DIR) return LASER_SEM_LEITURA;
  Laser &l = lasers[id];
  if (!l.ativo) return LASER_SEM_LEITURA;  // não toca no I2C

  uint32_t agora = millis();
  if (l.sensor.isRangeComplete()) {
    uint16_t mm = l.sensor.readRangeResult();
    bool erro = l.sensor.Status != VL53L0X_ERROR_NONE;
    l.ultimaLeitura = classificarLeitura(erro, l.sensor.readRangeStatus(), mm,
                                         LASER_DIST_MIN_MM, LASER_DIST_MAX_MM);
    l.ultimaAtualizacaoMs = agora;
  } else if (leituraVencida(agora, l.ultimaAtualizacaoMs, LASER_VALIDADE_MS)) {
    l.ultimaLeitura = LASER_TIMEOUT;
  }
  return l.ultimaLeitura;
}

static int paraLegado(int leitura) {
  return leitura >= 0 ? leitura : -1;
}

void readLasers(int &distEsq, int &distFte, int &distDir) {
  distEsq = paraLegado(ler_distancia_mm(LASER_ESQ));
  distFte = paraLegado(ler_distancia_mm(LASER_FTE));
  distDir = paraLegado(ler_distancia_mm(LASER_DIR));
}
