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

// =============================================================================
// CONTROLE DE PWM DOS MOTORES (CÓDIGO NOVO ATIVO)
// =============================================================================
// Configuração de canais PWM (LEDC) compatível com o framework Arduino-ESP32
// instalado pelo PlatformIO. Utiliza canais 0 e 1 em 10 kHz com resolução de 8 bits.
static const int PWM_CH_A = 0;
static const int PWM_CH_B = 1;

void initMotors() {
  pinMode(PIN_STBY, OUTPUT);
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_BIN2, OUTPUT);

  // Inicializa canais PWM e associa aos pinos PWMA e PWMB
  ledcSetup(PWM_CH_A, 10000, 8);
  ledcAttachPin(PIN_PWMA, PWM_CH_A);
  ledcSetup(PWM_CH_B, 10000, 8);
  ledcAttachPin(PIN_PWMB, PWM_CH_B);

  digitalWrite(PIN_STBY, HIGH);

  pinMode(ENC_A_C1, INPUT_PULLUP);
  pinMode(ENC_A_C2, INPUT_PULLUP);
  pinMode(ENC_B_C1, INPUT_PULLUP);
  pinMode(ENC_B_C2, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_A_C1), isrA, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_B_C1), isrB, RISING);
}

void stopMotors() {
  ledcWrite(PWM_CH_A, 0);
  ledcWrite(PWM_CH_B, 0);
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
  ledcWrite(PWM_CH_A, constrain(abs(pwmA), 0, 255));

  // Motor B
  if (pwmB >= 0) {
    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, LOW);
  } else {
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, HIGH);
  }
  ledcWrite(PWM_CH_B, constrain(abs(pwmB), 0, 255));
}

// =============================================================================
// CÓDIGO ANTERIOR ORIGINAL (COMENTADO PARA REFERÊNCIA)
// =============================================================================
// Motivo do comentário: O código original abaixo utiliza a API do ESP32 Arduino Core v3
// (`ledcAttach(pin, freq, res)` diretamente no pino). Como o pacote platform-espressif32
// atual do projeto utiliza a API por canais (Core v2.x), a compilação gerava o erro:
//   "error: 'ledcAttach' was not declared in this scope; suggested alternative: 'ledcAttachPin'"
// Caso o projeto seja futuramente atualizado para o Core v3, o bloco abaixo pode ser reativado.
// =============================================================================
/*
void initMotors_ORIGINAL() {
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

void stopMotors_ORIGINAL() {
  ledcWrite(PIN_PWMA, 0);
  ledcWrite(PIN_PWMB, 0);
}

void setMotorSpeed_ORIGINAL(int pwmA, int pwmB) {
  if (pwmA >= 0) {
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, LOW);
  } else {
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, HIGH);
  }
  ledcWrite(PIN_PWMA, constrain(abs(pwmA), 0, 255));

  if (pwmB >= 0) {
    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, LOW);
  } else {
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, HIGH);
  }
  ledcWrite(PIN_PWMB, constrain(abs(pwmB), 0, 255));
}
*/

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