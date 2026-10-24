#pragma once

#include <stdint.h>

// Códigos negativos devolvidos no lugar da distância (mm >= 0 é leitura válida)
static const int LASER_SEM_LEITURA = -1;  // Sensor inativo ou leitura sem qualidade
static const int LASER_FORA_ESCALA = -2;  // Sem alvo ao alcance ou além do limite
static const int LASER_TIMEOUT     = -3;  // Falha de comunicação/tempo esgotado no I2C

// Valores do RangeStatus da biblioteca (0 = válido; os demais descrevem o motivo)
static const uint8_t LASER_STATUS_VALIDO = 0;
static const uint8_t LASER_STATUS_FASE   = 4;  // Phase Fail: sem alvo ao alcance

// A falha de comunicação vem antes do RangeStatus: com o I2C falho, o
// RangeStatus e a distância são lixo e não podem ser interpretados.
inline int classificarLeitura(bool erroComunicacao, uint8_t rangeStatus,
                              uint16_t distanciaMm, int minMm, int maxMm) {
  if (erroComunicacao) return LASER_TIMEOUT;
  if (rangeStatus == LASER_STATUS_FASE) return LASER_FORA_ESCALA;
  if (rangeStatus != LASER_STATUS_VALIDO) return LASER_SEM_LEITURA;
  if (distanciaMm > maxMm) return LASER_FORA_ESCALA;
  if (distanciaMm < minMm) return LASER_SEM_LEITURA;
  return distanciaMm;
}
