#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <math.h>

enum class TipoControle { INATIVO, RPM, POSICAO, GIRO_ENCODER, GIRO_IMU, RETO };

struct ControleInfo {
    TipoControle tipo = TipoControle::INATIVO;
    int64_t inicioE = 0, inicioD = 0;
    int32_t alvoE = 0, alvoD = 0; // deslocamentos relativos em contagens
    int64_t progressoE = 0, progressoD = 0;
    float limiteRpm = 0;
    float desejadoE = 0, desejadoD = 0;
    float realE = 0, realD = 0;
    float rampaE = 0, rampaD = 0; // referencia realmente usada no PI
    bool ajudaPartidaE = false, ajudaPartidaD = false;
    uint32_t semPulsosMsE = 0, semPulsosMsD = 0;
    int pwmE = 0, pwmD = 0;
    bool finalizouE = false, finalizouD = false;
    float syncErroMm = 0, distanciaE = 0, distanciaD = 0;
    float syncCorrecaoRpm = 0;
    float syncErroMaxMm = 0;
    bool pausaSyncE = false, pausaSyncD = false;
    float alvoYawGraus = 0;
    float yawNoInicio = 0;
    float yawMedido = 0;
    float erroYaw = 0;
    bool usandoIMU = false;
    bool sincronismoSuave = false; // V4.3: configuravel para A/B com V4.2
    // V4.5: deslocamento retilineo nominal do centro do eixo (encoders).
    float retoAlvoMm = 0;
    float retoProgressoMm = 0;
    float retoErroMm = 0;
    float retoDistEMm = 0, retoDistDMm = 0;
    float retoSyncErroMm = 0, retoSyncMaxMm = 0;
    float retoSyncCorrecaoRpm = 0;
    bool retoSyncAtivo = false;
    float retoYawInicio = 0;
    float retoYawRelativo = NAN; // diagnostico IMU; nao controla a direcao
    float retoYawEncoder = 0;
    float retoOdomXmm = 0, retoOdomYmm = 0;
    // Estimativas somente por encoder; NAO medem escorregamento lateral.
    float odomXmm = 0, odomYmm = 0;
    float odomYawGraus = 0;
};

// Iniciar apenas em repouso. Dados CPR devem estar validados e >0.
bool iniciarControleRpm(float rpmE, float rpmD, float cprE, float cprD);
bool iniciarControlePosicao(int32_t alvoE, int32_t alvoD,
                            float limiteRpm, float cprE, float cprD);
// V4.5: signed mm; positivo = frente, negativo = re. Ambos os motores
// executam o movimento, e o sync suave e opcional para comparar A/B.
// IMU e lasers sao APENAS diagnostico: nao entram na malha.
bool iniciarControleReto(float distanciaMm, float limiteRpm, float cprE,
                         float cprD, bool sincronismoSuave);
// Retorno: nullptr se normal; string de motivo se deve interromper/frear.
const char* atualizarControleMovimento();
void desativarControleMovimento();
ControleInfo lerControleMovimento();

// Giro positivo = direita, negativo = esquerda; angulo fixo +/-90 deg.
bool iniciarControleGiro(bool direita, bool usaImu, float limiteRpm,
                         float cprE, float cprD, bool sincronismoSuave);
