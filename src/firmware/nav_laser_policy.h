#pragma once
#include <stdint.h>
#include "lasers_logic.h"

// Evidencia para o mapa, sem alterar a aquisicao/filtragem do VL53L0X.
// IMPORTANTE: o modo 'sem alvo = livre' so deve ser habilitado em labirinto
// supervisionado com paredes ja ensaiadas. Sinal fraco nao comprova ausencia
// de uma parede escura, inclinada ou pouco refletiva.
enum class EvidenciaParedeNav : uint8_t { INDEFINIDA, LIVRE, PRESENTE };

class HistoricoLaserNav {
public:
    void reiniciar(uint32_t ultimoTotal) {
        ultimoTotal_ = ultimoTotal;
        semAlvoSeguidas_ = 0;
        viuParede_ = false;
    }
    void novaAmostra(uint32_t total, int brutoMm, uint8_t rangeStatus, EstadoParede parede) {
        if (total == ultimoTotal_) return;
        ultimoTotal_ = total;
        if (brutoMm >= 0) {
            semAlvoSeguidas_ = 0;
            if (parede == PAREDE_PRESENTE) viuParede_ = true;
        } else if (brutoMm == LASER_SINAL_FRACO && rangeStatus == 2 && !viuParede_) {
            if (semAlvoSeguidas_ < 255) ++semAlvoSeguidas_;
        } else {
            semAlvoSeguidas_ = 0;
        }
    }
    EvidenciaParedeNav classificar(int brutoMm, uint8_t rangeStatus, EstadoParede parede,
                                   bool eventoRecente, bool leituraValidaRecente,
                                   bool permitirSemAlvo, uint8_t amostrasMinimas) const {
        if (!eventoRecente) return EvidenciaParedeNav::INDEFINIDA;
        if (brutoMm >= 0 && leituraValidaRecente) {
            if (parede == PAREDE_PRESENTE) return EvidenciaParedeNav::PRESENTE;
            if (parede == PAREDE_AUSENTE) return EvidenciaParedeNav::LIVRE;
            return EvidenciaParedeNav::INDEFINIDA;
        }
        if (permitirSemAlvo && !viuParede_ && brutoMm == LASER_SINAL_FRACO &&
            rangeStatus == 2 && semAlvoSeguidas_ >= amostrasMinimas && amostrasMinimas > 0)
            return EvidenciaParedeNav::LIVRE;
        return EvidenciaParedeNav::INDEFINIDA;
    }
    uint8_t semAlvoSeguidas() const { return semAlvoSeguidas_; }
private:
    uint32_t ultimoTotal_ = 0;
    uint8_t semAlvoSeguidas_ = 0;
    bool viuParede_ = false;
};
