#include <unity.h>
#include "speed_logic.h"

static const float DIAMETRO_MM = 34.0f;

void setUp() {}
void tearDown() {}

void test_velocidade_a_60_rpm() {
  // 60 RPM = 1 volta/s = pi * 34 mm/s = 10,68 cm/s
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.68f, velocidadeCmS(60.0f, 60.0f, DIAMETRO_MM));
}

void test_velocidade_usa_a_media_das_rodas() {
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.68f, velocidadeCmS(40.0f, 80.0f, DIAMETRO_MM));
}

void test_velocidade_negativa_de_re() {
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -5.34f, velocidadeCmS(-30.0f, -30.0f, DIAMETRO_MM));
}

void test_giro_no_proprio_eixo_tem_velocidade_zero() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, velocidadeCmS(40.0f, -40.0f, DIAMETRO_MM));
}

void test_rpm_da_roda_pelo_encoder() {
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 60.0f, rpmDaRoda(815.5f, 815.5f));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -30.0f, rpmDaRoda(-407.75f, 815.5f));
}

void test_sem_cpr_calibrado_nao_inventa_valor() {
  TEST_ASSERT_EQUAL_FLOAT(0.0f, rpmDaRoda(500.0f, 0.0f));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, distanciaRodaMm(500, 0.0f, DIAMETRO_MM));
}

void test_distancia_de_uma_volta_e_a_circunferencia() {
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 106.81f, distanciaRodaMm(816, 816.0f, DIAMETRO_MM));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -106.81f, distanciaRodaMm(-816, 816.0f, DIAMETRO_MM));
}

void test_media_e_distancia_sobre_tempo() {
  VelocidadeMedia m;
  for (int k = 0; k < 500; k++) m.acumular(2.0f, 20);  // 1000 mm em 10 s
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 1000.0f, m.distanciaMm());
  TEST_ASSERT_EQUAL_UINT32(10000, m.tempoMs());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, m.mediaCmS());
}

void test_media_conta_o_tempo_parado_dentro_do_teste() {
  VelocidadeMedia m;
  for (int k = 0; k < 250; k++) m.acumular(2.0f, 20);  // 500 mm em 5 s
  for (int k = 0; k < 250; k++) m.acumular(0.0f, 20);  // 5 s parado (ou girando no eixo)
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, m.mediaCmS());
}

void test_ir_e_voltar_soma_a_distancia() {
  VelocidadeMedia m;
  for (int k = 0; k < 100; k++) m.acumular(2.0f, 20);
  for (int k = 0; k < 100; k++) m.acumular(-2.0f, 20);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 400.0f, m.distanciaMm());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, m.mediaCmS());
}

void test_media_sem_tempo_e_zero() {
  VelocidadeMedia m;
  TEST_ASSERT_EQUAL_FLOAT(0.0f, m.mediaCmS());
  m.acumular(5.0f, 20);
  m.reiniciar();
  TEST_ASSERT_EQUAL_FLOAT(0.0f, m.mediaCmS());
  TEST_ASSERT_EQUAL_FLOAT(0.0f, m.distanciaMm());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_velocidade_a_60_rpm);
  RUN_TEST(test_velocidade_usa_a_media_das_rodas);
  RUN_TEST(test_velocidade_negativa_de_re);
  RUN_TEST(test_giro_no_proprio_eixo_tem_velocidade_zero);
  RUN_TEST(test_rpm_da_roda_pelo_encoder);
  RUN_TEST(test_sem_cpr_calibrado_nao_inventa_valor);
  RUN_TEST(test_distancia_de_uma_volta_e_a_circunferencia);
  RUN_TEST(test_media_e_distancia_sobre_tempo);
  RUN_TEST(test_media_conta_o_tempo_parado_dentro_do_teste);
  RUN_TEST(test_ir_e_voltar_soma_a_distancia);
  RUN_TEST(test_media_sem_tempo_e_zero);
  return UNITY_END();
}
