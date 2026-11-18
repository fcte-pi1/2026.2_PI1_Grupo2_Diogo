#pragma once

#include <math.h>
#include <stdint.h>

// Robô parado: taxa angular abaixo do limite por um tempo seguido.
// Serve para medir o ângulo final só depois que o robô terminou de girar por inércia
class DetectorParado {
public:
  DetectorParado(float limiteDps, uint32_t tempoMs) : limite(limiteDps), tempo(tempoMs) {}

  void reiniciar() { contando = false; }

  bool atualizar(float taxaDps, uint32_t agoraMs) {
    if (fabsf(taxaDps) > limite) {
      contando = false;
      return false;
    }
    if (!contando) {
      contando = true;
      desde = agoraMs;
    }
    return (uint32_t)(agoraMs - desde) >= tempo;
  }

private:
  float limite;
  uint32_t tempo;
  bool contando = false;
  uint32_t desde = 0;
};

// V5.2: trilha efetiva = distancia entre rodas que faz a conta do encoder
// bater com o giro REAL medido pela IMU (inclui o escorregamento do piso).
// arco = pi * trilha * angulo / 360   =>   trilha = arco * 360 / (pi * angulo)
inline float trilhaEfetivaMedidaMm(float arcoMedioMm, float anguloRealGraus) {
  if (!isfinite(arcoMedioMm) || !isfinite(anguloRealGraus) || fabsf(anguloRealGraus) < 1.0f)
    return NAN;
  return arcoMedioMm * 360.0f / (3.14159265f * fabsf(anguloRealGraus));
}
// Media exponencial; medida fora da faixa fisica e ignorada (mantem a atual).
inline float atualizarTrilhaEfetiva(float atualMm, float medidaMm, float alfa,
                                    float minMm, float maxMm) {
  if (!isfinite(medidaMm) || medidaMm < minMm || medidaMm > maxMm) return atualMm;
  if (!isfinite(atualMm)) return medidaMm;
  return atualMm + alfa * (medidaMm - atualMm);
}

enum class DecisaoCorrecao {
  DENTRO_DA_TOLERANCIA,   // não precisa corrigir
  CORRIGIR,               // girar o erro medido
  ERRO_GRANDE_DEMAIS,     // algo deu errado no giro: não tenta consertar sozinho
  TENTATIVAS_ESGOTADAS,
};

// erroGraus = alvo - medido, com o robô parado (positivo = falta girar para a direita)
inline DecisaoCorrecao decidirCorrecaoGiro(float erroGraus, float toleranciaGraus, float erroMaxGraus,
                                           uint8_t correcoesFeitas, uint8_t maxCorrecoes) {
  const float erro = fabsf(erroGraus);
  if (erro <= toleranciaGraus) return DecisaoCorrecao::DENTRO_DA_TOLERANCIA;
  if (erro > erroMaxGraus) return DecisaoCorrecao::ERRO_GRANDE_DEMAIS;
  if (correcoesFeitas >= maxCorrecoes) return DecisaoCorrecao::TENTATIVAS_ESGOTADAS;
  return DecisaoCorrecao::CORRIGIR;
}
