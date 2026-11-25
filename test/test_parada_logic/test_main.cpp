#include <unity.h>
#include "parada_logic.h"

// Parada confirmada entre movimentos da missão.

void setUp() {}
void tearDown() {}

static ParadaConfirmada nova() {
  ParadaConfirmada p(150, 1000, 1.0f);
  p.iniciar(1000, 10, 10);
  return p;
}

void test_confirma_parado_pelo_tempo() {
  ParadaConfirmada p = nova();
  p.atualizar(1000, 10, 10, true, 0.1f);
  p.atualizar(1149, 10, 10, true, 0.1f);
  TEST_ASSERT_FALSE(p.ok());
  p.atualizar(1150, 10, 10, true, 0.1f);
  TEST_ASSERT_TRUE(p.ok());
}

void test_roda_deslizando_adia() {
  ParadaConfirmada p = nova();
  p.atualizar(1000, 10, 10, true, 0.1f);
  p.atualizar(1140, 11, 10, true, 0.1f);
  p.atualizar(1200, 11, 10, true, 0.1f);
  TEST_ASSERT_FALSE(p.ok());
  p.atualizar(1290, 11, 10, true, 0.1f);
  TEST_ASSERT_TRUE(p.ok());
}

void test_chassi_girando_com_rodas_paradas_adia() {
  ParadaConfirmada p = nova();
  for (uint32_t t = 1000; t <= 1400; t += 5) p.atualizar(t, 10, 10, true, 5.0f);
  TEST_ASSERT_FALSE(p.ok());
}

void test_sem_imu_nao_confirma() {
  ParadaConfirmada p = nova();
  for (uint32_t t = 1000; t <= 1400; t += 5) p.atualizar(t, 10, 10, false, 0.0f);
  TEST_ASSERT_FALSE(p.ok());
}

void test_movimento_depois_de_confirmar_desfaz() {
  ParadaConfirmada p = nova();
  for (uint32_t t = 1000; t <= 1200; t += 5) p.atualizar(t, 10, 10, true, 0.0f);
  TEST_ASSERT_TRUE(p.ok());
  p.atualizar(1205, 12, 10, true, 0.0f);
  TEST_ASSERT_FALSE(p.ok());
}

void test_novo_inicio_zera() {
  ParadaConfirmada p = nova();
  for (uint32_t t = 1000; t <= 1200; t += 5) p.atualizar(t, 10, 10, true, 0.0f);
  p.iniciar(1300, 20, 20);
  TEST_ASSERT_FALSE(p.ok());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_confirma_parado_pelo_tempo);
  RUN_TEST(test_roda_deslizando_adia);
  RUN_TEST(test_chassi_girando_com_rodas_paradas_adia);
  RUN_TEST(test_sem_imu_nao_confirma);
  RUN_TEST(test_movimento_depois_de_confirmar_desfaz);
  RUN_TEST(test_novo_inicio_zera);
  return UNITY_END();
}
