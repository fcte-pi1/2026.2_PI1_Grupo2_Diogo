#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include "lasers.h"
#include "config.h"

static Adafruit_VL53L0X loxEsq = Adafruit_VL53L0X();
static Adafruit_VL53L0X loxFte = Adafruit_VL53L0X();
static Adafruit_VL53L0X loxDir = Adafruit_VL53L0X();

static bool laserEsqAtivo = false;
static bool laserFteAtivo = false;
static bool laserDirAtivo = false;

static void configLongRange(Adafruit_VL53L0X &sensor) {
  sensor.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_LONG_RANGE);
  sensor.setMeasurementTimingBudgetMicroSeconds(50000);
}

static bool iniciarLaser(Adafruit_VL53L0X &sensor, uint8_t pinoXshut,
                         uint8_t endereco, const char *nome) {
  digitalWrite(pinoXshut, HIGH);
  delay(15);
  if (sensor.begin(endereco)) {
    configLongRange(sensor);
    Serial.printf("[LASER] %s inicializado em 0x%02X\n", nome, endereco);
    return true;
  }
  digitalWrite(pinoXshut, LOW);
  Serial.printf("[LASER] %s FALHOU (begin em 0x%02X); XSHUT desligado\n", nome, endereco);
  return false;
}

void initLasers() {
  pinMode(XSHUT_ESQ, OUTPUT);
  pinMode(XSHUT_FTE, OUTPUT);
  pinMode(XSHUT_DIR, OUTPUT);

  digitalWrite(XSHUT_ESQ, LOW);
  digitalWrite(XSHUT_FTE, LOW);
  digitalWrite(XSHUT_DIR, LOW);
  delay(50);

  laserEsqAtivo = iniciarLaser(loxEsq, XSHUT_ESQ, ADDR_LASER_ESQ, "U2 (Esq)");
  laserFteAtivo = iniciarLaser(loxFte, XSHUT_FTE, ADDR_LASER_FTE, "U3 (Fte)");
  laserDirAtivo = iniciarLaser(loxDir, XSHUT_DIR, ADDR_LASER_DIR, "U4 (Dir)");
}

int ler_distancia_mm(SensorLaser id) {
  Adafruit_VL53L0X *sensor = nullptr;
  bool ativo = false;
  switch (id) {
    case LASER_ESQ: sensor = &loxEsq; ativo = laserEsqAtivo; break;
    case LASER_FTE: sensor = &loxFte; ativo = laserFteAtivo; break;
    case LASER_DIR: sensor = &loxDir; ativo = laserDirAtivo; break;
  }
  if (sensor == nullptr || !ativo) return LASER_SEM_LEITURA;  // não toca no I2C

  VL53L0X_RangingMeasurementData_t m = {};
  VL53L0X_Error status = sensor->rangingTest(&m, false);
  return classificarLeitura(status != VL53L0X_ERROR_NONE, m.RangeStatus,
                            m.RangeMilliMeter, LASER_DIST_MIN_MM, LASER_DIST_MAX_MM);
}

static int paraLegado(int leitura) {
  return leitura >= 0 ? leitura : -1;
}

void readLasers(int &distEsq, int &distFte, int &distDir) {
  distEsq = paraLegado(ler_distancia_mm(LASER_ESQ));
  distFte = paraLegado(ler_distancia_mm(LASER_FTE));
  distDir = paraLegado(ler_distancia_mm(LASER_DIR));
}