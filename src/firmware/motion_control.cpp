#include "motion_control.h"
#include "config.h"
#include "motors.h"
#include "encoders.h"
#include <math.h>
#include <stdlib.h>

namespace {
ControleInfo info;
float cprE_ = 0, cprD_ = 0;
float velocidadeAlvoE = 0, velocidadeAlvoD = 0;
float integralE = 0, integralD = 0;
uint32_t anteriorMs = 0;
uint32_t ultimoMovE = 0, ultimoMovD = 0;
int64_t ultimaContE = 0, ultimaContD = 0;

constexpr float KP_RPM = 0.30f; // ganhos INICIAIS para ajuste em bancada
constexpr float KI_RPM = 0.50f; // PWM/(RPM*s)
constexpr float K_POSICAO_RPM_POR_VOLTA = 220.0f;
constexpr float LIMITE_INTEGRAL = 45.0f;

float limitar(float x, float inferior, float superior) {
    return x < inferior ? inferior : (x > superior ? superior : x);
}

void inicializarBase(float cprE, float cprD) {
    DadosEncoder d = lerEncoders();
    info.inicioE = d.esq; info.inicioD = d.dir;
    info.alvoE = info.alvoD = 0;
    info.limiteRpm = 0;
    info.progressoE = info.progressoD = 0;
    info.desejadoE = info.desejadoD = 0;
    info.realE = info.realD = 0;
    info.pwmE = info.pwmD = 0;
    info.finalizouE = info.finalizouD = false;
    cprE_ = cprE; cprD_ = cprD;
    integralE = integralD = 0;
    velocidadeAlvoE = velocidadeAlvoD = 0;
    anteriorMs = millis();
    ultimoMovE = ultimoMovD = anteriorMs;
    ultimaContE = d.esq; ultimaContD = d.dir;
}

float referenciaPosicao(int64_t restante, float cpr, float maxRpm) {
    if (llabs(static_cast<long long>(restante)) <= POSICAO_TOLERANCIA_CONTAGENS)
        return 0;
    const float revolucoes = fabsf(static_cast<float>(restante)) / cpr;
    const float limiteFreio = 60.0f * sqrtf(2.0f * REV_DESACEL_POR_SEG2 * revolucoes);
    float rpm = fminf(maxRpm, fminf(K_POSICAO_RPM_POR_VOLTA * revolucoes, limiteFreio));
    rpm = fmaxf(rpm, 18.0f);  // aproximação lenta; testar capacidade real de partida
    return restante > 0 ? rpm : -rpm;
}

// Controlador PI com anti-windup simples e feedforward empírico;
// NÃO é uma malha sintonizada para o solo da competição.
int aplicarRoda(float referencia, float velocidade, float dt,
               float &integral, bool esquerda) {
    if (fabsf(referencia) < 2.0f) {
        integral = 0;
        if (esquerda) frearMotorEsq(); else frearMotorDir();
        return 0;
    }
    float erro = referencia - velocidade;
    float sinal = referencia > 0 ? 1.0f : -1.0f;
    float feedforward = 8.0f + 0.44f * fabsf(referencia);
    float propostoI = limitar(integral + erro * dt * sinal * KI_RPM,
                              -LIMITE_INTEGRAL, LIMITE_INTEGRAL);
    float dutyComI = feedforward + KP_RPM * erro * sinal + propostoI;
    if (dutyComI >= 0 && dutyComI <= PWM_LIMITE_CONTROLE) integral = propostoI;
    float duty = limitar(feedforward + KP_RPM * erro * sinal + integral,
                         PWM_MINIMO_TENTATIVA, PWM_LIMITE_CONTROLE);
    int pwm = static_cast<int>(lroundf(duty));
    if (esquerda) setMotorEsq(pwm, referencia > 0);
    else setMotorDir(pwm, referencia > 0);
    return pwm;
}

float aproximar(float atual, float alvo, float passoMaximo) {
    return atual + limitar(alvo - atual, -passoMaximo, passoMaximo);
}
} // namespace

bool iniciarControleRpm(float rpmE, float rpmD, float cprE, float cprD) {
    if (info.tipo != TipoControle::INATIVO || cprE <= 0 || cprD <= 0) return false;
    if (fabsf(rpmE) > RPM_MAX_TESTE || fabsf(rpmD) > RPM_MAX_TESTE ||
        (fabsf(rpmE) < 10.0f && fabsf(rpmD) < 10.0f) ||
        (rpmE != 0 && fabsf(rpmE) < 10.0f) ||
        (rpmD != 0 && fabsf(rpmD) < 10.0f)) return false;
    inicializarBase(cprE, cprD);
    info.tipo = TipoControle::RPM;
    info.desejadoE = rpmE;
    info.desejadoD = rpmD;
    return true;
}

