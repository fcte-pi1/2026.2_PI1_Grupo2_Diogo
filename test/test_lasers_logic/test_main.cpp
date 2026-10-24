#include <unity.h>
#include "lasers_logic.h"

static const int MIN_MM = 20;
static const int MAX_MM = 2000;

void setUp() {}
void tearDown() {}

void test_leitura_valida_devolve_a_distancia() {
  TEST_ASSERT_EQUAL(150, classificarLeitura(false, LASER_STATUS_VALIDO, 150, MIN_MM, MAX_MM));
}

void test_limites_da_faixa_sao_validos() {
  TEST_ASSERT_EQUAL(MIN_MM, classificarLeitura(false, LASER_STATUS_VALIDO, MIN_MM, MIN_MM, MAX_MM));
  TEST_ASSERT_EQUAL(MAX_MM, classificarLeitura(false, LASER_STATUS_VALIDO, MAX_MM, MIN_MM, MAX_MM));
}

void test_acima_do_maximo_e_fora_de_escala() {
  TEST_ASSERT_EQUAL(LASER_FORA_ESCALA, classificarLeitura(false, LASER_STATUS_VALIDO, MAX_MM + 1, MIN_MM, MAX_MM));
}

void test_abaixo_do_minimo_e_sem_leitura() {
  TEST_ASSERT_EQUAL(LASER_SEM_LEITURA, classificarLeitura(false, LASER_STATUS_VALIDO, MIN_MM - 1, MIN_MM, MAX_MM));
  TEST_ASSERT_EQUAL(LASER_SEM_LEITURA, classificarLeitura(false, LASER_STATUS_VALIDO, 0, MIN_MM, MAX_MM));
}

void test_phase_fail_e_fora_de_escala() {
  TEST_ASSERT_EQUAL(LASER_FORA_ESCALA, classificarLeitura(false, LASER_STATUS_FASE, 8190, MIN_MM, MAX_MM));
}

void test_demais_status_sao_sem_leitura() {
  const uint8_t ruins[] = {1, 2, 3, 5};  // Sigma, Signal, Min Range, Hardware
  for (uint8_t s : ruins) {
    TEST_ASSERT_EQUAL(LASER_SEM_LEITURA, classificarLeitura(false, s, 150, MIN_MM, MAX_MM));
  }
}

void test_erro_de_comunicacao_tem_prioridade() {
  // Mesmo com status 0 e uma distância plausível, o dado não vale nada
  TEST_ASSERT_EQUAL(LASER_TIMEOUT, classificarLeitura(true, LASER_STATUS_VALIDO, 150, MIN_MM, MAX_MM));
  TEST_ASSERT_EQUAL(LASER_TIMEOUT, classificarLeitura(true, LASER_STATUS_FASE, 8190, MIN_MM, MAX_MM));
}

void test_codigos_sao_negativos_e_distintos() {
  TEST_ASSERT_TRUE(LASER_SEM_LEITURA < 0);
  TEST_ASSERT_TRUE(LASER_FORA_ESCALA < 0);
  TEST_ASSERT_TRUE(LASER_TIMEOUT < 0);
  TEST_ASSERT_NOT_EQUAL(LASER_SEM_LEITURA, LASER_FORA_ESCALA);
  TEST_ASSERT_NOT_EQUAL(LASER_SEM_LEITURA, LASER_TIMEOUT);
  TEST_ASSERT_NOT_EQUAL(LASER_FORA_ESCALA, LASER_TIMEOUT);
}

void test_leitura_dentro_da_validade_nao_vence() {
  TEST_ASSERT_FALSE(leituraVencida(1000, 1000, 250));
  TEST_ASSERT_FALSE(leituraVencida(1250, 1000, 250));  // exatamente no limite
}

void test_leitura_alem_da_validade_vence() {
  TEST_ASSERT_TRUE(leituraVencida(1251, 1000, 250));
}

void test_validade_funciona_no_estouro_do_millis() {
  const uint32_t ultima = 0xFFFFFFF0u;  // 16 ms antes do estouro
  TEST_ASSERT_FALSE(leituraVencida(ultima + 200u, ultima, 250));  // agora já passou de 0
  TEST_ASSERT_TRUE(leituraVencida(ultima + 300u, ultima, 250));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_leitura_valida_devolve_a_distancia);
  RUN_TEST(test_limites_da_faixa_sao_validos);
  RUN_TEST(test_acima_do_maximo_e_fora_de_escala);
  RUN_TEST(test_abaixo_do_minimo_e_sem_leitura);
  RUN_TEST(test_phase_fail_e_fora_de_escala);
  RUN_TEST(test_demais_status_sao_sem_leitura);
  RUN_TEST(test_erro_de_comunicacao_tem_prioridade);
  RUN_TEST(test_codigos_sao_negativos_e_distintos);
  RUN_TEST(test_leitura_dentro_da_validade_nao_vence);
  RUN_TEST(test_leitura_alem_da_validade_vence);
  RUN_TEST(test_validade_funciona_no_estouro_do_millis);
  return UNITY_END();
}
