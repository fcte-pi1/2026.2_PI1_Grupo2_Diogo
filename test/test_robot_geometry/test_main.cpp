#include <unity.h>
#include "robot_geometry.h"

// Medidas reais do Xaropinho (18/11/2026), as mesmas que vao no config.h.
static GeometriaRobo medido() {
  GeometriaRobo g;
  g.larguraMm = 110.0f;
  g.comprimentoMm = 120.0f;
  g.lateralAoNarizMm = 40.0f;
  g.u3AoNarizMm = 60.0f;
  g.u3TocaMm = 60.0f;
  g.bordaAteGiroFrenteMm = 140.0f;
  g.bordaAteGiroReMm = 20.0f;
  return g;
}

void setUp() {}
void tearDown() {}

// Os dois ensaios no labirinto precisam devolver s e m separados.
void test_dois_ensaios_separam_sensor_e_corredor() {
  const GeometriaRobo g = medido();
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 60.0f, geomLateralAoEixoMm(g));  // (140-20)/2
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 80.0f, geomMeiaAberturaMm(g));   // (140+20)/2
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 160.0f, geomLarguraCorredorMm(g));
}

// Ida e volta: a geometria derivada tem de reproduzir os ensaios de origem.
void test_derivacao_reproduz_os_ensaios() {
  const GeometriaRobo g = medido();
  const float s = geomLateralAoEixoMm(g), m = geomMeiaAberturaMm(g);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, g.bordaAteGiroFrenteMm, m + s);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, g.bordaAteGiroReMm, m - s);
}

void test_posicao_do_eixo_e_do_u3() {
  const GeometriaRobo g = medido();
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, geomEixoAoNarizMm(g));  // 40 + 60
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, geomEixoATraseiraMm(g)); // 120 - 100
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 40.0f, geomU3AoEixoMm(g));      // 100 - 60
}

// O eixo fica atras: o nariz varre mais que a meia largura do corredor.
// O firmware precisa SABER disso, em vez de descobrir batendo.
void test_raio_de_giro_nao_cabe_no_corredor_deste_robo() {
  const GeometriaRobo g = medido();
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 114.02f, geomRaioGiroMm(g)); // hypot(100,55)
  TEST_ASSERT_FALSE(geomGiroCabeNoCorredor(g));
}

void test_robo_compacto_gira_no_corredor() {
  GeometriaRobo g = medido();
  g.comprimentoMm = 100.0f;
  g.lateralAoNarizMm = 0.0f;       // laser no nariz
  g.bordaAteGiroFrenteMm = 130.0f; // s = 50 -> eixo a 50 do nariz
  g.bordaAteGiroReMm = 30.0f;      // m = 80
  g.larguraMm = 90.0f;
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 67.27f, geomRaioGiroMm(g)); // hypot(50,45)
  TEST_ASSERT_TRUE(geomGiroCabeNoCorredor(g));
}

void test_validacao_recusa_medidas_incoerentes() {
  TEST_ASSERT_TRUE(geomValida(medido()));
  GeometriaRobo invertido = medido();
  invertido.bordaAteGiroFrenteMm = 20.0f;  // frente < re: ensaios trocados
  invertido.bordaAteGiroReMm = 140.0f;
  TEST_ASSERT_FALSE(geomValida(invertido));
  GeometriaRobo curto = medido();
  curto.comprimentoMm = 80.0f;             // eixo cairia fora do chassi
  TEST_ASSERT_FALSE(geomValida(curto));
  GeometriaRobo largo = medido();
  largo.bordaAteGiroFrenteMm = 400.0f;     // corredor absurdo
  TEST_ASSERT_FALSE(geomValida(largo));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_dois_ensaios_separam_sensor_e_corredor);
  RUN_TEST(test_derivacao_reproduz_os_ensaios);
  RUN_TEST(test_posicao_do_eixo_e_do_u3);
  RUN_TEST(test_raio_de_giro_nao_cabe_no_corredor_deste_robo);
  RUN_TEST(test_robo_compacto_gira_no_corredor);
  RUN_TEST(test_validacao_recusa_medidas_incoerentes);
  return UNITY_END();
}
