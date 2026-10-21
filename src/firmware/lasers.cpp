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

  // A ordem importa: um por vez, cada um já sai do 0x29 antes do próximo ligar.
  laserEsqAtivo = iniciarLaser(loxEsq, XSHUT_ESQ, ADDR_LASER_ESQ, "U2 (Esq)");
  laserFteAtivo = iniciarLaser(loxFte, XSHUT_FTE, ADDR_LASER_FTE, "U3 (Fte)");
  laserDirAtivo = iniciarLaser(loxDir, XSHUT_DIR, ADDR_LASER_DIR, "U4 (Dir)");
}

static int extrairDistancia(Adafruit_VL53L0X &sensor, bool ativo) {
  if (!ativo) return -1;  // nunca inicializou: não toca no I2C
  VL53L0X_RangingMeasurementData_t m;
  sensor.rangingTest(&m, false);
  if (m.RangeStatus == 0 && m.RangeMilliMeter < 2000 && m.RangeMilliMeter > 20) {
    return m.RangeMilliMeter;
  }
  return -1;
}

void readLasers(int &distEsq, int &distFte, int &distDir) {
  distEsq = extrairDistancia(loxEsq, laserEsqAtivo);
  distFte = extrairDistancia(loxFte, laserFteAtivo);
  distDir = extrairDistancia(loxDir, laserDirAtivo);
}