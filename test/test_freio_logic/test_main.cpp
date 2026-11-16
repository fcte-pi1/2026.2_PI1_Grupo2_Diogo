#include <unity.h>
#include "freio_logic.h"

void setUp() {}
void tearDown() {}

void test_fim_normal_freia() {
  const char* freia[] = {"posicao_meta_atingida", "reto_meta_atingida", "reto_chegada_com_dessincronia",
                         "giro_encoder_concluido", "giro_imu_concluido", "giro_fim_com_dessincronia",
                         "imu_alvo_com_dessincronia", "giro_dentro_da_tolerancia",
                         "giro_corrigido_dentro_da_tolerancia", "giro_correcoes_esgotadas"};
  for (const char* m : freia) TEST_ASSERT_TRUE_MESSAGE(paradaUsaFreioAtivo(m, false), m);
}

void test_laser_frontal_freia() {
  TEST_ASSERT_TRUE(paradaUsaFreioAtivo("frontal_U3_obstaculo_detectado", false));
  TEST_ASSERT_TRUE(paradaUsaFreioAtivo("frontal_U3_parede_estimada_pelo_percurso", false));
  TEST_ASSERT_TRUE(paradaUsaFreioAtivo("frontal_U3_leitura_invalida_ou_antiga", false));
}

void test_emergencia_corta_por_stby() {
  const char* corta[] = {"bateria_critica_corte", "watchdog_conexao_perdida", "parado_pela_pagina",
                         "parado_por_usb", "ota_iniciada"};
  for (const char* m : corta) TEST_ASSERT_FALSE_MESSAGE(paradaUsaFreioAtivo(m, true), m);
}

void test_falhas_cortam_por_stby() {
  const char* corta[] = {"atraso_loop_controle", "dessincronia_limite",
                         "encoder_ou_motor_direito_sem_movimento", "encoder_ou_motor_esquerdo_sem_movimento",
                         "giro_encoder_limite_curso", "giro_sentido_encoder_incorreto",
                         "imu_angulo_limite_excedido", "imu_indisponivel_ou_atrasada",
                         "imu_limite_encoder_excedido", "imu_sentido_inverso_conferir_montagem",
                         "reto_dessincronia_limite", "reto_limite_curso", "reto_sentido_encoder_invertido",
                         "sinal_encoder_direito_invertido", "sinal_encoder_esquerdo_invertido",
                         "giro_erro_grande_demais_para_corrigir", "giro_sem_correcao_imu_indisponivel",
                         "giro_sem_correcao_robo_nao_parou", "giro_correcao_nao_iniciou"};
  for (const char* m : corta) TEST_ASSERT_FALSE_MESSAGE(paradaUsaFreioAtivo(m, false), m);
}

void test_motivo_desconhecido_corta_por_stby() {
  TEST_ASSERT_FALSE(paradaUsaFreioAtivo("motivo_novo", true));
  TEST_ASSERT_FALSE(paradaUsaFreioAtivo("", true));
  TEST_ASSERT_FALSE(paradaUsaFreioAtivo(nullptr, true));
  TEST_ASSERT_FALSE(paradaUsaFreioAtivo("reto_meta_atingida_x", false));
}

void test_limite_de_tempo_so_freia_em_teste_temporizado() {
  TEST_ASSERT_TRUE(paradaUsaFreioAtivo("limite_de_tempo", true));
  TEST_ASSERT_FALSE(paradaUsaFreioAtivo("limite_de_tempo", false));
}

void test_solta_depois_de_parado() {
  LiberacaoFreio l(100, 600);
  l.iniciar(1000, 50, 60);
  TEST_ASSERT_FALSE(l.podeSoltar(1040, 52, 61));  // ainda girando
  TEST_ASSERT_FALSE(l.podeSoltar(1100, 52, 61));
  TEST_ASSERT_TRUE(l.podeSoltar(1140, 52, 61));   // 100 ms sem pulsos
}

void test_solta_no_tempo_maximo_mesmo_com_pulsos() {
  LiberacaoFreio l(100, 600);
  l.iniciar(1000, 0, 0);
  bool soltou = false;
  int64_t c = 0;
  for (uint32_t t = 1020; t <= 1600 && !soltou; t += 20) soltou = l.podeSoltar(t, ++c, c);
  TEST_ASSERT_TRUE(soltou);
}

void test_solta_no_estouro_do_millis() {
  LiberacaoFreio l(100, 600);
  l.iniciar(0xFFFFFFC0u, 10, 10);
  TEST_ASSERT_FALSE(l.podeSoltar(0xFFFFFFF0u, 10, 10));
  TEST_ASSERT_TRUE(l.podeSoltar(0x00000030u, 10, 10));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_fim_normal_freia);
  RUN_TEST(test_laser_frontal_freia);
  RUN_TEST(test_emergencia_corta_por_stby);
  RUN_TEST(test_falhas_cortam_por_stby);
  RUN_TEST(test_motivo_desconhecido_corta_por_stby);
  RUN_TEST(test_limite_de_tempo_so_freia_em_teste_temporizado);
  RUN_TEST(test_solta_depois_de_parado);
  RUN_TEST(test_solta_no_tempo_maximo_mesmo_com_pulsos);
  RUN_TEST(test_solta_no_estouro_do_millis);
  return UNITY_END();
}
