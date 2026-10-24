#include "motion_control.h"
#include "config.h"
#include "motors.h"
#include "encoders.h"
#include <math.h>
#include <stdlib.h>

// V3.1: Controle PI independente para cada roda com referencia acelerada,
// feedforward, anti-windup e ajuda de partida *temporaria*.
// Projeto experimental: calibraremos os ganhos com os proximos CSVs.
namespace {
ControleInfo info;
float cprE_ = 0, cprD_ = 0;
float velocidadeAlvoE = 0, velocidadeAlvoD = 0;
uint32_t anteriorMs = 0;
struct EstadoPI {
    float integral = 0;
    uint32_t ultimoPulsoMs = 0;
    int64_t ultimaContagem = 0;
    bool ajudaAtiva = false;
};
EstadoPI estadoE, estadoD;

float limitar(float x, float minimo, float maximo) {
    return x < minimo ? minimo : (x > maximo ? maximo : x);
}
float aproximar(float atual, float alvo, float passoMaximo) {
    return atual + limitar(alvo - atual, -passoMaximo, passoMaximo);
}

void inicializarBase(float cprE, float cprD) {
    DadosEncoder d = lerEncoders();
    info = ControleInfo{};
    info.inicioE = d.esq;
    info.inicioD = d.dir;
    cprE_ = cprE;
    cprD_ = cprD;
    velocidadeAlvoE = velocidadeAlvoD = 0.0f;
    anteriorMs = millis();
    estadoE = EstadoPI{};
    estadoD = EstadoPI{};
    estadoE.ultimoPulsoMs = estadoD.ultimoPulsoMs = anteriorMs;
    estadoE.ultimaContagem = d.esq;
    estadoD.ultimaContagem = d.dir;
}

float referenciaPosicao(int64_t restante, float cpr, float rpmMax) {
    if (llabs((long long)restante) <= POSICAO_TOLERANCIA_CONTAGENS)
        return 0.0f;
    const float revolucoes = fabsf((float)restante) / cpr;
    const float limitePelaDesaceleracao = 60.0f * sqrtf(2.0f * REV_DESACEL_POR_SEG2 * revolucoes);
    float rpm = fminf(rpmMax, fminf(220.0f * revolucoes, limitePelaDesaceleracao));
    // RPM baixa na aproximacao final pode requerer pulsos curtos de arranque;
    // manter piso apenas na REFERENCIA, nao no PWM permanente.
    rpm = fmaxf(rpm, 18.0f);
    return restante > 0 ? rpm : -rpm;
}

// Ref e real na convencao assinada: positivo = frente.
// "distanciaRestante" so controla a ajuda em POSICAO.
int acionarRoda(float ref, float real, float dt, EstadoPI &estado,
               bool esquerda, bool modoPosicao, int64_t distanciaRestante,
               uint32_t agora) {
    estado.ajudaAtiva = false;
    if (fabsf(ref) < 2.0f) {
        estado.integral = 0;
        if (esquerda) frearMotorEsq(); else frearMotorDir();
        return 0;
    }

    float sinal = ref > 0.0f ? 1.0f : -1.0f;
    float rpmDesejada = fabsf(ref);
    float rpmRealNoSentido = real * sinal;
    float erro = rpmDesejada - rpmRealNoSentido;
    float ff = FF_PWM_BASE + FF_PWM_POR_RPM * rpmDesejada;
    float base = ff + KP_RPM * erro + estado.integral;

    // Anti-windup condicional: o integrador nao cresce empurrando
    // ainda mais o duty quando ele ja saturou; e pode sair da saturacao.
    if ((base > PWM_MINIMO_CONTROLE && base < PWM_LIMITE_CONTROLE) ||
        (base >= PWM_LIMITE_CONTROLE && erro < 0.0f) ||
        (base <= PWM_MINIMO_CONTROLE && erro > 0.0f)) {
        estado.integral = limitar(estado.integral + KI_RPM * erro * dt,
                                  -LIMITE_INTEGRAL_PWM, LIMITE_INTEGRAL_PWM);
    }
    float comando = limitar(ff + KP_RPM * erro + estado.integral,
                           PWM_MINIMO_CONTROLE, PWM_LIMITE_CONTROLE);

    // Se nao houver pulsos por 140 ms, ajuda curta de torque para vencer
    // o atrito ESTATICO; cancela imediatamente quando ha novos pulsos.
    // Proximo do alvo a ajuda e menor para nao provocar grande ultrapassagem.
    uint32_t inativoMs = agora - estado.ultimoPulsoMs;
    if (inativoMs >= PARTIDA_SEM_PULSOS_MS &&
        inativoMs <= PARTIDA_AJUDA_ATE_MS && rpmDesejada >= 12.0f) {
        int minimoAjuda = PWM_AJUDA_PARTIDA;
        if (modoPosicao && llabs((long long)distanciaRestante) < 180)
            minimoAjuda = PWM_AJUDA_PARTIDA_CURTA;
        if (modoPosicao && llabs((long long)distanciaRestante) <= 22)
            minimoAjuda = PWM_AJUDA_APROXIMACAO;
        if (comando < minimoAjuda) {
            comando = (float)minimoAjuda;
            estado.ajudaAtiva = true;
        }
    }
    int pwm = (int)lroundf(comando);
    if (esquerda) setMotorEsq(pwm, sinal > 0);
    else setMotorDir(pwm, sinal > 0);
    return pwm;
}
} // namespace

