#pragma once

#include <math.h>
#include <stdint.h>

// RPM da roda pelas contagens por segundo do encoder. Sem CPR calibrado, retorna 0
inline float rpmDaRoda(float contagensPorSegundo, float cpr) {
  return cpr > 0.0f ? contagensPorSegundo * 60.0f / cpr : 0.0f;
}

// Deslocamento da roda em mm para um número de contagens (com sinal)
inline float distanciaRodaMm(int64_t contagens, float cpr, float diametroRodaMm) {
  return cpr > 0.0f ? (float)contagens * 3.14159265f * diametroRodaMm / cpr : 0.0f;
}

// Velocidade linear do centro do robô em cm/s, pela média do RPM das duas rodas (com sinal).
// Girando no próprio eixo (rodas em sentidos opostos) o resultado é zero
inline float velocidadeCmS(float rpmE, float rpmD, float diametroRodaMm) {
  const float rpmMedio = 0.5f * (rpmE + rpmD);
  return rpmMedio * 3.14159265f * diametroRodaMm / 600.0f;  // mm/min para cm/s
}

// Velocidade média = distância percorrida pelo centro / tempo em movimento.
// A distância soma o módulo de cada trecho, então ir e voltar não se cancelam
class VelocidadeMedia {
public:
  void reiniciar() {
    distancia = 0.0f;
    tempo = 0;
  }

  void acumular(float deltaCentroMm, uint32_t dtMs) {
    distancia += fabsf(deltaCentroMm);
    tempo += dtMs;
  }

  float distanciaMm() const { return distancia; }
  uint32_t tempoMs() const { return tempo; }
  float mediaCmS() const { return tempo > 0 ? (distancia / 10.0f) / (tempo / 1000.0f) : 0.0f; }

private:
  float distancia = 0.0f;
  uint32_t tempo = 0;
};
