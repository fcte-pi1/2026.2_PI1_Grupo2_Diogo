#include <unity.h>
#include "lasers_logic.h"

void setUp() {}
void tearDown() {}

void test_offset_e_subtraido_da_leitura() {
  TEST_ASSERT_EQUAL_INT(120, aplicarOffsetLaser(140, 20));
  TEST_ASSERT_EQUAL_INT(85, aplicarOffsetLaser(100, 15));
}

void test_offset_zero_nao_muda_a_leitura() {
  TEST_ASSERT_EQUAL_INT(140, aplicarOffsetLaser(140, 0));
}

void test_codigos_de_erro_passam_sem_offset() {
  const int codigos[] = {LASER_SEM_LEITURA, LASER_FORA_ESCALA, LASER_TIMEOUT,
                         LASER_FASE_INVALIDA, LASER_PERTO_DEMAIS, LASER_ERRO_COMUNICACAO};
  for (int codigo : codigos) TEST_ASSERT_EQUAL_INT(codigo, aplicarOffsetLaser(codigo, 20));
}

void test_leitura_corrigida_nunca_fica_negativa() {
  // Um valor negativo seria lido como codigo de erro pelo filtro e pelo detector
  TEST_ASSERT_EQUAL_INT(0, aplicarOffsetLaser(20, 20));
  TEST_ASSERT_EQUAL_INT(0, aplicarOffsetLaser(12, 20));
}

void test_offset_entra_antes_da_deteccao_de_parede() {
  // Parede real a 115 mm lida como 135 mm: sem offset fica entre os limiares
  DetectorParede semOffset(120, 160), comOffset(120, 160);
  semOffset.atualizar(170);  // ultimo estado conhecido: sem parede
  comOffset.atualizar(aplicarOffsetLaser(170, 20));
  TEST_ASSERT_EQUAL(PAREDE_AUSENTE, semOffset.atualizar(135));
  TEST_ASSERT_EQUAL(PAREDE_PRESENTE, comOffset.atualizar(aplicarOffsetLaser(135, 20)));
}

void test_leitura_invalida_continua_invalida_no_filtro() {
  FiltroLaser filtro(0.5f);
  TEST_ASSERT_EQUAL_INT(100, filtro.atualizar(aplicarOffsetLaser(120, 20)));
  TEST_ASSERT_EQUAL_INT(LASER_TIMEOUT, filtro.atualizar(aplicarOffsetLaser(LASER_TIMEOUT, 20)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_offset_e_subtraido_da_leitura);
  RUN_TEST(test_offset_zero_nao_muda_a_leitura);
  RUN_TEST(test_codigos_de_erro_passam_sem_offset);
  RUN_TEST(test_leitura_corrigida_nunca_fica_negativa);
  RUN_TEST(test_offset_entra_antes_da_deteccao_de_parede);
  RUN_TEST(test_leitura_invalida_continua_invalida_no_filtro);
  return UNITY_END();
}
