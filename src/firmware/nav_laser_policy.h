#pragma once
#include <stdint.h>
#include "lasers_logic.h"

// Evidencia para o mapa, sem alterar a aquisicao/filtragem do VL53L0X.
// V4.9.1: no labirinto, uma parede da propria celula fica a poucos centimetros
// do sensor e sempre devolve uma distancia valida. Quando o VL53L0X olha para
// um corredor aberto ele costuma devolver "sem alvo" (sinal fraco, falha de
// fase ou fora de escala) em vez de uma distancia. Antes isso ficava
// INDEFINIDO para sempre e o robo nunca confirmava passagem nenhuma.
// Agora "sem alvo" repetido (sem nenhuma parede vista na janela) conta como
// passagem LIVRE. Erro de I2C, timeout e leitura invalida continuam INDEFINIDOS.
enum class EvidenciaParedeNav : uint8_t { INDEFINIDA, LIVRE, PRESENTE };

// Codigos que significam "nao ha superficie ao alcance", e nao falha do sensor.
inline bool laserSemAlvoNav(int brutoMm) {
    return brutoMm == LASER_SINAL_FRACO || brutoMm == LASER_FASE_INVALIDA ||
           brutoMm == LASER_FORA_ESCALA;
}

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
        (void)rangeStatus;
        if (brutoMm >= 0) {
            semAlvoSeguidas_ = 0;
            if (parede == PAREDE_PRESENTE) viuParede_ = true;
        } else if (brutoMm == LASER_PERTO_DEMAIS) {
            // Abaixo do minimo do sensor: ha algo encostado, e parede.
            semAlvoSeguidas_ = 0;
            viuParede_ = true;
        } else if (laserSemAlvoNav(brutoMm) && !viuParede_) {
            if (semAlvoSeguidas_ < 255) ++semAlvoSeguidas_;
        } else {
            semAlvoSeguidas_ = 0;
        }
    }
    EvidenciaParedeNav classificar(int brutoMm, uint8_t rangeStatus, EstadoParede parede,
                                   bool eventoRecente, bool leituraValidaRecente,
                                   bool permitirSemAlvo, uint8_t amostrasMinimas) const {
        (void)rangeStatus;
        if (!eventoRecente) return EvidenciaParedeNav::INDEFINIDA;
        if (brutoMm == LASER_PERTO_DEMAIS) return EvidenciaParedeNav::PRESENTE;
        if (brutoMm >= 0 && leituraValidaRecente) {
            if (parede == PAREDE_PRESENTE) return EvidenciaParedeNav::PRESENTE;
            if (parede == PAREDE_AUSENTE) return EvidenciaParedeNav::LIVRE;
            return EvidenciaParedeNav::INDEFINIDA;
        }
        if (permitirSemAlvo && !viuParede_ && laserSemAlvoNav(brutoMm) &&
            semAlvoSeguidas_ >= amostrasMinimas && amostrasMinimas > 0)
            return EvidenciaParedeNav::LIVRE;
        return EvidenciaParedeNav::INDEFINIDA;
    }
    uint8_t semAlvoSeguidas() const { return semAlvoSeguidas_; }
private:
    uint32_t ultimoTotal_ = 0;
    uint8_t semAlvoSeguidas_ = 0;
    bool viuParede_ = false;
};
