#pragma once

#include <math.h>
#include <stdint.h>

// Partida conjunta das duas rodas (#152).

inline bool rodaPartiu(int64_t progresso, int64_t pulsosMin) {
  return (progresso < 0 ? -progresso : progresso) >= pulsosMin;
}

// Abaixo do piso não mexe: a roda precisa de RPM para pedir a ajuda de partida.
inline float segurarRampaPartida(float referenciaRpm, float pisoRpm) {
  if (fabsf(referenciaRpm) <= pisoRpm) return referenciaRpm;
  return referenciaRpm > 0 ? pisoRpm : -pisoRpm;
}

// A roda adiantada não recebe a ajuda da outra, senão dispara quando a outra está presa.
inline bool ajudaPartidaConjunta(bool precisaPropria, bool precisaOutra,
                                 float adiantamentoMm, float limiteMm) {
  return precisaPropria || (precisaOutra && adiantamentoMm <= limiteMm);
}

struct DadosPartida {
  int32_t partidaEMs = -1;      // -1 = não saiu
  int32_t partidaDMs = -1;
  float diferencaMm = 0;        // E - D no fim da janela
  float diferencaMaxMm = 0;
  bool janelaCompleta = false;

  bool duasPartiram() const { return partidaEMs >= 0 && partidaDMs >= 0; }
  // Positivo: a esquerda saiu antes.
  int32_t atrasoMs() const { return duasPartiram() ? partidaDMs - partidaEMs : 0; }
};

class RegistroPartida {
public:
  explicit RegistroPartida(float janelaMm) : janela(janelaMm) {}

  void reiniciar(uint32_t inicioMs) {
    dados = DadosPartida{};
    inicio = inicioMs;
  }

  // cursoE/cursoD em mm, positivos no sentido comandado.
  void atualizar(float cursoE, float cursoD, bool partiuE, bool partiuD, uint32_t agoraMs) {
    if (partiuE && dados.partidaEMs < 0) dados.partidaEMs = (int32_t)(agoraMs - inicio);
    if (partiuD && dados.partidaDMs < 0) dados.partidaDMs = (int32_t)(agoraMs - inicio);
    if (dados.janelaCompleta) return;
    if (0.5f * (cursoE + cursoD) > janela) {
      dados.janelaCompleta = true;
      return;
    }
    dados.diferencaMm = cursoE - cursoD;
    dados.diferencaMaxMm = fmaxf(dados.diferencaMaxMm, fabsf(dados.diferencaMm));
  }

  const DadosPartida& ler() const { return dados; }

private:
  float janela;
  uint32_t inicio = 0;
  DadosPartida dados;
};
