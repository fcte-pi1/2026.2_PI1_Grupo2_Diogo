#include <unity.h>
#include "centralizacao_logic.h"

static LateralLida lado(float mm) { LateralLida l; l.valida = true; l.mm = mm; return l; }
static const LateralLida NADA;

static ParametrosCentro parametros() {
  ParametrosCentro p;
  p.kp = 0.5f;
  p.kiPorMm = 0.0f;
  p.taxaMmPorS = 1000.0f;  // sem limite de taxa nos testes de sinal
  return p;
}

void setUp() {}
void tearDown() {}

void test_centrado_com_duas_paredes_nao_corrige() {
  Centralizador c(parametros());
  c.reiniciar();
  TEST_ASSERT_EQUAL_FLOAT(0.0f, c.atualizar(lado(50), lado(50), 1, 0.02f));
  TEST_ASSERT_TRUE(c.modo() == ModoCentro::DUAS);
}

void test_deslocado_para_direita_puxa_para_esquerda() {
  Centralizador c(parametros());
  c.reiniciar();
  const float alvo = c.atualizar(lado(60), lado(40), 1, 0.02f);
  TEST_ASSERT_EQUAL_FLOAT(10.0f, c.erro());
  TEST_ASSERT_TRUE(alvo < 0);  // direita anda mais que a esquerda
}

void test_deslocado_para_esquerda_puxa_para_direita() {
  Centralizador c(parametros());
  c.reiniciar();
  TEST_ASSERT_TRUE(c.atualizar(lado(40), lado(60), 1, 0.02f) > 0);
}

void test_assimetria_muda_o_centro() {
  ParametrosCentro p = parametros();
  p.assimetriaMm = 6;
  Centralizador c(p);
  c.reiniciar();
  c.atualizar(lado(53), lado(47), 1, 0.02f);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, c.erro());
}

void test_sem_referencia_uma_parede_nao_corrige() {
  Centralizador c(parametros());
  c.reiniciar();
  TEST_ASSERT_EQUAL_FLOAT(0.0f, c.atualizar(lado(30), NADA, 1, 0.02f));
  TEST_ASSERT_TRUE(c.modo() == ModoCentro::NENHUMA);
}

void test_uma_parede_usa_a_referencia_aprendida() {
  Centralizador c(parametros());
  c.reiniciar();
  c.atualizar(lado(50), lado(50), 1, 0.02f);
  TEST_ASSERT_EQUAL_FLOAT(50.0f, c.referencia());
  c.atualizar(lado(56), NADA, 1, 0.02f);
  TEST_ASSERT_TRUE(c.modo() == ModoCentro::ESQUERDA);
  TEST_ASSERT_EQUAL_FLOAT(6.0f, c.erro());
  c.atualizar(NADA, lado(56), 1, 0.02f);
  TEST_ASSERT_TRUE(c.modo() == ModoCentro::DIREITA);
  TEST_ASSERT_EQUAL_FLOAT(-6.0f, c.erro());
}

void test_referencia_fixa_vale_sem_aprender() {
  ParametrosCentro p = parametros();
  p.refFixaMm = 45;
  Centralizador c(p);
  c.reiniciar();
  c.atualizar(NADA, lado(40), 1, 0.02f);
  TEST_ASSERT_TRUE(c.modo() == ModoCentro::DIREITA);
  TEST_ASSERT_EQUAL_FLOAT(5.0f, c.erro());
}

void test_sem_parede_volta_a_zero_pela_taxa() {
  ParametrosCentro p = parametros();
  p.taxaMmPorS = 10;
  Centralizador c(p);
  c.reiniciar();
  for (int i = 0; i < 100; i++) c.atualizar(lado(60), lado(40), 1, 0.02f);
  TEST_ASSERT_EQUAL_FLOAT(-5.0f, c.alvo());
  c.atualizar(NADA, NADA, 1, 0.02f);
  TEST_ASSERT_TRUE(c.modo() == ModoCentro::NENHUMA);
  TEST_ASSERT_EQUAL_FLOAT(-4.8f, c.alvo());  // 10 mm/s * 20 ms
  for (int i = 0; i < 100; i++) c.atualizar(NADA, NADA, 1, 0.02f);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, c.alvo());
}

void test_integral_fica_congelada_sem_parede() {
  ParametrosCentro p = parametros();
  p.kp = 0;
  p.kiPorMm = 0.01f;
  Centralizador c(p);
  c.reiniciar();
  c.atualizar(lado(60), lado(40), 10, 0.02f);  // erro 10 * 10 mm * 0,01
  TEST_ASSERT_EQUAL_FLOAT(-1.0f, c.alvo());
  c.atualizar(NADA, NADA, 100, 0.02f);
  TEST_ASSERT_EQUAL_FLOAT(-1.0f, c.alvo());
}

