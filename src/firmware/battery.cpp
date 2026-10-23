#include "battery.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <math.h>

static Adafruit_INA219 ina219(0x40);
static bool presente = false;
static float tensao = NAN;
static float corrente = NAN;
static uint32_t ultimaLeitura = 0;

void initBattery() {
    presente = ina219.begin();
    if (!presente) {
        Serial.println("INA219 ausente: telemetria de energia indisponivel.");
        return;
    }
    ina219.setCalibration_32V_2A();
    Serial.println("INA219 conectado (faixa padrao calibrada para 2 A).");
    ultimaLeitura = millis() - AMOSTRA_BATERIA_MS;
    updateBattery();
}

void updateBattery() {
    if (!presente || millis() - ultimaLeitura < AMOSTRA_BATERIA_MS) return;
    ultimaLeitura = millis();
    tensao = ina219.getBusVoltage_V();
    corrente = ina219.getCurrent_mA();
}

bool batteryAvailable() { return presente; }
float getVoltage() { return tensao; }
float getCurrent() { return corrente; }
