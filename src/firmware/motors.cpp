#include "motors.h"
#include "config.h"
#include <esp_arduino_version.h>

// NOVA ESTRUTURA: canal A -> motor ESQUERDO (AO1/AO2);
//                 canal B -> motor DIREITO (BO1/BO2).
// As saidas AO1/AO2/BO1/BO2 NAO sao GPIOs da ESP32.
// Frente para cada motor e configuravel por ESQ_FRENTE_IN1_ALTO
// e DIR_FRENTE_IN1_ALTO; confirmar fisicamente com as rodas suspensas.

static bool pwmConfigurado = false;
static bool standbyHabilitado = false;

#if ESP_ARDUINO_VERSION_MAJOR < 3
constexpr uint8_t CANAL_A_LEDC = 0;
constexpr uint8_t CANAL_B_LEDC = 1;
#endif

static void pwmCanalA(uint8_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(PIN_PWMA, duty);
#else
    ledcWrite(CANAL_A_LEDC, duty);
#endif
}

static void pwmCanalB(uint8_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(PIN_PWMB, duty);
#else
    ledcWrite(CANAL_B_LEDC, duty);
#endif
}

void setupMotors() {
    pinMode(PIN_STBY, OUTPUT);
    digitalWrite(PIN_STBY, LOW);
    standbyHabilitado = false;

    pinMode(PIN_AIN1, OUTPUT);
    pinMode(PIN_AIN2, OUTPUT);
    pinMode(PIN_BIN1, OUTPUT);
    pinMode(PIN_BIN2, OUTPUT);

    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
    bool okA = ledcAttach(PIN_PWMA, PWM_FREQUENCIA_HZ, PWM_RESOLUCAO_BITS);
    bool okB = ledcAttach(PIN_PWMB, PWM_FREQUENCIA_HZ, PWM_RESOLUCAO_BITS);
    pwmConfigurado = okA && okB;
    if (!pwmConfigurado) {
        Serial.println("ERRO: falha ao configurar PWM LEDC.");
    }
#else
    double freqA = ledcSetup(CANAL_A_LEDC, PWM_FREQUENCIA_HZ, PWM_RESOLUCAO_BITS);
    double freqB = ledcSetup(CANAL_B_LEDC, PWM_FREQUENCIA_HZ, PWM_RESOLUCAO_BITS);
    pwmConfigurado = freqA > 0 && freqB > 0;
    ledcAttachPin(PIN_PWMA, CANAL_A_LEDC);
    ledcAttachPin(PIN_PWMB, CANAL_B_LEDC);
#endif

    pwmCanalA(0);
    pwmCanalB(0);
    // Mantem ponte H DESABILITADA ate um comando explicito de movimento.
    pararMotores();
}

bool motorHardwareReady() { return pwmConfigurado; }
bool ponteHabilitada() { return standbyHabilitado; }

// Para iniciar PWM/RPM/posicao/giro: apenas habilita o driver com sinais ZERO.
// O controlador decide o PWM no loop seguinte. Jamais usar duty=255 para parar.
void prepararMotoresControle() {
    if (!pwmConfigurado) return;
    digitalWrite(PIN_STBY, LOW);
    standbyHabilitado = false;
    pwmCanalA(0);
    pwmCanalB(0);
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
    digitalWrite(PIN_STBY, HIGH);
    standbyHabilitado = true;
}

// V4.4.1 - DIAGNOSTICO DE PARADA SEGURA
// Esta variante NAO usa PWM=255 ao "frear".
// Para evitar movimento indevido em uma placa ainda nao validada,
// a parada global zera os PWMs e coloca STBY em LOW.
// ATENCAO: isso e modo COAST/STANDBY, NAO short-brake ativo!

// Motor esquerdo LOGICO -> canal A (AO1/AO2).
void setMotorEsq(int pwm, bool frente) {
    if (!pwmConfigurado) return;
    pwm = constrain(pwm, 0, PWM_LIMITE_TESTE);
    pwmCanalA(0);
    if (pwm == 0) {
        digitalWrite(PIN_AIN1, LOW);
        digitalWrite(PIN_AIN2, LOW);
        return; // nao reabilita o driver por um comando 0
    }
    const bool in1 = frente ? ESQ_FRENTE_IN1_ALTO : !ESQ_FRENTE_IN1_ALTO;
    digitalWrite(PIN_AIN1, in1 ? HIGH : LOW);
    digitalWrite(PIN_AIN2, in1 ? LOW : HIGH);
    pwmCanalA(static_cast<uint8_t>(pwm));
    digitalWrite(PIN_STBY, HIGH); // so habilita apos configurar as entradas
    standbyHabilitado = true;
}

// Motor direito LOGICO -> canal B (BO1/BO2).
void setMotorDir(int pwm, bool frente) {
    if (!pwmConfigurado) return;
    pwm = constrain(pwm, 0, PWM_LIMITE_TESTE);
    pwmCanalB(0);
    if (pwm == 0) {
        digitalWrite(PIN_BIN1, LOW);
        digitalWrite(PIN_BIN2, LOW);
        return;
    }
    const bool in1 = frente ? DIR_FRENTE_IN1_ALTO : !DIR_FRENTE_IN1_ALTO;
    digitalWrite(PIN_BIN1, in1 ? HIGH : LOW);
    digitalWrite(PIN_BIN2, in1 ? LOW : HIGH);
    pwmCanalB(static_cast<uint8_t>(pwm));
    digitalWrite(PIN_STBY, HIGH);
    standbyHabilitado = true;
}

void pararMotores() {
    // Prioridade: inibir fisicamente ambos canais antes de qualquer outra escrita.
    digitalWrite(PIN_STBY, LOW);
    standbyHabilitado = false;
    pwmCanalA(0);
    pwmCanalB(0);
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
}

// Nas paradas individuais do controle PI, nao impor PWM maximo.
// O outro motor pode continuar girando; STBY e compartilhado.
void frearMotorEsq() {
    pwmCanalA(0);
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
}

void frearMotorDir() {
    pwmCanalB(0);
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
}

void frearMotoresTudo() {
    pararMotores(); // modo seguro STBY, sem freio eletrico ativo
}