void test_alvo_tem_limite() {
  ParametrosCentro p = parametros();
  p.alvoMaxMm = 4;
  Centralizador c(p);
  c.reiniciar();
  TEST_ASSERT_EQUAL_FLOAT(-4.0f, c.atualizar(lado(100), lado(10), 1, 0.02f));
}

static uint32_t n = 0;
static bool ler(LeitorLateral &l, int mm, bool parede = true) { return l.atualizar(++n, mm, parede, 10).valida; }

void test_leitor_exige_duas_leituras_coerentes() {
  LeitorLateral l(120, 150, 15);
  TEST_ASSERT_FALSE(ler(l, 50));
  TEST_ASSERT_TRUE(ler(l, 52));
}

void test_leitor_so_avalia_amostra_nova() {
  LeitorLateral l(120, 150, 15);
  ler(l, 50);
  ler(l, 51);
  TEST_ASSERT_FALSE(ler(l, 80));  // salto
  // A mesma amostra repetida no loop nao vira leitura aceita.
  TEST_ASSERT_FALSE(l.atualizar(n, 80, true, 10).valida);
  TEST_ASSERT_FALSE(l.atualizar(n, 80, true, 10).valida);
}

void test_leitor_recusa_borda_de_parede() {
  LeitorLateral l(120, 150, 15);
  ler(l, 49); ler(l, 49);
  TEST_ASSERT_FALSE(ler(l, 75));
  TEST_ASSERT_FALSE(ler(l, 110));
  TEST_ASSERT_FALSE(ler(l, 400, false));
  // Parede voltando: os valores intermediarios nao valem.
  TEST_ASSERT_FALSE(ler(l, 110));
  TEST_ASSERT_FALSE(ler(l, 75));
  TEST_ASSERT_FALSE(ler(l, 50));
  TEST_ASSERT_TRUE(ler(l, 49));
}

void test_leitor_recusa_pico_isolado() {
  LeitorLateral l(120, 150, 15);
  ler(l, 49); ler(l, 49);
  TEST_ASSERT_FALSE(ler(l, 85));
  TEST_ASSERT_FALSE(ler(l, 49));
  TEST_ASSERT_TRUE(ler(l, 50));
}

void test_leitor_recusa_sem_parede_longe_ou_velha() {
  LeitorLateral l(120, 150, 15);
  ler(l, 49); ler(l, 49);
  TEST_ASSERT_FALSE(l.atualizar(n, 49, true, 151).valida);
  LeitorLateral longe(120, 150, 15);
  ler(longe, 130); TEST_ASSERT_FALSE(ler(longe, 130));
  LeitorLateral sem(120, 150, 15);
  ler(sem, 60, false); TEST_ASSERT_FALSE(ler(sem, 60, false));
  LeitorLateral invalida(120, 150, 15);
  ler(invalida, 49); TEST_ASSERT_FALSE(ler(invalida, -3));
}

void test_leitor_exige_duas_de_novo_depois_de_pausa() {
  LeitorLateral l(120, 150, 15);
  ler(l, 49); ler(l, 49);
  TEST_ASSERT_FALSE(l.atualizar(n, 49, true, 300).valida);  // sensor parou
  TEST_ASSERT_FALSE(ler(l, 50));
  TEST_ASSERT_TRUE(ler(l, 50));
}

void test_leitor_continua_aceitando_por_muito_tempo() {
  LeitorLateral l(120, 150, 15);
  for (int i = 0; i < 600; i++) ler(l, 49);
  TEST_ASSERT_TRUE(ler(l, 50));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_centrado_com_duas_paredes_nao_corrige);
  RUN_TEST(test_deslocado_para_direita_puxa_para_esquerda);
  RUN_TEST(test_deslocado_para_esquerda_puxa_para_direita);
  RUN_TEST(test_assimetria_muda_o_centro);
  RUN_TEST(test_sem_referencia_uma_parede_nao_corrige);
  RUN_TEST(test_uma_parede_usa_a_referencia_aprendida);
  RUN_TEST(test_referencia_fixa_vale_sem_aprender);
  RUN_TEST(test_sem_parede_volta_a_zero_pela_taxa);
  RUN_TEST(test_integral_fica_congelada_sem_parede);
  RUN_TEST(test_alvo_tem_limite);
  RUN_TEST(test_leitor_exige_duas_leituras_coerentes);
  RUN_TEST(test_leitor_so_avalia_amostra_nova);
  RUN_TEST(test_leitor_recusa_borda_de_parede);
  RUN_TEST(test_leitor_recusa_pico_isolado);
  RUN_TEST(test_leitor_recusa_sem_parede_longe_ou_velha);
  RUN_TEST(test_leitor_exige_duas_de_novo_depois_de_pausa);
  RUN_TEST(test_leitor_continua_aceitando_por_muito_tempo);
  return UNITY_END();
}
