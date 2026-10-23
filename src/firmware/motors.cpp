#include "motors.h"
#include "config.h"
#include <esp_arduino_version.h>

static bool pwmConfigurado = false;

#if ESP_ARDUINO_VERSION_MAJOR < 3
constexpr uint8_t CANAL_ESQ = 0;
constexpr uint8_t CANAL_DIR = 1;
#endif

static void pwmEsq(uint8_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(PIN_PWMA, duty);
#else
    ledcWrite(CANAL_ESQ, duty);
#endif
}

static void pwmDir(uint8_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(PIN_PWMB, duty);
#else
    ledcWrite(CANAL_DIR, duty);
#endif
}

void setupMotors() {
    // Desabilita o driver durante a configuração.
    pinMode(PIN_STBY, OUTPUT);
    digitalWrite(PIN_STBY, LOW);
    pinMode(PIN_AIN1, OUTPUT);
    pinMode(PIN_AIN2, OUTPUT);
    pinMode(PIN_BIN1, OUTPUT);
    pinMode(PIN_BIN2, OUTPUT);
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    bool okE = ledcAttach(PIN_PWMA, PWM_FREQUENCIA_HZ, PWM_RESOLUCAO_BITS);
    bool okD = ledcAttach(PIN_PWMB, PWM_FREQUENCIA_HZ, PWM_RESOLUCAO_BITS);
    pwmConfigurado = okE && okD;
    if (!pwmConfigurado) {
        Serial.println("ERRO: falha ao configurar PWM LEDC.");
    }
#else
    double freqE = ledcSetup(CANAL_ESQ, PWM_FREQUENCIA_HZ, PWM_RESOLUCAO_BITS);
    double freqD = ledcSetup(CANAL_DIR, PWM_FREQUENCIA_HZ, PWM_RESOLUCAO_BITS);
    pwmConfigurado = freqE > 0 && freqD > 0;
    ledcAttachPin(PIN_PWMA, CANAL_ESQ);
    ledcAttachPin(PIN_PWMB, CANAL_DIR);
#endif
    pwmEsq(0);
    pwmDir(0);
    digitalWrite(PIN_STBY, pwmConfigurado ? HIGH : LOW);
    pararMotores();
}

bool motorHardwareReady() { return pwmConfigurado; }

void setMotorEsq(int pwm, bool frente) {
    pwm = constrain(pwm, 0, 255);
    pwmEsq(0);
    digitalWrite(PIN_AIN1, (pwm && !frente) ? HIGH : LOW);
    digitalWrite(PIN_AIN2, (pwm && frente) ? HIGH : LOW);
    pwmEsq(static_cast<uint8_t>(pwm));
}

void setMotorDir(int pwm, bool frente) {
    pwm = constrain(pwm, 0, 255);
    pwmDir(0);
    digitalWrite(PIN_BIN1, (pwm && frente) ? HIGH : LOW);
    digitalWrite(PIN_BIN2, (pwm && !frente) ? HIGH : LOW);
    pwmDir(static_cast<uint8_t>(pwm));
}

void pararMotores() {
    pwmEsq(0);
    pwmDir(0);
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
}

void frearMotorEsq() {
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, HIGH);
    pwmEsq(255);
}

void frearMotorDir() {
    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, HIGH);
    pwmDir(255);
}

void frearMotoresTudo() {
    frearMotorEsq();
    frearMotorDir();
}
