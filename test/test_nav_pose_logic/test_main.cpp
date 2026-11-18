#include <unity.h>
#include "nav_pose_logic.h"
#include "robot_geometry.h"

// Geometria medida do Xaropinho. Derivadas: s=60, m=80, eixo a 100 do nariz,
// eixo a 20 da traseira, U3 a 40 a frente do eixo.
static GeometriaRobo medido() {
  GeometriaRobo g;
  g.larguraMm = 110.0f; g.comprimentoMm = 120.0f;
  g.lateralAoNarizMm = 40.0f; g.u3AoNarizMm = 60.0f; g.u3TocaMm = 60.0f;
  g.bordaAteGiroFrenteMm = 140.0f; g.bordaAteGiroReMm = 20.0f;
  return g;
}
static const float CELULA = 180.0f;
static const float FOLGA = 15.0f;

void setUp() {}
void tearDown() {}

// CENARIO 1: a missao comeca com a traseira encostada, nao no centro.
void test_partida_com_traseira_encostada() {
  const GeometriaRobo g = medido();
  const float m = geomMeiaAberturaMm(g);
  const float off = navPoseOffsetTraseiraEncostadaMm(m, geomEixoATraseiraMm(g));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -60.0f, off);   // 20 - 80
  // Centralizar sem trocar de celula: anda 60 mm para frente.
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 60.0f, navPoseMetaCentragemMm(off, 0.0f));
  // Ou atravessa direto para o centro da proxima celula: 180 + 60.
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 240.0f, navPoseMetaRetaMm(off, CELULA, 0.0f));
}

// A reta e sempre "o que falta". Partindo do centro, e uma celula cheia.
void test_reta_a_partir_do_centro_e_uma_celula() {
  TEST_ASSERT_FLOAT_WITHIN(0.01f, CELULA, navPoseMetaRetaMm(0.0f, CELULA, 0.0f));
}

// CENARIO 2: leitura frontal <-> posicao do eixo.
void test_leitura_frontal_vira_posicao() {
  const GeometriaRobo g = medido();
  const float m = geomMeiaAberturaMm(g), u3 = geomU3AoEixoMm(g);
  // Centrado numa celula com parede a frente o U3 le 40 mm.
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 40.0f, navPoseLeituraFrontalEsperadaMm(0.0f, m, u3));
  // E a conversao inversa fecha.
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, navPoseOffsetPelaLeituraFrontalMm(40.0f, m, u3));
  // Tocando a parede o U3 le 60: o eixo ainda esta 20 mm ATRAS do centro.
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -20.0f, navPoseOffsetPelaLeituraFrontalMm(g.u3TocaMm, m, u3));
}

// CENARIO 3: com parede a frente este robo NAO alcanca o centro da celula.
// Nao e erro de calibracao: o nariz (100 mm) passa da meia largura (80 mm).
void test_parede_a_frente_limita_o_quanto_se_avanca() {
  const GeometriaRobo g = medido();
  const float m = geomMeiaAberturaMm(g), u3 = geomU3AoEixoMm(g);
  const float lim = navPoseOffsetMaxComParedeMm(m, u3, g.u3TocaMm, FOLGA);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -35.0f, lim);   // 80 - 40 - 75
  TEST_ASSERT_TRUE(lim < 0.0f);
  // Vindo do centro da celula anterior, a reta encurta de 180 para 145 mm.
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 145.0f, navPoseMetaRetaMm(0.0f, CELULA, lim));
}

