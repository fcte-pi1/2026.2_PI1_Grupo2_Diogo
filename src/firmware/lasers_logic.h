#pragma once

#include <stdint.h>

static const int LASER_SEM_LEITURA = -1;
static const int LASER_FORA_ESCALA = -2;
static const int LASER_TIMEOUT = -3;

static const uint8_t LASER_STATUS_VALIDO = 0;
static const uint8_t LASER_STATUS_FASE = 4;

inline int classificarLeitura(bool erroComunicacao, uint8_t rangeStatus, uint16_t distanciaMm, int minMm, int maxMm) {
  if (erroComunicacao) return LASER_TIMEOUT;
  if (rangeStatus == LASER_STATUS_FASE) return LASER_FORA_ESCALA;
  if (rangeStatus != LASER_STATUS_VALIDO) return LASER_SEM_LEITURA;
  if (distanciaMm > maxMm) return LASER_FORA_ESCALA;
  if (distanciaMm < minMm) return LASER_SEM_LEITURA;
  return distanciaMm;
}

inline uint16_t periodoMedicaoMs(uint32_t orcamentoUs, uint16_t margemMs) {
  return (uint16_t)((orcamentoUs + 999u) / 1000u + margemMs);
}

inline uint32_t validadeLeituraMs(uint16_t periodoMs, uint8_t ciclos) {
  return (uint32_t)periodoMs * ciclos;
}

inline bool leituraVencida(uint32_t agoraMs, uint32_t ultimaMs, uint32_t validadeMs) {
  return (agoraMs - ultimaMs) > validadeMs;
}
