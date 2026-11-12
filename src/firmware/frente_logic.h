#pragma once

#include <stdint.h>
#include "lasers_logic.h"

// Proteção frontal do reto com centralização (#155): separa "sem alvo à frente"
// (corredor longo) de falha do sensor.

enum class DecisaoFrente : uint8_t { SEGUIR, PAREDE, INVALIDA };

inline const char* textoDecisaoFrente(DecisaoFrente d) {
  switch (d) {
    case DecisaoFrente::SEGUIR: return "SEGUIR";
    case DecisaoFrente::PAREDE: return "PAREDE";
    default: return "INVALIDA";
  }
}

// Só estes códigos indicam nada dentro do alcance; os demais (perto demais,
// timeout, erro de I2C, status 1 a 3) são falha e sempre freiam.
inline bool frenteSemAlvo(int leituraMm) {
  return leituraMm == LASER_FORA_ESCALA || leituraMm == LASER_FASE_INVALIDA;
}

class ProtecaoFrontal {
public:
  ProtecaoFrontal(uint32_t idadeMaxMs, uint8_t semAlvoParaPartir, uint32_t memoriaPartidaMs)
      : idadeMax(idadeMaxMs), semAlvoPartida(semAlvoParaPartir), memoria(memoriaPartidaMs) {}

  void reiniciar() { temEvento = temValida = false; semAlvoSeguidas = 0; }

  // Robô girou ou mudou de lugar: a parede vista antes não vale mais.
  void esquecerParede() { temValida = false; }

  // Chamar a cada passada. percursoMm: avanço do reto atual (0 parado).
  void observar(uint32_t amostra, int leituraMm, uint32_t agoraMs, float percursoMm) {
    if (temEvento && amostra == ultimaAmostra) return;
    temEvento = true;
    ultimaAmostra = amostra;
    eventoMs = agoraMs;
    if (leituraMm >= 0) {
      temValida = true;
      validaMm = (float)leituraMm;
      validaMs = agoraMs;
      percursoNaValida = percursoMm;
      semAlvoSeguidas = 0;
    } else if (frenteSemAlvo(leituraMm)) {
      if (semAlvoSeguidas < 255) ++semAlvoSeguidas;
    } else {
      semAlvoSeguidas = 0;
    }
  }

  // Parado desde a última leitura: a parede continua onde estava.
  void iniciarMovimento(uint32_t agoraMs) {
    if (temValida && (uint32_t)(agoraMs - validaMs) > memoria) temValida = false;
    percursoNaValida = 0;
  }

  // Distância da última parede vista, descontando o que o robô andou; < 0 = nenhuma.
  float paredeEstimadaMm(float percursoMm) const {
    return temValida ? validaMm - (percursoMm - percursoNaValida) : -1.0f;
  }

  DecisaoFrente decidir(int leituraMm, float limiarMm, uint32_t agoraMs, float percursoMm) const {
    if (!temEvento || (uint32_t)(agoraMs - eventoMs) > idadeMax) return DecisaoFrente::INVALIDA;
    if (leituraMm >= 0) return leituraMm <= limiarMm ? DecisaoFrente::PAREDE : DecisaoFrente::SEGUIR;
    if (!frenteSemAlvo(leituraMm)) return DecisaoFrente::INVALIDA;
    if (temValida && paredeEstimadaMm(percursoMm) <= limiarMm) return DecisaoFrente::PAREDE;
    return DecisaoFrente::SEGUIR;
  }

  bool podePartir(int leituraMm, float limiarMm, uint32_t agoraMs) const {
    if (!temEvento || (uint32_t)(agoraMs - eventoMs) > idadeMax) return false;
    if (leituraMm >= 0) return leituraMm > limiarMm;
    if (!frenteSemAlvo(leituraMm) || semAlvoSeguidas < semAlvoPartida) return false;
    return !(temValida && (uint32_t)(agoraMs - validaMs) <= memoria && validaMm <= limiarMm);
  }

  uint8_t semAlvoSeguidasAgora() const { return semAlvoSeguidas; }

private:
  uint32_t idadeMax;
  uint8_t semAlvoPartida;
  uint32_t memoria;
  bool temEvento = false;
  uint32_t ultimaAmostra = 0, eventoMs = 0;
  bool temValida = false;
  float validaMm = 0, percursoNaValida = 0;
  uint32_t validaMs = 0;
  uint8_t semAlvoSeguidas = 0;
};