bool iniciarControleRpm(float rpmE, float rpmD, float cprE, float cprD) {
    if (info.tipo != TipoControle::INATIVO || cprE <= 0 || cprD <= 0) return false;
    if (fabsf(rpmE) > RPM_MAX_TESTE || fabsf(rpmD) > RPM_MAX_TESTE ||
        (rpmE != 0 && fabsf(rpmE) < 10.0f) ||
        (rpmD != 0 && fabsf(rpmD) < 10.0f) ||
        (rpmE == 0 && rpmD == 0)) return false;
    inicializarBase(cprE,cprD);
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
    inicializarBase(cprE,cprD);
    info.tipo = TipoControle::POSICAO;
    info.alvoE = alvoE;
    info.alvoD = alvoD;
    info.limiteRpm = limiteRpm;
    return true;
}

const char* atualizarControleMovimento() {
    if (info.tipo == TipoControle::INATIVO) return nullptr;
    uint32_t agora = millis();
    if (agora - anteriorMs < PERIODO_CONTROLE_MS) return nullptr;
    float dt = (agora - anteriorMs) * 0.001f;
    anteriorMs = agora;
    if (dt <= 0.0f || dt > 0.2f) return "atraso_loop_controle";

    DadosEncoder d = lerEncoders();
    info.progressoE = d.esq - info.inicioE;
    info.progressoD = d.dir - info.inicioD;
    info.realE = d.cpsEsq * 60.0f / cprE_;
    info.realD = d.cpsDir * 60.0f / cprD_;

    if (d.esq != estadoE.ultimaContagem) {
        estadoE.ultimoPulsoMs = agora;
        estadoE.ultimaContagem = d.esq;
    }
    if (d.dir != estadoD.ultimaContagem) {
        estadoD.ultimoPulsoMs = agora;
        estadoD.ultimaContagem = d.dir;
    }

    int64_t restanteE = 0, restanteD = 0;
    if (info.tipo == TipoControle::POSICAO) {
        restanteE = (int64_t)info.alvoE - info.progressoE;
        restanteD = (int64_t)info.alvoD - info.progressoD;
        // Os motores nao revertem o sentido se ultrapassarem a meta.
        info.finalizouE = info.alvoE == 0 ||
            (info.alvoE > 0 ? restanteE <= POSICAO_TOLERANCIA_CONTAGENS :
                              restanteE >= -POSICAO_TOLERANCIA_CONTAGENS);
        info.finalizouD = info.alvoD == 0 ||
            (info.alvoD > 0 ? restanteD <= POSICAO_TOLERANCIA_CONTAGENS :
                              restanteD >= -POSICAO_TOLERANCIA_CONTAGENS);
        if (info.finalizouE && info.finalizouD) {
            info.rampaE = info.rampaD = 0;
            info.ajudaPartidaE = info.ajudaPartidaD = false;
            return "posicao_meta_atingida";
        }
        info.desejadoE = info.finalizouE ? 0 : referenciaPosicao(restanteE, cprE_, info.limiteRpm);
        info.desejadoD = info.finalizouD ? 0 : referenciaPosicao(restanteD, cprD_, info.limiteRpm);
        if (info.alvoE != 0 && !info.finalizouE)
            info.desejadoE = copysignf(fabsf(info.desejadoE), (float)info.alvoE);
        if (info.alvoD != 0 && !info.finalizouD)
            info.desejadoD = copysignf(fabsf(info.desejadoD), (float)info.alvoD);
    }

    float passoMax = RPM_ACELERACAO_POR_SEG * dt;
    velocidadeAlvoE = aproximar(velocidadeAlvoE, info.desejadoE, passoMax);
    velocidadeAlvoD = aproximar(velocidadeAlvoD, info.desejadoD, passoMax);
    if (info.tipo == TipoControle::POSICAO && info.finalizouE) velocidadeAlvoE = 0;
    if (info.tipo == TipoControle::POSICAO && info.finalizouD) velocidadeAlvoD = 0;

    info.rampaE = velocidadeAlvoE;
    info.rampaD = velocidadeAlvoD;
    info.pwmE = acionarRoda(velocidadeAlvoE, info.realE, dt, estadoE, true,
                           info.tipo == TipoControle::POSICAO, restanteE, agora);
    info.pwmD = acionarRoda(velocidadeAlvoD, info.realD, dt, estadoD, false,
                           info.tipo == TipoControle::POSICAO, restanteD, agora);
    info.ajudaPartidaE = estadoE.ajudaAtiva;
    info.ajudaPartidaD = estadoD.ajudaAtiva;
    info.semPulsosMsE = agora - estadoE.ultimoPulsoMs;
    info.semPulsosMsD = agora - estadoD.ultimoPulsoMs;

    // Seguranca: parar se o sentido medido diverge do comando.
    if (info.pwmE > 0 && fabsf(velocidadeAlvoE) >= 10 &&
        (d.esq - info.inicioE) * (velocidadeAlvoE > 0 ? 1 : -1) < -12)
        return "sinal_encoder_esquerdo_invertido";
    if (info.pwmD > 0 && fabsf(velocidadeAlvoD) >= 10 &&
        (d.dir - info.inicioD) * (velocidadeAlvoD > 0 ? 1 : -1) < -12)
        return "sinal_encoder_direito_invertido";

    // Diagnostico de travamento: referencia solicitada mas SEM pulsos.
    if (fabsf(velocidadeAlvoE) >= 10 && info.semPulsosMsE > CONTROLE_STALL_MS)
        return "encoder_ou_motor_esquerdo_sem_movimento";
    if (fabsf(velocidadeAlvoD) >= 10 && info.semPulsosMsD > CONTROLE_STALL_MS)
        return "encoder_ou_motor_direito_sem_movimento";
    return nullptr;
}

void desativarControleMovimento() {
    info.tipo = TipoControle::INATIVO;
    estadoE.integral = estadoD.integral = 0;
    velocidadeAlvoE = velocidadeAlvoD = 0;
    info.pwmE = info.pwmD = 0;
    info.desejadoE = info.desejadoD = 0;
    info.rampaE = info.rampaD = 0;
    info.ajudaPartidaE = info.ajudaPartidaD = false;
}

ControleInfo lerControleMovimento() { return info; }
