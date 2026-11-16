#include "i2c_diagnostics.h"
#include "config.h"
#include "imu_chip_id.h"
#include <Wire.h>

namespace {
uint8_t obterWho(uint8_t endereco, int& erro) {
    Wire.beginTransmission(endereco);
    Wire.write((uint8_t)0x75);  // WHO_AM_I
    erro = Wire.endTransmission(false);
    if (erro != 0) return 0;
    if (Wire.requestFrom((int)endereco, 1) != 1) { erro = -1; return 0; }
    return (uint8_t)Wire.read();
}
String hex2(uint8_t valor) {
    char v[7];
    snprintf(v, sizeof(v), "0x%02X", valor);
    return String(v);
}
}

String diagnosticoI2CJson() {
    // Executar sob demanda, com motores parados. Nao reinicializa o Wire.
    String r = "{\"sda\":" + String(PIN_SDA) + ",\"scl\":" + String(PIN_SCL);
    r += ",\"frequenciaHz\":100000,\"enderecos\":[";
    bool primeiro = true;
    for (uint8_t addr = 0x08; addr <= 0x77; ++addr) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            if (!primeiro) r += ',';
            r += "\"" + hex2(addr) + "\"";
            primeiro = false;
        }
    }
    r += "],\"mpu\":[";
    const uint8_t candidatas[2] = {0x68, 0x69};
    for (uint8_t i = 0; i < 2; ++i) {
        uint8_t addr = candidatas[i];
        Wire.beginTransmission(addr);
        int ack = Wire.endTransmission();
        if (i) r += ',';
        r += "{\"endereco\":\"" + hex2(addr) + "\",\"ack\":";
        r += (ack == 0) ? "true" : "false";
        if (ack == 0) {
            int erro = 0;
            uint8_t who = obterWho(addr, erro);
            r += ",\"who\":";
            r += (erro == 0) ? ("\"" + hex2(who) + "\"") : String("null");
            r += ",\"erroWire\":" + String(erro);
            r += ",\"identidadeValida\":";
            r += ((erro == 0 && imuIdentidadeSuportada(who)) ? "true" : "false");
            r += ",\"modelo\":\"" + String((erro == 0) ? imuModelo(who) : "DESCONHECIDO") + "\"";
        }
        r += '}';
    }
    r += "]}";
    return r;
}

void imprimirDiagnosticoI2C() {
    Serial.println("\n=== I2C SCAN: GPIO21 SDA / GPIO22 SCL; 100 kHz ===");
    Serial.println(diagnosticoI2CJson().c_str());
    Serial.println("Esperado: INA219=0x40; IMU I2C=0x68 ou 0x69; WHO_AM_I=0x68 (6050) ou 0x74 (6515).");
    Serial.println("V4.4: lasers ainda em XSHUT no scan inicial; initLasers ocorrera em seguida.");
}
