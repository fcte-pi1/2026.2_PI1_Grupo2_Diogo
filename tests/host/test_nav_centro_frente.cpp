#include <cstdio>
#include <cmath>
#include "nav_centro_frente.h"

static int falhas = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FALHOU: %s (linha %d)\n", msg, __LINE__); falhas++; } else printf("ok   %s\n", msg); } while (0)

// Celula de 180 (meia = 90), U3 a 10 mm a frente do eixo, 60 mm ate o nariz: alvo = 80 mm
static float corr(float leitura, float aEixo = 10.0f, bool recuar = false) {
  return navCorrecaoCentroFrente(leitura, aEixo, 90.0f, 60.0f, 12.0f, 4.0f, 35.0f, recuar);
}

int main() {
  CHECK(std::fabs(corr(100.0f) - 20.0f) < 1e-4, "parou curto: anda para frente a diferenca");
  CHECK(corr(81.0f) == 0.0f, "dentro da tolerancia: nao mexe");
  CHECK(corr(84.0f) == 4.0f, "exatamente no minimo: corrige");
  CHECK(std::isnan(corr(70.0f)), "passou do centro sem poder recuar: nao corrige");
  CHECK(std::fabs(corr(70.0f, 10.0f, true) - (-10.0f)) < 1e-4, "passou do centro com celula de tras livre: recua");
  CHECK(std::isnan(corr(130.0f)), "erro acima do maximo: leitura nao e desta parede");
  CHECK(std::isnan(corr(100.0f, -1.0f)), "U3 ao eixo nao medido: desligado");
  CHECK(std::isnan(corr(100.0f, 25.0f)), "geometria impossivel (nariz a 5 mm da parede no alvo): nunca arma");
  CHECK(std::isnan(corr(NAN)), "leitura invalida");
  CHECK(std::isnan(corr(-1.0f)), "codigo de erro do laser");
  CHECK(std::isnan(corr(0.0f)), "leitura zero");
  // Nunca manda andar alem do alvo: ao aplicar a correcao a leitura final e o alvo
  for (float l = 50.0f; l <= 115.0f; l += 1.0f) {
    float c = corr(l, 10.0f, true);
    if (std::isfinite(c)) { float final = l - c; if (c != 0.0f && std::fabs(final - 80.0f) > 1e-3) { printf("FALHOU: final=%f p/ leitura %f\n", final, l); falhas++; } }
  }
  puts(falhas ? "FALHOU" : "PASS: correcao do centro pelo laser frontal");
  return falhas ? 1 : 0;
}
