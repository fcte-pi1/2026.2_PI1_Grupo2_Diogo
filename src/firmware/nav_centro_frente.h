#pragma once
#include <math.h>

// Correção da posição de parada pelo laser frontal (V4.10).
// Parado com parede à frente, o U3 deve ler alvoU3Mm. Retorna o deslocamento em mm (positivo =
// para frente) que leva a leitura até lá, 0 se já está dentro da tolerância, ou NAN se NÃO se
// deve corrigir:
//  - alvoU3Mm negativo: função desligada;
//  - no alvo o nariz ficaria a menos de folgaNarizMm da parede;
//  - leitura inválida ou com erro acima do máximo (pode não ser a parede desta célula);
//  - correção para trás sem a célula de trás comprovadamente livre (não há sensor traseiro).
// O alvo NÃO é "eixo no centro da célula": com o nariz bem à frente do eixo isso pode colocar
// o nariz dentro da parede. É a leitura em que o giro de 90 graus passa livre, ajustada por ensaio.
inline float navCorrecaoCentroFrente(float leituraU3Mm, float alvoU3Mm, float u3AteNarizMm,
                                     float folgaNarizMm, float minMm, float maxMm,
                                     bool podeRecuar) {
  if (!isfinite(leituraU3Mm) || !isfinite(alvoU3Mm) || !isfinite(u3AteNarizMm) ||
      !isfinite(folgaNarizMm))
    return NAN;
  if (alvoU3Mm < 0.0f || leituraU3Mm <= 0.0f) return NAN;
  if (alvoU3Mm - u3AteNarizMm < folgaNarizMm) return NAN;  // nariz perto demais da parede
  const float erro = leituraU3Mm - alvoU3Mm;                // positivo: ainda falta andar
  if (fabsf(erro) > maxMm) return NAN;
  if (fabsf(erro) < minMm) return 0.0f;
  if (erro < 0.0f && !podeRecuar) return NAN;
  return erro;
}
