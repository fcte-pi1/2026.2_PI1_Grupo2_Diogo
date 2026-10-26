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
  TEST_ASSERT_FALSE(leituraVencida(ultima + 200u, ultima, 250));
  TEST_ASSERT_TRUE(leituraVencida(ultima + 300u, ultima, 250));
}

void test_periodo_e_orcamento_mais_margem() {
  TEST_ASSERT_EQUAL(60, periodoMedicaoMs(50000, 10));
  TEST_ASSERT_EQUAL(210, periodoMedicaoMs(200000, 10));  // alta precisão
}

void test_periodo_arredonda_o_orcamento_para_cima() {
  TEST_ASSERT_EQUAL(44, periodoMedicaoMs(33500, 10));
  TEST_ASSERT_TRUE(periodoMedicaoMs(33001, 0) * 1000u >= 33001u);
}

void test_validade_e_multiplo_do_periodo() {
  TEST_ASSERT_EQUAL(240, validadeLeituraMs(60, 4));
  TEST_ASSERT_EQUAL(840, validadeLeituraMs(210, 4));
}

void test_filtro_primeira_leitura_passa_direto() {
  FiltroLaser f(0.5f);
  TEST_ASSERT_EQUAL(100, f.atualizar(100));
}

void test_filtro_remove_pico_isolado() {
  FiltroLaser f(0.5f);
  f.atualizar(100);
  f.atualizar(100);
  TEST_ASSERT_EQUAL(100, f.atualizar(300));
  TEST_ASSERT_EQUAL(100, f.atualizar(100));
}

void test_filtro_suaviza_degrau_com_media_exponencial() {
  FiltroLaser f(0.5f);
  f.atualizar(100);
  f.atualizar(100);
  f.atualizar(100);
  TEST_ASSERT_EQUAL(100, f.atualizar(200));
  TEST_ASSERT_EQUAL(150, f.atualizar(200));
  TEST_ASSERT_EQUAL(175, f.atualizar(200));
}

void test_filtro_com_alfa_um_so_aplica_a_mediana() {
  FiltroLaser f(1.0f);
  f.atualizar(100);
  f.atualizar(100);
  f.atualizar(100);
  f.atualizar(200);
  TEST_ASSERT_EQUAL(200, f.atualizar(200));
}

void test_filtro_codigo_de_erro_passa_e_reinicia() {
  FiltroLaser f(0.5f);
  f.atualizar(100);
  f.atualizar(100);
  TEST_ASSERT_EQUAL(LASER_TIMEOUT, f.atualizar(LASER_TIMEOUT));
  TEST_ASSERT_EQUAL(300, f.atualizar(300));
}

void test_parede_presente_e_ausente_com_histerese() {
  DetectorParede d(120, 160);
  TEST_ASSERT_EQUAL(PAREDE_PRESENTE, d.atualizar(100));
  TEST_ASSERT_EQUAL(PAREDE_PRESENTE, d.atualizar(140));
  TEST_ASSERT_EQUAL(PAREDE_AUSENTE, d.atualizar(161));
  TEST_ASSERT_EQUAL(PAREDE_AUSENTE, d.atualizar(140));
  TEST_ASSERT_EQUAL(PAREDE_PRESENTE, d.atualizar(120));
}

void test_parede_comeca_na_faixa_de_histerese() {
  DetectorParede perto(120, 160);
  DetectorParede longe(120, 160);
  TEST_ASSERT_EQUAL(PAREDE_PRESENTE, perto.atualizar(130));
  TEST_ASSERT_EQUAL(PAREDE_AUSENTE, longe.atualizar(150));
}

void test_parede_fora_de_escala_e_ausente() {
  DetectorParede d(120, 160);
  d.atualizar(100);
  TEST_ASSERT_EQUAL(PAREDE_AUSENTE, d.atualizar(LASER_FORA_ESCALA));
}

void test_parede_indefinida_em_falha_e_lembra_o_estado() {
  DetectorParede d(120, 160);
  d.atualizar(100);
  TEST_ASSERT_EQUAL(PAREDE_INDEFINIDA, d.atualizar(LASER_SEM_LEITURA));
  TEST_ASSERT_EQUAL(PAREDE_INDEFINIDA, d.atualizar(LASER_TIMEOUT));
  TEST_ASSERT_EQUAL(PAREDE_PRESENTE, d.atualizar(140));
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
  RUN_TEST(test_periodo_e_orcamento_mais_margem);
  RUN_TEST(test_periodo_arredonda_o_orcamento_para_cima);
  RUN_TEST(test_validade_e_multiplo_do_periodo);
  RUN_TEST(test_filtro_primeira_leitura_passa_direto);
  RUN_TEST(test_filtro_remove_pico_isolado);
  RUN_TEST(test_filtro_suaviza_degrau_com_media_exponencial);
  RUN_TEST(test_filtro_com_alfa_um_so_aplica_a_mediana);
  RUN_TEST(test_filtro_codigo_de_erro_passa_e_reinicia);
  RUN_TEST(test_parede_presente_e_ausente_com_histerese);
  RUN_TEST(test_parede_comeca_na_faixa_de_histerese);
  RUN_TEST(test_parede_fora_de_escala_e_ausente);
  RUN_TEST(test_parede_indefinida_em_falha_e_lembra_o_estado);
  return UNITY_END();
}
