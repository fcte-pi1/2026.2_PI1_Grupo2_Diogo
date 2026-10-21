#include <unity.h>
#include "testes/energia_logic.h"

void setUp() {}
void tearDown() {}

void test_media_minimo_e_maximo() {
  EstatisticaEnergia e;
  e.adicionar(7.80f, 150.0f);
  e.adicionar(7.70f, 170.0f);
  e.adicionar(7.75f, 160.0f);
  TEST_ASSERT_EQUAL_UINT32(3, e.n);
  TEST_ASSERT_FLOAT_WITHIN(1e-4f, 7.75f, e.tensaoMedia());
  TEST_ASSERT_FLOAT_WITHIN(1e-3f, 160.0f, e.correnteMedia());
  TEST_ASSERT_EQUAL_FLOAT(150.0f, e.minI);
  TEST_ASSERT_EQUAL_FLOAT(170.0f, e.maxI);
}

void test_minimo_com_corrente_negativa() {
  // Ruído do shunt perto de zero pode dar corrente negativa
  EstatisticaEnergia e;
  e.adicionar(7.8f, 2.0f);
  e.adicionar(7.8f, -1.5f);
  TEST_ASSERT_EQUAL_FLOAT(-1.5f, e.minI);
  TEST_ASSERT_EQUAL_FLOAT(2.0f, e.maxI);
}

void test_sem_leituras_retorna_zero() {
  EstatisticaEnergia e;
  TEST_ASSERT_EQUAL_FLOAT(0.0f, e.tensaoMedia());
  TEST_ASSERT_EQUAL_FLOAT(0.0f, e.correnteMedia());
}

void test_conversao_para_o_trilho() {
  // 20 mA a 3,3 V com rendimento de 80% puxam 11,15 mA de uma bateria a 7,4 V
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, correnteNoTrilho(11.149f, 7.4f, 3.3f, 0.80f));
  // Motor: 350 mA a 6 V com 80% puxam 354,7 mA a 7,4 V
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 350.0f, correnteNoTrilho(354.73f, 7.4f, 6.0f, 0.80f));
}

void test_trilho_invalido_nao_divide_por_zero() {
  TEST_ASSERT_EQUAL_FLOAT(0.0f, correnteNoTrilho(100.0f, 7.4f, 0.0f, 0.8f));
}

void test_integrador_uma_hora_a_1A() {
  // 1000 mA por 1 h em passos de 10 ms (como no firmware) = 1000 mAh; a 7,4 V = 7400 mWh
  IntegradorEnergia e;
  for (int i = 0; i < 360000; i++) e.adicionar(7.4f, 1000.0f, 10);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 1000.0f, (float)e.mAh);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 7400.0f, (float)e.mWh);
  TEST_ASSERT_EQUAL_UINT32(3600000, e.duracaoMs);
}

void test_integrador_perfil_parado_e_andando() {
  // 5 min a 160 mA + 10 min a 870 mA, a 7,4 V: 13,33 + 145 = 158,3 mAh
  IntegradorEnergia e;
  for (int i = 0; i < 30000; i++) e.adicionar(7.4f, 160.0f, 10);
  for (int i = 0; i < 60000; i++) e.adicionar(7.4f, 870.0f, 10);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 158.333f, (float)e.mAh);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 1171.67f, (float)e.mWh);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_integrador_uma_hora_a_1A);
  RUN_TEST(test_integrador_perfil_parado_e_andando);
  RUN_TEST(test_media_minimo_e_maximo);
  RUN_TEST(test_minimo_com_corrente_negativa);
  RUN_TEST(test_sem_leituras_retorna_zero);
  RUN_TEST(test_conversao_para_o_trilho);
  RUN_TEST(test_trilho_invalido_nao_divide_por_zero);
  return UNITY_END();
}