bool iniciarControlePosicao(int32_t alvoE, int32_t alvoD,
                            float limiteRpm, float cprE, float cprD) {
    if (info.tipo != TipoControle::INATIVO || cprE <= 0 || cprD <= 0 ||
        limiteRpm < 20 || limiteRpm > RPM_MAX_POSICAO ||
        abs(alvoE) > POSICAO_META_MAX_CONTAGENS ||
        abs(alvoD) > POSICAO_META_MAX_CONTAGENS ||
        (alvoE == 0 && alvoD == 0)) return false;
    inicializarBase(cprE, cprD);
    info.tipo = TipoControle::POSICAO;
    info.alvoE = alvoE; info.alvoD = alvoD;
    info.limiteRpm = limiteRpm;
    return true;
}

const char* atualizarControleMovimento() {
    if (info.tipo == TipoControle::INATIVO) return nullptr;
    uint32_t agora = millis();
    if (agora - anteriorMs < PERIODO_CONTROLE_MS) return nullptr;
    float dt = (agora - anteriorMs) * 0.001f;
    anteriorMs = agora;
    if (dt <= 0 || dt > 0.2f) return "atraso_loop_controle";

    DadosEncoder d = lerEncoders();
    info.progressoE = d.esq - info.inicioE;
    info.progressoD = d.dir - info.inicioD;
    info.realE = d.cpsEsq * 60.0f / cprE_;
    info.realD = d.cpsDir * 60.0f / cprD_;

    if (info.tipo == TipoControle::POSICAO) {
        int64_t restanteE = static_cast<int64_t>(info.alvoE) - info.progressoE;
        int64_t restanteD = static_cast<int64_t>(info.alvoD) - info.progressoD;
        // Não reverter o motor após passar do alvo no primeiro ensaio.
        info.finalizouE = info.alvoE == 0 ||
           (info.alvoE > 0 ? restanteE <= POSICAO_TOLERANCIA_CONTAGENS :
                            restanteE >= -POSICAO_TOLERANCIA_CONTAGENS);
        info.finalizouD = info.alvoD == 0 ||
           (info.alvoD > 0 ? restanteD <= POSICAO_TOLERANCIA_CONTAGENS :
                            restanteD >= -POSICAO_TOLERANCIA_CONTAGENS);
        if (info.finalizouE && info.finalizouD) return "posicao_meta_atingida";
        info.desejadoE = info.finalizouE ? 0 :
            referenciaPosicao(restanteE, cprE_, info.limiteRpm);
        info.desejadoD = info.finalizouD ? 0 :
            referenciaPosicao(restanteD, cprD_, info.limiteRpm);
        // Força direção planejada; referência nunca inverte por erro de posição.
        if (info.alvoE != 0 && !info.finalizouE)
            info.desejadoE = copysignf(fabsf(info.desejadoE), (float)info.alvoE);
        if (info.alvoD != 0 && !info.finalizouD)
            info.desejadoD = copysignf(fabsf(info.desejadoD), (float)info.alvoD);
    }
    float passo = RPM_ACELERACAO_POR_SEG * dt;
    velocidadeAlvoE = aproximar(velocidadeAlvoE, info.desejadoE, passo);
    velocidadeAlvoD = aproximar(velocidadeAlvoD, info.desejadoD, passo);
    // Na conclusão individual, frear a roda imediatamente.
    if (info.tipo == TipoControle::POSICAO && info.finalizouE) velocidadeAlvoE = 0;
    if (info.tipo == TipoControle::POSICAO && info.finalizouD) velocidadeAlvoD = 0;

    info.pwmE = aplicarRoda(velocidadeAlvoE, info.realE, dt, integralE, true);
    info.pwmD = aplicarRoda(velocidadeAlvoD, info.realD, dt, integralD, false);

    // Falha de direção: a referência para frente requer contagem positiva.
    if (info.pwmE > 0 && fabsf(velocidadeAlvoE) >= 10 &&
        (d.esq - info.inicioE) * (velocidadeAlvoE > 0 ? 1 : -1) < -12) return "sinal_encoder_esquerdo_invertido";
    if (info.pwmD > 0 && fabsf(velocidadeAlvoD) >= 10 &&
        (d.dir - info.inicioD) * (velocidadeAlvoD > 0 ? 1 : -1) < -12) return "sinal_encoder_direito_invertido";

    if (d.esq != ultimaContE) { ultimoMovE = agora; ultimaContE = d.esq; }
    if (d.dir != ultimaContD) { ultimoMovD = agora; ultimaContD = d.dir; }
    if (info.pwmE > 45 && agora - ultimoMovE > CONTROLE_STALL_MS) return "encoder_ou_motor_esquerdo_sem_movimento";
    if (info.pwmD > 45 && agora - ultimoMovD > CONTROLE_STALL_MS) return "encoder_ou_motor_direito_sem_movimento";
    return nullptr;
}

void desativarControleMovimento() {
    info.tipo = TipoControle::INATIVO;
    integralE = integralD = 0;
    velocidadeAlvoE = velocidadeAlvoD = 0;
    info.pwmE = info.pwmD = 0;
    info.desejadoE = info.desejadoD = 0;
}

ControleInfo lerControleMovimento() { return info; }
