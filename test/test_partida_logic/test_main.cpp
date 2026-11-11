#include <unity.h>
#include "partida_logic.h"

void setUp() {}
void tearDown() {}

void test_roda_parte_com_pulsos_nos_dois_sentidos() {
  TEST_ASSERT_FALSE(rodaPartiu(2, 3));
  TEST_ASSERT_TRUE(rodaPartiu(3, 3));
  TEST_ASSERT_TRUE(rodaPartiu(-5, 3));
  TEST_ASSERT_FALSE(rodaPartiu(-2, 3));
}

void test_rampa_segura_no_piso_mantendo_o_sinal() {
  TEST_ASSERT_EQUAL_FLOAT(18.0f, segurarRampaPartida(40.0f, 18.0f));
  TEST_ASSERT_EQUAL_FLOAT(-18.0f, segurarRampaPartida(-40.0f, 18.0f));
}

void test_rampa_abaixo_do_piso_nao_muda() {
  TEST_ASSERT_EQUAL_FLOAT(9.6f, segurarRampaPartida(9.6f, 18.0f));
  TEST_ASSERT_EQUAL_FLOAT(-3.2f, segurarRampaPartida(-3.2f, 18.0f));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, segurarRampaPartida(0.0f, 18.0f));
}

void test_ajuda_vale_para_as_duas_rodas() {
  TEST_ASSERT_TRUE(ajudaPartidaConjunta(true, false, 0.0f, 1.0f));
  TEST_ASSERT_TRUE(ajudaPartidaConjunta(false, true, 0.0f, 1.0f));
  TEST_ASSERT_TRUE(ajudaPartidaConjunta(false, true, -3.0f, 1.0f));  // atrasada também recebe
  TEST_ASSERT_FALSE(ajudaPartidaConjunta(false, false, 0.0f, 1.0f));
}

void test_roda_adiantada_nao_recebe_a_ajuda_da_outra() {
  TEST_ASSERT_FALSE(ajudaPartidaConjunta(false, true, 1.5f, 1.0f));
  TEST_ASSERT_TRUE(ajudaPartidaConjunta(true, false, 1.5f, 1.0f));  // a própria ajuda continua
}

void test_registro_marca_a_saida_de_cada_roda() {
  RegistroPartida r(50.0f);
  r.reiniciar(1000);
  r.atualizar(0, 0, false, false, 1020);
  TEST_ASSERT_FALSE(r.ler().duasPartiram());
  r.atualizar(0.4f, 0, true, false, 1040);
  r.atualizar(1.0f, 0.4f, true, true, 1100);
  TEST_ASSERT_TRUE(r.ler().duasPartiram());
  TEST_ASSERT_EQUAL_INT32(40, r.ler().partidaEMs);
  TEST_ASSERT_EQUAL_INT32(100, r.ler().partidaDMs);
  TEST_ASSERT_EQUAL_INT32(60, r.ler().atrasoMs());
}

void test_atraso_negativo_quando_a_direita_sai_antes() {
  RegistroPartida r(50.0f);
  r.reiniciar(0);
  r.atualizar(0, 0.4f, false, true, 20);
  r.atualizar(0.4f, 1.0f, true, true, 80);
  TEST_ASSERT_EQUAL_INT32(-60, r.ler().atrasoMs());
}

void test_registro_guarda_diferenca_dentro_da_janela() {
  RegistroPartida r(50.0f);
  r.reiniciar(0);
  r.atualizar(10.0f, 7.0f, true, true, 100);
  r.atualizar(30.0f, 29.0f, true, true, 200);
  TEST_ASSERT_FALSE(r.ler().janelaCompleta);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, r.ler().diferencaMm);
  TEST_ASSERT_EQUAL_FLOAT(3.0f, r.ler().diferencaMaxMm);
}

void test_registro_congela_depois_da_janela() {
  RegistroPartida r(50.0f);
  r.reiniciar(0);
  r.atualizar(49.0f, 48.0f, true, true, 100);
  r.atualizar(60.0f, 50.0f, true, true, 200);  // média 55 mm: fora da janela
  r.atualizar(90.0f, 70.0f, true, true, 300);
  TEST_ASSERT_TRUE(r.ler().janelaCompleta);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, r.ler().diferencaMm);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, r.ler().diferencaMaxMm);
}

void test_reiniciar_limpa_o_registro() {
  RegistroPartida r(50.0f);
  r.reiniciar(0);
  r.atualizar(60.0f, 40.0f, true, true, 100);
  r.reiniciar(500);
  TEST_ASSERT_FALSE(r.ler().janelaCompleta);
  TEST_ASSERT_FALSE(r.ler().duasPartiram());
  TEST_ASSERT_EQUAL_FLOAT(0.0f, r.ler().diferencaMaxMm);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_roda_parte_com_pulsos_nos_dois_sentidos);
  RUN_TEST(test_rampa_segura_no_piso_mantendo_o_sinal);
  RUN_TEST(test_rampa_abaixo_do_piso_nao_muda);
  RUN_TEST(test_ajuda_vale_para_as_duas_rodas);
  RUN_TEST(test_roda_adiantada_nao_recebe_a_ajuda_da_outra);
  RUN_TEST(test_registro_marca_a_saida_de_cada_roda);
  RUN_TEST(test_atraso_negativo_quando_a_direita_sai_antes);
  RUN_TEST(test_registro_guarda_diferenca_dentro_da_janela);
  RUN_TEST(test_registro_congela_depois_da_janela);
  RUN_TEST(test_reiniciar_limpa_o_registro);
  return UNITY_END();
}
