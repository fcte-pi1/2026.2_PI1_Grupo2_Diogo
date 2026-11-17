#pragma once
#include <math.h>
// Nao e novo limiar do laser. Classifica apenas a odometria apos parada.
// Uma interrupcao precoce nunca vira chegada nem autoriza re automaticamente.
inline bool navWallArrivalAtTarget(float progress,float target,float sync,float yaw,
                                   float posTol,float syncTol,float yawTol) {
  return isfinite(progress)&&isfinite(target)&&target>0&&progress>0&&
         isfinite(sync)&&isfinite(yaw)&&fabsf(progress-target)<=posTol&&
         fabsf(sync)<=syncTol&&fabsf(yaw)<=yawTol;
}
