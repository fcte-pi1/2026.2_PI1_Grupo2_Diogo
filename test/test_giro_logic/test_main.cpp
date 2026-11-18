#include <unity.h>
#include "giro_logic.h"

void setUp() {}
void tearDown() {}

// pulsosAlvo 100, tolerância 2 pulsos; alvo 90 graus, tolerância 2 graus
static ParadaGiro avaliar(float pulsos, float angulo, bool valido = true) {
  return avaliarParadaGiro(pulsos, 100.0f, 2.0f, angulo, 90.0f, 2.0f, valido);
}

void test_no_inicio_continua_girando() {
  TEST_ASSERT_EQUAL(ParadaGiro::EmAndamento, avaliar(0.0f, 0.0f));
}

void test_para_por_pulsos_quando_o_angulo_ainda_nao_chegou() {
  TEST_ASSERT_EQUAL(ParadaGiro::Pulsos, avaliar(98.0f, 70.0f));
}

void test_para_por_angulo_se_a_roda_escorrega() {
  // Encoders contaram pouco, mas o giroscópio já viu 88 graus
  TEST_ASSERT_EQUAL(ParadaGiro::Angulo, avaliar(80.0f, 88.0f));
}

void test_angulo_funciona_nos_dois_sentidos() {
  TEST_ASSERT_EQUAL(ParadaGiro::Angulo, avaliar(80.0f, -88.5f));
}

void test_imu_invalida_decide_so_por_pulsos() {
  TEST_ASSERT_EQUAL(ParadaGiro::EmAndamento, avaliar(80.0f, 90.0f, false));
  TEST_ASSERT_EQUAL(ParadaGiro::Pulsos, avaliar(99.0f, 10.0f, false));
}

void test_justo_antes_da_tolerancia_continua() {
  TEST_ASSERT_EQUAL(ParadaGiro::EmAndamento, avaliar(97.5f, 87.5f));
}

void test_pulsos_tem_prioridade_se_os_dois_chegam_juntos() {
  TEST_ASSERT_EQUAL(ParadaGiro::Pulsos, avaliar(100.0f, 90.0f));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_no_inicio_continua_girando);
  RUN_TEST(test_para_por_pulsos_quando_o_angulo_ainda_nao_chegou);
  RUN_TEST(test_para_por_angulo_se_a_roda_escorrega);
  RUN_TEST(test_angulo_funciona_nos_dois_sentidos);
  RUN_TEST(test_imu_invalida_decide_so_por_pulsos);
  RUN_TEST(test_justo_antes_da_tolerancia_continua);
  RUN_TEST(test_pulsos_tem_prioridade_se_os_dois_chegam_juntos);
  return UNITY_END();
}
