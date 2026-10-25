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

class FiltroLaser {
public:
  explicit FiltroLaser(float alfa) : alfa(alfa) {}

  int atualizar(int leitura) {
    if (leitura < 0) {
      reiniciar();
      return leitura;
    }
    janela[proximo] = leitura;
    proximo = (proximo + 1) % 3;
    if (quantidade < 3) quantidade++;

    float mediana = medianaDaJanela();
    suave = temSuave ? alfa * mediana + (1.0f - alfa) * suave : mediana;
    temSuave = true;
    return (int)(suave + 0.5f);
  }

  void reiniciar() {
    quantidade = 0;
    proximo = 0;
    temSuave = false;
  }

private:
  float medianaDaJanela() const {
    if (quantidade == 1) return (float)janela[0];
    if (quantidade == 2) return (janela[0] + janela[1]) / 2.0f;
    int a = janela[0], b = janela[1], c = janela[2];
    int menor = a < b ? a : b;
    int maior = a < b ? b : a;
    int meio = c < maior ? c : maior;
    return (float)(menor > meio ? menor : meio);
  }

  float alfa;
  int janela[3] = {0, 0, 0};
  int quantidade = 0;
  int proximo = 0;
  float suave = 0.0f;
  bool temSuave = false;
};

enum EstadoParede { PAREDE_INDEFINIDA, PAREDE_AUSENTE, PAREDE_PRESENTE };

class DetectorParede {
public:
  DetectorParede(int limiarComMm, int limiarSemMm) : limiarCom(limiarComMm), limiarSem(limiarSemMm) {}

  EstadoParede atualizar(int leitura) {
    if (leitura == LASER_FORA_ESCALA) conhecido = PAREDE_AUSENTE;
    else if (leitura < 0) return PAREDE_INDEFINIDA;
    else if (leitura <= limiarCom) conhecido = PAREDE_PRESENTE;
    else if (leitura > limiarSem) conhecido = PAREDE_AUSENTE;
    else if (conhecido == PAREDE_INDEFINIDA) {
      conhecido = leitura <= (limiarCom + limiarSem) / 2 ? PAREDE_PRESENTE : PAREDE_AUSENTE;
    }
    return conhecido;
  }

private:
  int limiarCom;
  int limiarSem;
  EstadoParede conhecido = PAREDE_INDEFINIDA;
};

inline const char *textoParede(EstadoParede estado) {
  switch (estado) {
    case PAREDE_PRESENTE: return "SIM";
    case PAREDE_AUSENTE:  return "NAO";
    default:              return "?";
  }
}
