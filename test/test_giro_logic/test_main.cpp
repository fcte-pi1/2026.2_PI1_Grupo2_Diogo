#include <unity.h>
#include "giro_logic.h"

static const float TOL = 1.0f;
static const float ERRO_MAX = 15.0f;
static const uint8_t MAX_CORRECOES = 2;

void setUp() {}
void tearDown() {}

void test_erro_dentro_da_tolerancia_nao_corrige() {
  TEST_ASSERT_TRUE(DecisaoCorrecao::DENTRO_DA_TOLERANCIA == decidirCorrecaoGiro(0.7f, TOL, ERRO_MAX, 0, MAX_CORRECOES));
  TEST_ASSERT_TRUE(DecisaoCorrecao::DENTRO_DA_TOLERANCIA == decidirCorrecaoGiro(-1.0f, TOL, ERRO_MAX, 0, MAX_CORRECOES));
}

void test_erro_acima_da_tolerancia_corrige_nos_dois_sentidos() {
  TEST_ASSERT_TRUE(DecisaoCorrecao::CORRIGIR == decidirCorrecaoGiro(1.4f, TOL, ERRO_MAX, 0, MAX_CORRECOES));
  TEST_ASSERT_TRUE(DecisaoCorrecao::CORRIGIR == decidirCorrecaoGiro(-3.0f, TOL, ERRO_MAX, 1, MAX_CORRECOES));
}

void test_erro_grande_demais_nao_e_corrigido() {
  TEST_ASSERT_TRUE(DecisaoCorrecao::ERRO_GRANDE_DEMAIS == decidirCorrecaoGiro(40.0f, TOL, ERRO_MAX, 0, MAX_CORRECOES));
  TEST_ASSERT_TRUE(DecisaoCorrecao::ERRO_GRANDE_DEMAIS == decidirCorrecaoGiro(-15.5f, TOL, ERRO_MAX, 0, MAX_CORRECOES));
}

void test_para_de_corrigir_quando_as_tentativas_acabam() {
  TEST_ASSERT_TRUE(DecisaoCorrecao::TENTATIVAS_ESGOTADAS == decidirCorrecaoGiro(1.4f, TOL, ERRO_MAX, 2, MAX_CORRECOES));
}

void test_dentro_da_tolerancia_vale_mesmo_sem_tentativas() {
  TEST_ASSERT_TRUE(DecisaoCorrecao::DENTRO_DA_TOLERANCIA == decidirCorrecaoGiro(0.5f, TOL, ERRO_MAX, 2, MAX_CORRECOES));
}

void test_parado_so_depois_do_tempo_seguido() {
  DetectorParado p(1.0f, 100);
  TEST_ASSERT_FALSE(p.atualizar(0.2f, 1000));
  TEST_ASSERT_FALSE(p.atualizar(-0.3f, 1060));
  TEST_ASSERT_TRUE(p.atualizar(0.1f, 1100));
}

void test_movimento_reinicia_a_contagem_de_parado() {
  DetectorParado p(1.0f, 100);
  p.atualizar(0.2f, 1000);
  TEST_ASSERT_FALSE(p.atualizar(5.0f, 1080));   // ainda girando por inércia
  TEST_ASSERT_FALSE(p.atualizar(0.2f, 1120));   // a contagem recomeça aqui
  TEST_ASSERT_FALSE(p.atualizar(0.2f, 1200));
  TEST_ASSERT_TRUE(p.atualizar(0.2f, 1220));
}

void test_giro_para_qualquer_lado_conta_como_movimento() {
  DetectorParado p(1.0f, 100);
  p.atualizar(0.0f, 1000);
  TEST_ASSERT_FALSE(p.atualizar(-5.0f, 1150));
}

void test_reiniciar_esquece_o_tempo_parado() {
  DetectorParado p(1.0f, 100);
  p.atualizar(0.0f, 1000);
  p.reiniciar();
  TEST_ASSERT_FALSE(p.atualizar(0.0f, 1200));
  TEST_ASSERT_TRUE(p.atualizar(0.0f, 1300));
}

void test_parado_funciona_no_estouro_do_millis() {
  DetectorParado p(1.0f, 100);
  TEST_ASSERT_FALSE(p.atualizar(0.0f, 0xFFFFFFE0u));
  TEST_ASSERT_FALSE(p.atualizar(0.0f, 0x00000010u));  // 48 ms depois
  TEST_ASSERT_TRUE(p.atualizar(0.0f, 0x00000050u));   // 112 ms depois
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_erro_dentro_da_tolerancia_nao_corrige);
  RUN_TEST(test_erro_acima_da_tolerancia_corrige_nos_dois_sentidos);
  RUN_TEST(test_erro_grande_demais_nao_e_corrigido);
  RUN_TEST(test_para_de_corrigir_quando_as_tentativas_acabam);
  RUN_TEST(test_dentro_da_tolerancia_vale_mesmo_sem_tentativas);
  RUN_TEST(test_parado_so_depois_do_tempo_seguido);
  RUN_TEST(test_movimento_reinicia_a_contagem_de_parado);
  RUN_TEST(test_giro_para_qualquer_lado_conta_como_movimento);
  RUN_TEST(test_reiniciar_esquece_o_tempo_parado);
  RUN_TEST(test_parado_funciona_no_estouro_do_millis);
  return UNITY_END();
}
