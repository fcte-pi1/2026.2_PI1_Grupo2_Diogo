#include <Arduino.h>
#include "motors.h"
#include "config.h"

static volatile long countA = 0;
static volatile long countB = 0;

static void IRAM_ATTR isrA() {
  if (digitalRead(ENC_A_C2) == HIGH) countA++;
  else countA--;
}

static void IRAM_ATTR isrB() {
  if (digitalRead(ENC_B_C2) == HIGH) countB++;
  else countB--;
}

void initMotors() {
  pinMode(PIN_STBY, OUTPUT);
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_BIN2, OUTPUT);

  ledcAttach(PIN_PWMA, 10000, 8);
  ledcAttach(PIN_PWMB, 10000, 8);
  digitalWrite(PIN_STBY, HIGH);

  pinMode(ENC_A_C1, INPUT_PULLUP);
  pinMode(ENC_A_C2, INPUT_PULLUP);
  pinMode(ENC_B_C1, INPUT_PULLUP);
  pinMode(ENC_B_C2, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_A_C1), isrA, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_B_C1), isrB, RISING);
}

void stopMotors() {
  ledcWrite(PIN_PWMA, 0);
  ledcWrite(PIN_PWMB, 0);
}

void setMotorSpeed(int pwmA, int pwmB) {
  // Motor A
  if (pwmA >= 0) {
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, LOW);
  } else {
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, HIGH);
  }
  ledcWrite(PIN_PWMA, constrain(abs(pwmA), 0, 255));

  // Motor B
  if (pwmB >= 0) {
    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, LOW);
  } else {
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, HIGH);
  }
  ledcWrite(PIN_PWMB, constrain(abs(pwmB), 0, 255));
}

void setTurnEffort(int esforco) {
  esforco = constrain(esforco, -255, 255);
  int pwm = abs(esforco);
  if (pwm > 0 && pwm < 45) pwm = 45; // Vence atrito estático

  if (esforco > 0) {
    // Giro anti-horário (A recua, B avança)
    setMotorSpeed(-pwm, pwm);
  } else if (esforco < 0) {
    // Giro horário (A avança, B recua)
    setMotorSpeed(pwm, -pwm);
  } else {
    stopMotors();
  }
}

void getEncoderSpeeds(float dt, float &rpsA, float &rpsB) {
  if (dt <= 0.0f) return;
  noInterrupts();
  long pA = countA;
  long pB = countB;
  countA = 0;
  countB = 0;
  interrupts();

  rpsA = (pA / PULSOS_POR_VOLTA) / dt;
  rpsB = (pB / PULSOS_POR_VOLTA) / dt;
}