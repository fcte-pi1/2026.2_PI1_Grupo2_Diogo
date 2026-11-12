#include <unity.h>
#include "frente_logic.h"

static const float LIMIAR = 75.0f;   // 50 mm + margem de frenagem a 35 RPM
static uint32_t n = 0, t = 0;

static ProtecaoFrontal nova() { return ProtecaoFrontal(150, 3, 500); }
static void amostra(ProtecaoFrontal &p, int leitura, float percurso, uint32_t passoMs = 60) {
  t += passoMs;
  p.observar(++n, leitura, t, percurso);
}
static DecisaoFrente decide(const ProtecaoFrontal &p, int leitura, float percurso) {
  return p.decidir(leitura, LIMIAR, t, percurso);
}

void setUp() { n = 0; t = 1000; }
void tearDown() {}

void test_sem_alvo_so_fora_de_faixa_e_fase() {
  TEST_ASSERT_TRUE(frenteSemAlvo(LASER_FORA_ESCALA));
  TEST_ASSERT_TRUE(frenteSemAlvo(LASER_FASE_INVALIDA));
  TEST_ASSERT_FALSE(frenteSemAlvo(LASER_SEM_LEITURA));
  TEST_ASSERT_FALSE(frenteSemAlvo(LASER_PERTO_DEMAIS));
  TEST_ASSERT_FALSE(frenteSemAlvo(LASER_TIMEOUT));
  TEST_ASSERT_FALSE(frenteSemAlvo(LASER_ERRO_COMUNICACAO));
}

void test_corredor_aberto_segue_e_para_na_parede_que_surge() {
  ProtecaoFrontal p = nova();
  for (int i = 0; i < 5; i++) amostra(p, LASER_FORA_ESCALA, 0);
  TEST_ASSERT_TRUE(p.podePartir(LASER_FORA_ESCALA, 85, t));
  p.iniciarMovimento(t);
  float s = 0;
  for (int i = 0; i < 10; i++) { s += 4; amostra(p, LASER_FORA_ESCALA, s); }
  TEST_ASSERT_TRUE(DecisaoFrente::SEGUIR == decide(p, LASER_FORA_ESCALA, s));
  int parede = 1100;
  DecisaoFrente d = DecisaoFrente::SEGUIR;
  while (d == DecisaoFrente::SEGUIR && parede > 0) {
    s += 4; parede -= 4;
    amostra(p, parede, s);
    d = decide(p, parede, s);
  }
  TEST_ASSERT_TRUE(DecisaoFrente::PAREDE == d);
  TEST_ASSERT_TRUE(parede <= LIMIAR && parede > LIMIAR - 5);
}

void test_parede_vista_que_vira_sem_alvo_para_pelo_percurso() {
  ProtecaoFrontal p = nova();
  amostra(p, 300, 0);
  p.iniciarMovimento(t);
  float s = 0;
  DecisaoFrente d = DecisaoFrente::SEGUIR;
  while (d == DecisaoFrente::SEGUIR && s < 400) {
    s += 4;
    amostra(p, LASER_FASE_INVALIDA, s);
    d = decide(p, LASER_FASE_INVALIDA, s);
  }
  TEST_ASSERT_TRUE(DecisaoFrente::PAREDE == d);
  TEST_ASSERT_FLOAT_WITHIN(5.0f, 300.0f - LIMIAR, s);
}

void test_leitura_piscando_usa_a_ultima_valida() {
  ProtecaoFrontal p = nova();
  float s = 0;
  amostra(p, 500, s);
  for (int i = 0; i < 20; i++) {
    s += 4;
    const int leitura = (i % 2) ? LASER_FORA_ESCALA : 500 - (int)s;
    amostra(p, leitura, s);
    TEST_ASSERT_TRUE(DecisaoFrente::SEGUIR == decide(p, leitura, s));
  }
  TEST_ASSERT_FLOAT_WITHIN(5.0f, 500.0f - s, p.paredeEstimadaMm(s));
}

void test_falhas_sempre_param() {
  ProtecaoFrontal p = nova();
  amostra(p, LASER_FORA_ESCALA, 0);
  TEST_ASSERT_TRUE(DecisaoFrente::INVALIDA == decide(p, LASER_SEM_LEITURA, 0));
  TEST_ASSERT_TRUE(DecisaoFrente::INVALIDA == decide(p, LASER_PERTO_DEMAIS, 0));
  TEST_ASSERT_TRUE(DecisaoFrente::INVALIDA == decide(p, LASER_ERRO_COMUNICACAO, 0));
  TEST_ASSERT_TRUE(DecisaoFrente::INVALIDA == decide(p, LASER_TIMEOUT, 0));
}

