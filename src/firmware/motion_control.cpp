#include "motion_control.h"
#include "config.h"
#include "motors.h"
#include "encoders.h"
#include "imu.h"
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
int sentidoGiro = 1;  // +1 direita; E frente D tras
uint32_t syncForaDesde = 0;
float alvoFisicoMm = 0;


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
    syncForaDesde = 0;
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


bool iniciarControleGiro(bool direita, bool usaImu, float limiteRpm,
                         float cprE, float cprD) {
    if (info.tipo != TipoControle::INATIVO || cprE <= 0 || cprD <= 0 ||
        limiteRpm < 20 || limiteRpm > GIRO_RPM_MAX) return false;
    if (usaImu && !zerarYawIMU()) return false;
    inicializarBase(cprE, cprD);
    sentidoGiro = direita ? +1 : -1;
    const float distancia = 3.14159265358979323846f * ENTRE_RODAS_MM * 0.25f;
    alvoFisicoMm = distancia;
    info.tipo = usaImu ? TipoControle::GIRO_IMU : TipoControle::GIRO_ENCODER;
    info.usandoIMU = usaImu;
    info.alvoYawGraus = direita ? 90.0f : -90.0f;
    // No encoder puro, o IMU NAO interfere: le apenas para o CSV.
    DadosIMU g = lerIMU();
    info.yawNoInicio = g.confiavel ? g.yawDireitaGraus : 0;
    info.alvoE = sentidoGiro * (int32_t)lroundf(distancia * cprE / (3.14159265358979323846f * DIAMETRO_RODA_MM));
    info.alvoD = -sentidoGiro * (int32_t)lroundf(distancia * cprD / (3.14159265358979323846f * DIAMETRO_RODA_MM));
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
        syncForaDesde = 0;
    estadoE.ultimaContagem = d.esq;
    }
    if (d.dir != estadoD.ultimaContagem) {
        estadoD.ultimoPulsoMs = agora;
        estadoD.ultimaContagem = d.dir;
    }

    // V4: giro simetrico com duas rodas obrigatoriamente sincronizadas.
    const bool giroE = info.tipo == TipoControle::GIRO_ENCODER;
    const bool giroI = info.tipo == TipoControle::GIRO_IMU;
    const bool girando = giroE || giroI;
    DadosIMU imu = lerIMU();
    if (girando) {
        info.yawMedido = imu.confiavel ? imu.yawDireitaGraus - info.yawNoInicio : NAN;
        info.erroYaw = imu.confiavel ? info.alvoYawGraus - info.yawMedido : NAN;
        const float cir = 3.14159265358979323846f * DIAMETRO_RODA_MM;
        const float viagemE = (float)(sentidoGiro * info.progressoE) * cir / cprE_;
        const float viagemD = (float)(-sentidoGiro * info.progressoD) * cir / cprD_;
        info.distanciaE = viagemE;
        info.distanciaD = viagemD;
        info.syncErroMm = viagemE - viagemD;
        info.syncErroMaxMm = fmaxf(info.syncErroMaxMm, fabsf(info.syncErroMm));
        if (viagemE < -2.0f || viagemD < -2.0f) return "giro_sentido_encoder_incorreto";
        if (fabsf(info.syncErroMm) > GIRO_SYNC_FALHA_MM) {
            if (!syncForaDesde) syncForaDesde = agora;
            if (agora - syncForaDesde > GIRO_SYNC_FALHA_MS) return "dessincronia_limite";
        } else syncForaDesde = 0;

        float rpmBase = 0;
        if (giroE) {
            // Meta encoder = distancia ideal; sincronismo atua DURANTE o movimento.
            const float faltandoE = alvoFisicoMm - viagemE;
            const float faltandoD = alvoFisicoMm - viagemD;
            if (faltandoE <= 0.5f && faltandoD <= 0.5f) return "giro_encoder_concluido";
            const float rpmEsq = referenciaPosicao((int64_t)lroundf(fmaxf(0, faltandoE) * cprE_ / cir), cprE_, info.limiteRpm);
            const float rpmDir = referenciaPosicao((int64_t)lroundf(fmaxf(0, faltandoD) * cprD_ / cir), cprD_, info.limiteRpm);
            rpmBase = fminf(fabsf(rpmEsq), fabsf(rpmDir));
        } else {
            if (!imu.confiavel) return "imu_indisponivel_ou_atrasada";
            if (viagemE > alvoFisicoMm * GIRO_LIMITE_DESLOCAMENTO_REL ||
                viagemD > alvoFisicoMm * GIRO_LIMITE_DESLOCAMENTO_REL) return "imu_limite_encoder_excedido";
            const float progressoYaw = (float)sentidoGiro * info.yawMedido;
            if (fmaxf(viagemE, viagemD) > 8.0f && progressoYaw < -3.0f)
                return "imu_sentido_inverso_conferir_montagem";
            if (progressoYaw > 112.0f) return "imu_angulo_limite_excedido";
            float falta = 90.0f - progressoYaw;
            // Freia ao atingir o angulo dentro da tolerancia, sem trocar
            // de sentido; resultado final e conferido depois do freio.
            if (falta <= IMU_TOLERANCIA_ANGULO_GRAUS) {
                if (fabsf(info.syncErroMm) > GIRO_SYNC_PAUSA_MM)
                    return "imu_alvo_com_dessincronia";
                return "giro_imu_concluido";
            }
            const float ganhoAngular = 6.0f * DIAMETRO_RODA_MM / ENTRE_RODAS_MM; // graus/s por RPM
            const float taxaMax = sqrtf(2.0f * IMU_ACEL_ANGULAR_DESACEL_DPS2 * fmaxf(falta, 0));
            rpmBase = limitar(taxaMax / ganhoAngular, 17.0f, info.limiteRpm);
        }
        if (rpmBase <= 0) rpmBase = 17.0f;
        // Distancias NORMALIZADAS pelas CPRs individuais: mesma distancia
        // fisica percorrida em sentidos opostos => giro no centro do eixo.
        const float corr = limitar(GIRO_SYNC_K_RPM_POR_MM * info.syncErroMm,
                                  -GIRO_SYNC_CORRECAO_RPM_MAX, GIRO_SYNC_CORRECAO_RPM_MAX);
        info.syncCorrecaoRpm = corr;
        // Histerese evita alternar rapidamente freio/potencia.
        if (info.pausaSyncE && info.syncErroMm < GIRO_SYNC_RETOMADA_MM) info.pausaSyncE = false;
        if (info.pausaSyncD && info.syncErroMm > -GIRO_SYNC_RETOMADA_MM) info.pausaSyncD = false;
        if (info.syncErroMm > GIRO_SYNC_PAUSA_MM) info.pausaSyncE = true;
        if (info.syncErroMm < -GIRO_SYNC_PAUSA_MM) info.pausaSyncD = true;
        info.desejadoE = info.pausaSyncE ? 0 : sentidoGiro * fmaxf(16.0f, rpmBase - corr);
        info.desejadoD = info.pausaSyncD ? 0 : -sentidoGiro * fmaxf(16.0f, rpmBase + corr);
        // Não deixa o encoder ultrapassar o limite de curso no modo IMU.
        if (giroI) {
            if (viagemE > alvoFisicoMm * GIRO_LIMITE_DESLOCAMENTO_REL - 2.0f) info.desejadoE = 0;
            if (viagemD > alvoFisicoMm * GIRO_LIMITE_DESLOCAMENTO_REL - 2.0f) info.desejadoD = 0;
        }
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

    // A logica de POSICAO original continua intacta para movimentos normais.
    float passoMax = RPM_ACELERACAO_POR_SEG * dt;
    velocidadeAlvoE = aproximar(velocidadeAlvoE, info.desejadoE, passoMax);
    velocidadeAlvoD = aproximar(velocidadeAlvoD, info.desejadoD, passoMax);
    if (info.tipo == TipoControle::POSICAO && info.finalizouE) velocidadeAlvoE = 0;
    if (info.tipo == TipoControle::POSICAO && info.finalizouD) velocidadeAlvoD = 0;
    if (girando && info.pausaSyncE) velocidadeAlvoE = 0;
    if (girando && info.pausaSyncD) velocidadeAlvoD = 0;

    info.rampaE = velocidadeAlvoE;
    info.rampaD = velocidadeAlvoD;
    info.pwmE = acionarRoda(velocidadeAlvoE, info.realE, dt, estadoE, true,
                           info.tipo == TipoControle::POSICAO || girando, restanteE, agora);
    info.pwmD = acionarRoda(velocidadeAlvoD, info.realD, dt, estadoD, false,
                           info.tipo == TipoControle::POSICAO || girando, restanteD, agora);
    if (girando && info.pausaSyncE) info.pwmE = 0;
    if (girando && info.pausaSyncD) info.pwmD = 0;
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
