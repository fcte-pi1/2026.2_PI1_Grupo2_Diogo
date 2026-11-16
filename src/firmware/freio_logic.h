#pragma once

#include <stdint.h>
#include <string.h>

// Freio ativo no fim do movimento (#163).

// Fim normal e paradas pelo laser frontal freiam; emergência, falha e motivo
// desconhecido cortam por STBY. limite_de_tempo só é fim normal em PWM e RPM.
inline bool paradaUsaFreioAtivo(const char* motivo, bool testeTemporizado) {
  if (!motivo) return false;
  static const char* const FREIA[] = {
      "posicao_meta_atingida",
      "reto_meta_atingida",
      "reto_chegada_com_dessincronia",
      "giro_encoder_concluido",
      "giro_imu_concluido",
      "giro_fim_com_dessincronia",
      "imu_alvo_com_dessincronia",
      "giro_dentro_da_tolerancia",
      "giro_corrigido_dentro_da_tolerancia",
      "giro_correcoes_esgotadas",
      "frontal_U3_obstaculo_detectado",
      "frontal_U3_parede_estimada_pelo_percurso",
      "frontal_U3_leitura_invalida_ou_antiga",
  };
  for (const char* m : FREIA)
    if (strcmp(motivo, m) == 0) return true;
  return testeTemporizado && strcmp(motivo, "limite_de_tempo") == 0;
}

// Solta o freio quando as rodas ficam paradas por um tempo ou no tempo máximo.
class LiberacaoFreio {
public:
  LiberacaoFreio(uint32_t paradoMs, uint32_t maximoMs) : parado(paradoMs), maximo(maximoMs) {}

  void iniciar(uint32_t agoraMs, int64_t contE, int64_t contD) {
    inicio = desde = agoraMs;
    ultimaE = contE;
    ultimaD = contD;
  }

  bool podeSoltar(uint32_t agoraMs, int64_t contE, int64_t contD) {
    if (contE != ultimaE || contD != ultimaD) {
      ultimaE = contE;
      ultimaD = contD;
      desde = agoraMs;
    }
    return (uint32_t)(agoraMs - desde) >= parado || (uint32_t)(agoraMs - inicio) >= maximo;
  }

private:
  uint32_t parado, maximo;
  uint32_t inicio = 0, desde = 0;
  int64_t ultimaE = 0, ultimaD = 0;
};
