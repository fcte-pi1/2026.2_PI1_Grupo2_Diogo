#include "speed.h"
#include "speed_logic.h"
#include "config.h"
#include "encoders.h"

namespace {
DadosVelocidade dados;
VelocidadeMedia trecho, total;
uint32_t ultimoMs = 0;
int64_t ultimoE = 0, ultimoD = 0;
bool movendo = false;
}

void atualizarVelocidade(float cprE, float cprD, bool emMovimento) {
    const uint32_t agora = millis();
    if (agora - ultimoMs < AMOSTRA_ENCODER_MS) return;
    const uint32_t dtMs = agora - ultimoMs;
    ultimoMs = agora;

    const DadosEncoder enc = lerEncoders();
    const int64_t deltaE = enc.esq - ultimoE;
    const int64_t deltaD = enc.dir - ultimoD;
    ultimoE = enc.esq;
    ultimoD = enc.dir;

    dados.valida = cprE > 0 && cprD > 0;
    dados.rpmE = rpmDaRoda(enc.cpsEsq, cprE);
    dados.rpmD = rpmDaRoda(enc.cpsDir, cprD);
    dados.instantaneaCmS = dados.valida ? velocidadeCmS(dados.rpmE, dados.rpmD, DIAMETRO_RODA_MM) : 0.0f;

    // Cada teste com motor abre um trecho novo. A primeira amostra so marca o
    // ponto de partida, para nao contar o que aconteceu com o robo parado.
    const bool comecou = emMovimento && !movendo;
    movendo = emMovimento;
    if (comecou) trecho.reiniciar();
    if (emMovimento && !comecou && dados.valida) {
        const float deltaCentroMm = 0.5f * (distanciaRodaMm(deltaE, cprE, DIAMETRO_RODA_MM) +
                                            distanciaRodaMm(deltaD, cprD, DIAMETRO_RODA_MM));
        trecho.acumular(deltaCentroMm, dtMs);
        total.acumular(deltaCentroMm, dtMs);
    }
    dados.mediaTrechoCmS = trecho.mediaCmS();
    dados.distanciaTrechoMm = trecho.distanciaMm();
    dados.tempoTrechoMs = trecho.tempoMs();
    dados.mediaTotalCmS = total.mediaCmS();
    dados.distanciaTotalMm = total.distanciaMm();
    dados.tempoTotalMs = total.tempoMs();
}

DadosVelocidade lerVelocidade() { return dados; }
