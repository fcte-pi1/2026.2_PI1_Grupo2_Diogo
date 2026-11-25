#pragma once

#include <stdint.h>
#include "freio_logic.h"
#include "giro_logic.h"

// Robô parado de verdade: rodas sem pulsos e chassi sem girar por um tempo seguido.
class ParadaConfirmada {
public:
  ParadaConfirmada(uint32_t paradoMs, uint32_t maximoMs, float limiteDps)
      : rodas(paradoMs, maximoMs), chassi(limiteDps, paradoMs) {}

  void iniciar(uint32_t agoraMs, int64_t contE, int64_t contD) {
    rodas.iniciar(agoraMs, contE, contD);
    chassi.reiniciar();
    confirmada = false;
  }

  void atualizar(uint32_t agoraMs, int64_t contE, int64_t contD, bool imuConfiavel, float taxaDps) {
    const bool rodasOk = rodas.podeSoltar(agoraMs, contE, contD);
    if (!imuConfiavel) {
      chassi.reiniciar();
      confirmada = false;
      return;
    }
    const bool chassiOk = chassi.atualizar(taxaDps, agoraMs);
    confirmada = rodasOk && chassiOk;
  }

  bool ok() const { return confirmada; }

private:
  LiberacaoFreio rodas;
  DetectorParado chassi;
  bool confirmada = false;
};
