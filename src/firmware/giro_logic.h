#pragma once

#include <math.h>

// Por que o giro terminou
enum class ParadaGiro {
  EmAndamento,
  Pulsos,  // As rodas andaram o necessário
  Angulo   // O giroscópio mediu o ângulo pedido
};

// O giro termina no que vier primeiro: pulsos dos encoders ou ângulo do giroscópio.
// Os pulsos protegem contra um giroscópio com defeito; o ângulo corrige a roda que escorrega,
// porque aí os pulsos contam mais do que o robô realmente girou.
// anguloValido é falso se a IMU não calibrou, e então só os pulsos decidem.
inline ParadaGiro avaliarParadaGiro(float pulsosMedios, float pulsosAlvo, float tolPulsos,
                                    float anguloGraus, float alvoGraus, float tolGraus,
                                    bool anguloValido) {
  if (pulsosAlvo - pulsosMedios <= tolPulsos) return ParadaGiro::Pulsos;
  if (anguloValido && fabsf(anguloGraus) >= alvoGraus - tolGraus) return ParadaGiro::Angulo;
  return ParadaGiro::EmAndamento;
}
