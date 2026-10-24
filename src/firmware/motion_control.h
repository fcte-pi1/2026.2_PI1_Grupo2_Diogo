#pragma once
#include <Arduino.h>
#include <stdint.h>

enum class TipoControle { INATIVO, RPM, POSICAO };

struct ControleInfo {
    TipoControle tipo = TipoControle::INATIVO;
    int64_t inicioE = 0, inicioD = 0;
    int32_t alvoE = 0, alvoD = 0; // deslocamentos relativos em contagens
    int64_t progressoE = 0, progressoD = 0;
    float limiteRpm = 0;
    float desejadoE = 0, desejadoD = 0;
    float realE = 0, realD = 0;
    int pwmE = 0, pwmD = 0;
    bool finalizouE = false, finalizouD = false;
};

// Iniciar apenas em repouso. Dados CPR devem estar validados e >0.
bool iniciarControleRpm(float rpmE, float rpmD, float cprE, float cprD);
bool iniciarControlePosicao(int32_t alvoE, int32_t alvoD,
                            float limiteRpm, float cprE, float cprD);
// Retorno: nullptr se normal; string de motivo se deve interromper/frear.
const char* atualizarControleMovimento();
void desativarControleMovimento();
ControleInfo lerControleMovimento();
