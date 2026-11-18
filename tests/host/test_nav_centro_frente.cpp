#include <cstdio>
#include <cmath>
#include "nav_centro_frente.h"

static int falhas = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FALHOU: %s (linha %d)\n", msg, __LINE__); falhas++; } else printf("ok   %s\n", msg); } while (0)

// Nariz a 60 mm do U3, folga minima 12 mm, corrige entre 4 e 35 mm. Alvo padrao dos testes: 85 mm.
static float corr(float leitura, float alvo = 85.0f, bool recuar = false) {
  return navCorrecaoCentroFrente(leitura, alvo, 60.0f, 12.0f, 4.0f, 35.0f, recuar);
}

int main() {
  CHECK(std::fabs(corr(110.0f) - 25.0f) < 1e-4, "parou curto: anda para frente a diferenca");
  CHECK(corr(86.0f) == 0.0f, "dentro da tolerancia: nao mexe");
  CHECK(corr(89.0f) == 4.0f, "exatamente no minimo: corrige");
  CHECK(std::isnan(corr(75.0f)), "passou do alvo sem poder recuar: nao corrige");
  CHECK(std::fabs(corr(75.0f, 85.0f, true) - (-10.0f)) < 1e-4, "passou do alvo com celula de tras livre: recua");
  CHECK(std::isnan(corr(130.0f)), "erro acima do maximo: leitura nao e desta parede");
  CHECK(std::isnan(corr(110.0f, -1.0f)), "alvo negativo: desligado");
  CHECK(std::isnan(corr(110.0f, 70.0f)), "alvo com nariz a 10 mm da parede (< folga 12): nunca arma");
  CHECK(!std::isnan(corr(95.0f, 72.0f)), "alvo com nariz exatamente na folga minima: arma");
  CHECK(std::isnan(corr(NAN)), "leitura invalida");
  CHECK(std::isnan(corr(-1.0f)), "codigo de erro do laser");
  CHECK(std::isnan(corr(0.0f)), "leitura zero");
  // Aplicar a correcao devolve exatamente o alvo, e o nariz nunca fica abaixo da folga
  for (float l = 50.0f; l <= 125.0f; l += 1.0f) {
    float c = corr(l, 85.0f, true);
    if (!std::isfinite(c) || c == 0.0f) continue;
    float final = l - c;
    if (std::fabs(final - 85.0f) > 1e-3 || final - 60.0f < 12.0f) { printf("FALHOU: final=%f p/ leitura %f\n", final, l); falhas++; }
  }
  puts(falhas ? "FALHOU" : "PASS: correcao da parada pelo laser frontal");
  return falhas ? 1 : 0;
}
