#pragma once
#include <math.h>

// Correção do centro da célula pelo laser frontal (V4.10).
// O eixo das rodas é o ponto que precisa ficar no centro da célula, porque é em volta dele
// que o robô gira. Parado com parede à frente, o U3 deve ler (meia célula - u3AoEixo).
// Retorna o deslocamento em mm (positivo = para frente) que leva o eixo até lá,
// 0 se já está dentro da tolerância, ou NAN se NÃO se deve corrigir:
//  - u3AoEixo negativo: distância ainda não medida no robô, função desligada;
//  - geometria impossível: no alvo o nariz ficaria a menos de folgaNariz da parede;
//  - leitura inválida ou fora da faixa esperada (pode não ser a parede desta célula);
//  - correção para trás sem a célula de trás comprovadamente livre (não há sensor traseiro).
inline float navCorrecaoCentroFrente(float leituraU3Mm, float u3AoEixoMm, float meiaCelulaMm,
                                     float u3AteNarizMm, float folgaNarizMm,
                                     float minMm, float maxMm, bool podeRecuar) {
  if (!isfinite(leituraU3Mm) || !isfinite(u3AoEixoMm) || !isfinite(meiaCelulaMm) ||
      !isfinite(u3AteNarizMm) || !isfinite(folgaNarizMm))
    return NAN;
  if (u3AoEixoMm < 0.0f || meiaCelulaMm <= 0.0f || leituraU3Mm <= 0.0f) return NAN;
  const float alvo = meiaCelulaMm - u3AoEixoMm;
  if (alvo - u3AteNarizMm < folgaNarizMm) return NAN;  // nariz bateria na parede no alvo
  const float erro = leituraU3Mm - alvo;               // positivo: ainda falta andar
  if (fabsf(erro) > maxMm) return NAN;
  if (fabsf(erro) < minMm) return 0.0f;
  if (erro < 0.0f && !podeRecuar) return NAN;
  return erro;
}
