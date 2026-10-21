#pragma once
#include <stdint.h>

// Lógica pura do teste de consumo por componente (sem Arduino, testável no PC)

// Acumula as leituras do INA219 durante uma etapa
struct EstatisticaEnergia {
  uint32_t n = 0;
  float somaV = 0.0f;
  float somaI = 0.0f;
  float minI = 0.0f;
  float maxI = 0.0f;

  void adicionar(float tensaoV, float correnteMA) {
    if (n == 0 || correnteMA < minI) minI = correnteMA;
    if (n == 0 || correnteMA > maxI) maxI = correnteMA;
    somaV += tensaoV;
    somaI += correnteMA;
    n++;
  }

  float tensaoMedia() const { return n ? somaV / n : 0.0f; }
  float correnteMedia() const { return n ? somaI / n : 0.0f; }
};

// Integra a carga (mAh) e a energia (mWh) tiradas da bateria durante um ensaio.
// Usa double porque em 15 min a 100 Hz são 90 mil parcelas pequenas.
struct IntegradorEnergia {
  double mAh = 0.0;
  double mWh = 0.0;
  uint32_t duracaoMs = 0;

  void adicionar(float tensaoV, float correnteMA, uint32_t dtMs) {
    mAh += (double)correnteMA * dtMs / 3600000.0;
    mWh += (double)tensaoV * correnteMA * dtMs / 3600000.0;
    duracaoMs += dtMs;
  }
};

// Converte a corrente puxada da bateria para a corrente no trilho do componente.
// O regulador entrega ao trilho rendimento x potência da bateria:
// I_trilho = I_bateria * V_bateria * rendimento / V_trilho
inline float correnteNoTrilho(float iBateriaMA, float vBateria, float vTrilho, float rendimento) {
  if (vTrilho <= 0.0f) return 0.0f;
  return iBateriaMA * vBateria * rendimento / vTrilho;
}
