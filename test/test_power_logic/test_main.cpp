#include <unity.h>
#include "power_logic.h"

static const LimitesBateria LIM = {7.0f, 6.6f, 2000, 2000.0f, 500};

void setUp() {}
void tearDown() {}

void test_percentual_nos_limites() {
  TEST_ASSERT_EQUAL_FLOAT(100.0f, bateriaPercentual(8.4f, 2));
  TEST_ASSERT_EQUAL_FLOAT(100.0f, bateriaPercentual(8.6f, 2));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, bateriaPercentual(6.6f, 2));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, bateriaPercentual(5.0f, 2));
}

void test_percentual_interpola_entre_pontos() {
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 50.0f, bateriaPercentual(2 * 3.84f, 2));
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 55.0f, bateriaPercentual(2 * 3.855f, 2));
}

void test_percentual_nunca_diminui_com_a_tensao() {
  float anterior = 0.0f;
  for (float v = 6.5f; v <= 8.5f; v += 0.01f) {
    float pct = bateriaPercentual(v, 2);
    TEST_ASSERT_TRUE(pct >= anterior);
    anterior = pct;
  }
}

void test_queda_curta_no_pico_nao_corta() {
  MonitorBateria m(LIM);
  m.atualizar(7.4f, 800.0f, 0);
  m.atualizar(6.3f, 1500.0f, 100);
  m.atualizar(6.3f, 1500.0f, 1500);
  m.atualizar(7.3f, 900.0f, 1600);
  m.atualizar(7.3f, 900.0f, 5000);
  TEST_ASSERT_EQUAL(BATERIA_NORMAL, m.estado());
}

void test_subtensao_sustentada_corta() {
  MonitorBateria m(LIM);
  m.atualizar(6.5f, 500.0f, 0);
  m.atualizar(6.5f, 500.0f, 1999);
  TEST_ASSERT_NOT_EQUAL(BATERIA_CRITICA, m.estado());
  m.atualizar(6.5f, 500.0f, 2000);
  TEST_ASSERT_EQUAL(BATERIA_CRITICA, m.estado());
}

void test_corte_fica_travado_mesmo_se_tensao_subir() {
  MonitorBateria m(LIM);
  m.atualizar(6.5f, 500.0f, 0);
  m.atualizar(6.5f, 500.0f, 2000);
  m.atualizar(7.6f, 150.0f, 5000);
  TEST_ASSERT_EQUAL(BATERIA_CRITICA, m.estado());
}

void test_alerta_de_bateria_baixa_antes_do_corte() {
  MonitorBateria m(LIM);
  m.atualizar(6.9f, 500.0f, 0);
  m.atualizar(6.9f, 500.0f, 2000);
  TEST_ASSERT_EQUAL(BATERIA_BAIXA, m.estado());
  m.atualizar(7.3f, 150.0f, 3000);
  TEST_ASSERT_EQUAL(BATERIA_BAIXA, m.estado());
}

void test_sobrecorrente_so_depois_do_tempo_limite() {
  MonitorBateria m(LIM);
  m.atualizar(7.4f, 2100.0f, 0);
  m.atualizar(7.4f, 2100.0f, 499);
  TEST_ASSERT_FALSE(m.sobrecorrente());
  m.atualizar(7.4f, 2100.0f, 500);
  TEST_ASSERT_TRUE(m.sobrecorrente());
  m.atualizar(7.4f, 900.0f, 600);
  TEST_ASSERT_FALSE(m.sobrecorrente());
}

void test_corte_funciona_no_estouro_do_millis() {
  MonitorBateria m(LIM);
  m.atualizar(6.5f, 500.0f, 0xFFFFFC00u);  // 1024 ms antes do estouro
  m.atualizar(6.5f, 500.0f, 0xFFFFFF00u);  // 768 ms depois: ainda não pode cortar
  TEST_ASSERT_NOT_EQUAL(BATERIA_CRITICA, m.estado());
  m.atualizar(6.5f, 500.0f, 0x000003FFu);  // 2047 ms depois
  TEST_ASSERT_EQUAL(BATERIA_CRITICA, m.estado());
}

void test_limites_de_ensaio_so_deixam_a_protecao_mais_sensivel() {
  TEST_ASSERT_TRUE(limitesDeEnsaioValidos(8.0f, 100.0f, LIM, 9.0f, 50.0f));
  TEST_ASSERT_TRUE(limitesDeEnsaioValidos(6.6f, 2000.0f, LIM, 9.0f, 50.0f));   // os próprios padrões
  TEST_ASSERT_FALSE(limitesDeEnsaioValidos(6.0f, 2000.0f, LIM, 9.0f, 50.0f));  // corte abaixo do padrão
  TEST_ASSERT_FALSE(limitesDeEnsaioValidos(6.6f, 3000.0f, LIM, 9.0f, 50.0f));  // sobrecorrente acima do padrão
  TEST_ASSERT_FALSE(limitesDeEnsaioValidos(9.5f, 2000.0f, LIM, 9.0f, 50.0f));
  TEST_ASSERT_FALSE(limitesDeEnsaioValidos(6.6f, 10.0f, LIM, 9.0f, 50.0f));
}

void test_corte_real_nao_rearma_com_bateria_descarregada() {
  TEST_ASSERT_FALSE(podeRearmarCorte(BATERIA_CRITICA, 6.9f, LIM.vAlerta));
  TEST_ASSERT_TRUE(podeRearmarCorte(BATERIA_CRITICA, 7.34f, LIM.vAlerta));  // corte de ensaio com a bateria boa
}

void test_sem_corte_os_limites_podem_ser_trocados() {
  TEST_ASSERT_TRUE(podeRearmarCorte(BATERIA_NORMAL, 6.9f, LIM.vAlerta));
  TEST_ASSERT_TRUE(podeRearmarCorte(BATERIA_BAIXA, 6.9f, LIM.vAlerta));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_percentual_nos_limites);
  RUN_TEST(test_percentual_interpola_entre_pontos);
  RUN_TEST(test_percentual_nunca_diminui_com_a_tensao);
  RUN_TEST(test_queda_curta_no_pico_nao_corta);
  RUN_TEST(test_subtensao_sustentada_corta);
  RUN_TEST(test_corte_fica_travado_mesmo_se_tensao_subir);
  RUN_TEST(test_alerta_de_bateria_baixa_antes_do_corte);
  RUN_TEST(test_sobrecorrente_so_depois_do_tempo_limite);
  RUN_TEST(test_corte_funciona_no_estouro_do_millis);
  RUN_TEST(test_limites_de_ensaio_so_deixam_a_protecao_mais_sensivel);
  RUN_TEST(test_corte_real_nao_rearma_com_bateria_descarregada);
  RUN_TEST(test_sem_corte_os_limites_podem_ser_trocados);
  return UNITY_END();
}