void test_sensor_parado_e_leitura_velha() {
  ProtecaoFrontal p = nova();
  amostra(p, LASER_FORA_ESCALA, 0);
  t += 200;  // nenhuma amostra nova
  TEST_ASSERT_TRUE(DecisaoFrente::INVALIDA == decide(p, LASER_FORA_ESCALA, 0));
  ProtecaoFrontal vazia = nova();
  TEST_ASSERT_TRUE(DecisaoFrente::INVALIDA == vazia.decidir(500, LIMIAR, t, 0));
}

void test_mesma_amostra_repetida_nao_conta_de_novo() {
  ProtecaoFrontal p = nova();
  amostra(p, LASER_FORA_ESCALA, 0);
  for (int i = 0; i < 10; i++) p.observar(n, LASER_FORA_ESCALA, t, 0);
  TEST_ASSERT_EQUAL_UINT8(1, p.semAlvoSeguidasAgora());
  TEST_ASSERT_FALSE(p.podePartir(LASER_FORA_ESCALA, 85, t));
}

void test_partida_exige_sem_alvo_seguidas() {
  ProtecaoFrontal p = nova();
  amostra(p, LASER_FORA_ESCALA, 0);
  amostra(p, LASER_FORA_ESCALA, 0);
  TEST_ASSERT_FALSE(p.podePartir(LASER_FORA_ESCALA, 85, t));
  amostra(p, LASER_SEM_LEITURA, 0);  // falha zera a contagem
  amostra(p, LASER_FORA_ESCALA, 0);
  amostra(p, LASER_FORA_ESCALA, 0);
  TEST_ASSERT_FALSE(p.podePartir(LASER_FORA_ESCALA, 85, t));
  amostra(p, LASER_FORA_ESCALA, 0);
  TEST_ASSERT_TRUE(p.podePartir(LASER_FORA_ESCALA, 85, t));
}

void test_partida_com_leitura_valida() {
  ProtecaoFrontal p = nova();
  amostra(p, 80, 0);
  TEST_ASSERT_FALSE(p.podePartir(80, 85, t));
  amostra(p, 400, 0);
  TEST_ASSERT_TRUE(p.podePartir(400, 85, t));
}

void test_partida_recusa_parede_perto_que_sumiu() {
  ProtecaoFrontal p = nova();
  amostra(p, 60, 0);
  for (int i = 0; i < 3; i++) amostra(p, LASER_FASE_INVALIDA, 0, 50);
  TEST_ASSERT_FALSE(p.podePartir(LASER_FASE_INVALIDA, 85, t));
  // Muito tempo depois, a lembrança expira.
  for (int i = 0; i < 10; i++) amostra(p, LASER_FASE_INVALIDA, 0, 100);
  TEST_ASSERT_TRUE(p.podePartir(LASER_FASE_INVALIDA, 85, t));
}

void test_esquecer_parede_depois_de_girar() {
  ProtecaoFrontal p = nova();
  amostra(p, 120, 0);
  p.esquecerParede();
  TEST_ASSERT_TRUE(p.paredeEstimadaMm(0) < 0);
  amostra(p, LASER_FORA_ESCALA, 0);
  TEST_ASSERT_TRUE(DecisaoFrente::SEGUIR == decide(p, LASER_FORA_ESCALA, 500));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_sem_alvo_so_fora_de_faixa_e_fase);
  RUN_TEST(test_corredor_aberto_segue_e_para_na_parede_que_surge);
  RUN_TEST(test_parede_vista_que_vira_sem_alvo_para_pelo_percurso);
  RUN_TEST(test_leitura_piscando_usa_a_ultima_valida);
  RUN_TEST(test_falhas_sempre_param);
  RUN_TEST(test_sensor_parado_e_leitura_velha);
  RUN_TEST(test_mesma_amostra_repetida_nao_conta_de_novo);
  RUN_TEST(test_partida_exige_sem_alvo_seguidas);
  RUN_TEST(test_partida_com_leitura_valida);
  RUN_TEST(test_partida_recusa_parede_perto_que_sumiu);
  RUN_TEST(test_esquecer_parede_depois_de_girar);
  return UNITY_END();
}
