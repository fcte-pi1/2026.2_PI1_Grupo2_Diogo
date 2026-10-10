#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "imu.h"

static Adafruit_MPU6050 mpu;
static float yawAngle = 0.0f;
static float yawRate = 0.0f;
static float biasZ = 0.0f;

bool initIMU() {
  if (!mpu.begin()) {
    Serial.println("[IMU] Falha ao iniciar MPU-6050!");
    return false;
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("[IMU] Calibrando giroscópio (mantenha o robô imóvel)...");
  float soma = 0.0f;
  const int n = 150;
  for (int i = 0; i < n; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    soma += (g.gyro.z * 57.2957795f);
    delay(10);
  }
  biasZ = soma / n;
  Serial.printf("[IMU] Pronto! Bias Z: %.3f deg/s\n", biasZ);
  return true;
}

void updateIMU(float dt) {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  yawRate = (g.gyro.z * 57.2957795f) - biasZ;
  if (abs(yawRate) < 0.25f) yawRate = 0.0f; // Zona morta

  yawAngle += yawRate * dt;
}

float getYawAngle() { return yawAngle; }
float getYawRate() { return yawRate; }
void resetYawAngle() { yawAngle = 0.0f; }