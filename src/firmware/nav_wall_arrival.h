#pragma once
#include <math.h>
// Classifica a parada provocada pela parede da frente. Nao e um novo limiar do
// laser: so interpreta a odometria depois que o robo ja parou.
//
// V5.0 - por que isto mudou
// A versao anterior exigia que a parada frontal caisse a menos de 0,7 mm da
// meta nominal da reta (RETO_META_TOL_MM) para a pose ser aceita. Uma parada
// disparada pelo laser acontece onde a parede esta, nunca onde a meta estava:
// na pratica a condicao era quase sempre falsa e TODA chegada contra a parede
// terminava em "obstaculo_frontal_antes_da_celula_pose_incerta" -> FAULT.
// Agora separamos duas perguntas independentes:
//   1) a parada foi limpa? (rodas sincronizadas, sem guinada)  -> pose util
//   2) o eixo chegou a cruzar a fronteira da celula?           -> trocou de celula
// Uma parada limpa que nao cruzou a fronteira nao e falha: o robo continua na
// mesma celula, com o deslocamento conhecido, e apenas replaneja.

// 1) A parada foi limpa o bastante para a odometria valer.
inline bool navWallArrivalPoseOk(float progress, float sync, float yaw,
                                 float syncTol, float yawTol) {
  return isfinite(progress) && progress > 0 && isfinite(sync) && isfinite(yaw) &&
         fabsf(sync) <= syncTol && fabsf(yaw) <= yawTol;
}
// 2) O eixo cruzou a fronteira entre as duas celulas?
// A fronteira esta a meiaAbertura a frente do CENTRO da celula de partida, e o
// eixo comecou a reta em offsetInicial em relacao a esse centro.
inline float navWallArrivalCrossingMm(float offsetInicial, float meiaAbertura) {
  return meiaAbertura - offsetInicial;
}
inline bool navWallArrivalCrossed(float progress, float offsetInicial, float meiaAbertura) {
  return isfinite(progress) && progress >= navWallArrivalCrossingMm(offsetInicial, meiaAbertura);
}
