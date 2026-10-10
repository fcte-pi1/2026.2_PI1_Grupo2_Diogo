#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "motors.h"
#include "imu.h"
#include "lasers.h"
#include "ota_manager.h"

unsigned long tempoAnterior = 0;
const unsigned long INTERVALO_MS = 100;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Wire.setClock(400000);

  initMotors();
  initIMU();
  initLasers();
  initOTA();

  Serial.println("\n--- Robô Inicializado com Sucesso ---");
  tempoAnterior = millis();
}

void loop() {
  handleOTA(); // Escuta requisições de gravação sem fio

  unsigned long agora = millis();
  unsigned long delta = agora - tempoAnterior;

  if (delta >= INTERVALO_MS) {
    float dt = delta / 1000.0f;
    tempoAnterior = agora;

    // Atualiza odometria e sensores
    updateIMU(dt);

    float rpsA = 0, rpsB = 0;
    getEncoderSpeeds(dt, rpsA, rpsB);

    int dEsq = -1, dFte = -1, dDir = -1;
    readLasers(dEsq, dFte, dDir);

    // Telemetria no Serial
    Serial.printf("[MOTORES] A: %5.2f RPS | B: %5.2f RPS\n", rpsA, rpsB);
    Serial.printf("[IMU]     Yaw: %6.1f deg | Taxa: %5.2f deg/s\n", getYawAngle(), getYawRate());
    Serial.printf("[LASER]   Esq: %4d mm | Frente: %4d mm | Dir: %4d mm\n", dEsq, dFte, dDir);
    Serial.println("--------------------------------------------------");
  }
}