// CENARIO 4: durante o avanco a leitura frontal encurta a meta a tempo.
void test_meta_encurta_quando_a_parede_aparece() {
  const GeometriaRobo g = medido();
  // A 80 mm de progresso o U3 le 120 mm: sobra 120-75 = 45 -> meta 125 mm.
  const float nova = navPoseMetaEncurtadaPorParedeMm(120.0f, 80.0f, 180.0f, g.u3TocaMm, FOLGA, 10.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 125.0f, nova);
}
void test_meta_nao_muda_quando_a_parede_esta_longe() {
  const GeometriaRobo g = medido();
  // Corredor livre adiante: a parede so apareceria depois da meta.
  TEST_ASSERT_TRUE(isnan(navPoseMetaEncurtadaPorParedeMm(400.0f, 10.0f, 180.0f, g.u3TocaMm, FOLGA, 10.0f)));
}
void test_meta_nao_encurta_para_tras_nem_sem_folga() {
  const GeometriaRobo g = medido();
  // Parede em cima: a meta nova ficaria atras do progresso atual -> freia por
  // obstaculo, nao por meta. Encurtar aqui mandaria o robo andar negativo.
  TEST_ASSERT_TRUE(isnan(navPoseMetaEncurtadaPorParedeMm(76.0f, 100.0f, 180.0f, g.u3TocaMm, FOLGA, 10.0f)));
  TEST_ASSERT_TRUE(isnan(navPoseMetaEncurtadaPorParedeMm(NAN, 10.0f, 180.0f, g.u3TocaMm, FOLGA, 10.0f)));
}

// CENARIO 5: o offset acumula entre celulas e a reta seguinte o desconta.
void test_offset_acumula_e_a_proxima_reta_desconta() {
  // Parou 35 mm antes do centro por causa da parede.
  const float off = navPoseOffsetAposAvancoMm(0.0f, 145.0f, CELULA);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -35.0f, off);
  // A reta seguinte anda 215 mm e termina no centro: o erro NAO se acumula.
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 215.0f, navPoseMetaRetaMm(off, CELULA, 0.0f));
  // Ajuste dentro da mesma celula nao troca de celula.
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -5.0f, navPoseOffsetAposAjusteMm(-35.0f, 30.0f));
}

// CENARIO 6: antes do giro o eixo precisa voltar ao centro da abertura.
void test_ajuste_antes_do_giro() {
  const GeometriaRobo g = medido();
  const float m = geomMeiaAberturaMm(g), u3 = geomU3AoEixoMm(g);
  // Parou 35 mm atras: avanca 35 mm para girar no centro da abertura.
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 35.0f, navPoseAjusteAntesDoGiroMm(-35.0f, NAN, 3.0f, 60.0f));
  // Passou do centro: recua.
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -12.0f, navPoseAjusteAntesDoGiroMm(12.0f, NAN, 3.0f, 60.0f));
  // Dentro da tolerancia nao mexe.
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, navPoseAjusteAntesDoGiroMm(-2.0f, NAN, 3.0f, 60.0f));
  // Com parede a frente o alvo nao pode passar do limite seguro.
  const float lim = navPoseOffsetMaxComParedeMm(m, u3, g.u3TocaMm, FOLGA); // -35
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, navPoseAjusteAntesDoGiroMm(-35.0f, lim, 3.0f, 60.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, navPoseAjusteAntesDoGiroMm(-50.0f, lim, 3.0f, 60.0f));
}

void test_plausibilidade_do_offset() {
  TEST_ASSERT_TRUE(navPoseOffsetPlausivel(-60.0f, 80.0f, 20.0f));
  TEST_ASSERT_FALSE(navPoseOffsetPlausivel(-140.0f, 80.0f, 20.0f)); // perdeu a conta
  TEST_ASSERT_FALSE(navPoseOffsetPlausivel(NAN, 80.0f, 20.0f));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_partida_com_traseira_encostada);
  RUN_TEST(test_reta_a_partir_do_centro_e_uma_celula);
  RUN_TEST(test_leitura_frontal_vira_posicao);
  RUN_TEST(test_parede_a_frente_limita_o_quanto_se_avanca);
  RUN_TEST(test_meta_encurta_quando_a_parede_aparece);
  RUN_TEST(test_meta_nao_muda_quando_a_parede_esta_longe);
  RUN_TEST(test_meta_nao_encurta_para_tras_nem_sem_folga);
  RUN_TEST(test_offset_acumula_e_a_proxima_reta_desconta);
  RUN_TEST(test_ajuste_antes_do_giro);
  RUN_TEST(test_plausibilidade_do_offset);
  return UNITY_END();
}
