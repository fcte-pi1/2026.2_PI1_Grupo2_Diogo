#include "encoders.h"
#include "config.h"
#include <ESP32Encoder.h>

static ESP32Encoder esq;
static ESP32Encoder dir;
static int64_t ultimoEsq = 0;
static int64_t ultimoDir = 0;
static float cpsEsq = 0.0f;
static float cpsDir = 0.0f;
static uint32_t ultimoUs = 0;

bool iniciarEncoders() {
    // Adequado a GPIOs com níveis 3,3 V; verifique a elétrica do seu encoder.
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    esq.attachFullQuad(ENC_ESQ_A, ENC_ESQ_B);
    dir.attachFullQuad(ENC_DIR_A, ENC_DIR_B);
    if (!esq.isAttached() || !dir.isAttached()) {
        Serial.println("ERRO: PCNT nao conseguiu registrar os dois encoders.");
        return false;
    }
    esq.clearCount();
    dir.clearCount();
    ultimoEsq = ultimoDir = 0;
    cpsEsq = cpsDir = 0;
    ultimoUs = micros();
    Serial.println("Encoders: quadratura X4 por PCNT ativada.");
    return true;
}

void atualizarEncoders() {
    uint32_t agora = micros();
    uint32_t dtUs = agora - ultimoUs;
    if (dtUs < AMOSTRA_ENCODER_MS * 1000UL) return;
    int64_t atualE = esq.getCount() * SINAL_ENCODER_ESQ;
    int64_t atualD = dir.getCount() * SINAL_ENCODER_DIR;
    float dt = dtUs / 1000000.0f;
    float instantE = (atualE - ultimoEsq) / dt;
    float instantD = (atualD - ultimoDir) / dt;
    constexpr float alpha = 0.35f;
    cpsEsq += alpha * (instantE - cpsEsq);
    cpsDir += alpha * (instantD - cpsDir);
    ultimoEsq = atualE;
    ultimoDir = atualD;
    ultimoUs = agora;
}

DadosEncoder lerEncoders() {
    return {esq.getCount() * SINAL_ENCODER_ESQ,
            dir.getCount() * SINAL_ENCODER_DIR,
            cpsEsq, cpsDir};
}

void zerarEncoders() {
    esq.clearCount();
    dir.clearCount();
    ultimoEsq = ultimoDir = 0;
    cpsEsq = cpsDir = 0;
    ultimoUs = micros();
}
