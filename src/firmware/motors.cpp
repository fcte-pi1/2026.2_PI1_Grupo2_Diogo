#include "motors.h"
#include "config.h"
#include <esp_arduino_version.h>

// LIGACAO CONFIRMADA NO CARRINHO:
// Canal A do TB6612FNG (PWMA/AIN1/AIN2) = roda fisica DIREITA.
// Canal B do TB6612FNG (PWMB/BIN1/BIN2) = roda fisica ESQUERDA.
//
// A convencao "frente" original estava invertida em ambos os canais.
// Depois da correcao:
//   esquerda frente: BIN1=LOW,  BIN2=HIGH (canal B)
//   direita frente:  AIN1=HIGH, AIN2=LOW  (canal A)
// O nome do canal fisico NAO precisa coincidir com o lado logico do robo.

static bool pwmConfigurado = false;

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
    digitalWrite(PIN_STBY, pwmConfigurado ? HIGH : LOW);
    pararMotores();
}

bool motorHardwareReady() { return pwmConfigurado; }

// Motor esquerdo LOGICO = canal B FISICO.
void setMotorEsq(int pwm, bool frente) {
    if (!pwmConfigurado) return;
    pwm = constrain(pwm, 0, 255);
    pwmCanalB(0);
    digitalWrite(PIN_BIN1, (pwm > 0 && !frente) ? HIGH : LOW);
    digitalWrite(PIN_BIN2, (pwm > 0 && frente) ? HIGH : LOW);
    pwmCanalB(static_cast<uint8_t>(pwm));
}

// Motor direito LOGICO = canal A FISICO.
void setMotorDir(int pwm, bool frente) {
    if (!pwmConfigurado) return;
    pwm = constrain(pwm, 0, 255);
    pwmCanalA(0);
    digitalWrite(PIN_AIN1, (pwm > 0 && frente) ? HIGH : LOW);
    digitalWrite(PIN_AIN2, (pwm > 0 && !frente) ? HIGH : LOW);
    pwmCanalA(static_cast<uint8_t>(pwm));
}

void pararMotores() {
    pwmCanalA(0);
    pwmCanalB(0);
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
}

// Funcoes de frenagem tambem seguem LADO LOGICO, nao canal fisico.
void frearMotorEsq() {  // freia roda esquerda, ligada ao canal B
    pwmCanalB(0);
    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, HIGH);
    pwmCanalB(255);
}

void frearMotorDir() { // freia roda direita, ligada ao canal A
    pwmCanalA(0);
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, HIGH);
    pwmCanalA(255);
}

void frearMotoresTudo() {
    frearMotorEsq();
    frearMotorDir();
}
