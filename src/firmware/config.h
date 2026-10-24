#pragma once
#include <Arduino.h>

// === PINOS ORIGINAIS DO SEU MICROMOUSE (mantidos) ===
// CANAL A (PWMA/AIN1/AIN2) = roda fisica DIREITA
// CANAL B (PWMB/BIN1/BIN2) = roda fisica ESQUERDA
// ENCODER ESQ (32/33) = roda esquerda; ENCODER DIR (14/13) = roda direita
#define PIN_STBY 5
#define PIN_PWMA 15
#define PIN_AIN1 2
#define PIN_AIN2 4
#define ENC_ESQ_A 32
#define ENC_ESQ_B 33
#define PIN_PWMB 25
#define PIN_BIN1 26
#define PIN_BIN2 27
#define ENC_DIR_A 14
#define ENC_DIR_B 13

#define PIN_SDA 21
#define PIN_SCL 22

// Pelos testes CSV anteriores, durante a marcha a re no firmware antigo:
// ENC_ESQ diminuia, ENC_DIR aumentava. Ao inverter ambos os motores,
// esperamos ambos os encoders POSITIVOS ao andar para FRENTE com o
// sinal da direita invertido. CONFIRMAR em teste com rodas suspensas.
#define SINAL_ENCODER_ESQ 1
#define SINAL_ENCODER_DIR -1

// Valores geométricos NOMINAIS: NÃO são calibração concluída.
constexpr float DIAMETRO_RODA_MM = 34.0f;
constexpr float ENTRE_RODAS_MM = 100.0f;

constexpr unsigned int PWM_FREQUENCIA_HZ = 10000;
constexpr unsigned int PWM_RESOLUCAO_BITS = 8;
constexpr int PWM_LIMITE_TESTE = 180;  // teto provisório de segurança
constexpr uint32_t AMOSTRA_ENCODER_MS = 20;
constexpr uint32_t PERIODO_CONTROLE_MS = 20;
constexpr uint32_t AMOSTRA_BATERIA_MS = 750;
constexpr uint32_t TESTE_DURACAO_MAX_MS = 10000;
constexpr uint32_t TESTE_DURACAO_MIN_MS = 250;
constexpr uint32_t WATCHDOG_WEB_MS = 1200;

// V3.1: limites de bancada; primeiro com rodas suspensas; solo somente em area livre.
constexpr float RPM_MAX_TESTE = 120.0f;
constexpr float RPM_MAX_POSICAO = 75.0f;
constexpr float RPM_ACELERACAO_POR_SEG = 160.0f;
constexpr float REV_DESACEL_POR_SEG2 = 2.4f;
constexpr int PWM_LIMITE_CONTROLE = 130;
// Na V3 o piso fixo de 35 impedia regular 40 RPM sem carga.
// V3.1 permite duty abaixo de 35, com ajuda TEMPORARIA de partida.
constexpr int PWM_MINIMO_CONTROLE = 0;
constexpr int PWM_AJUDA_PARTIDA = 65;
constexpr int PWM_AJUDA_PARTIDA_CURTA = 55;
constexpr int PWM_AJUDA_APROXIMACAO = 43;
constexpr uint32_t PARTIDA_SEM_PULSOS_MS = 140;
constexpr uint32_t PARTIDA_AJUDA_ATE_MS = 850;
constexpr float KP_RPM = 0.40f;                 // parametros experimentais
constexpr float KI_RPM = 1.25f;                 // PWM/(RPM*s)
constexpr float FF_PWM_BASE = 8.0f;
constexpr float FF_PWM_POR_RPM = 0.44f;
constexpr float LIMITE_INTEGRAL_PWM = 70.0f;
constexpr int POSICAO_TOLERANCIA_CONTAGENS = 4;
constexpr int POSICAO_META_MAX_CONTAGENS = 2000;
constexpr uint32_t CONTROLE_STALL_MS = 1200;
constexpr uint32_t FREIO_MEDICAO_FINAL_MS = 1000;

constexpr char WIFI_SSID[] = "Xaropinho-WiFi";
constexpr char WIFI_PASSWORD[] = "pi12pi34";

// Por segurança, o OTA ficará DESLIGADO enquanto esta senha não for alterada.
// Coloque uma senha forte e diferente da senha Wi-Fi para habilitar o OTA.
constexpr char OTA_PASSWORD[] = "DEFINA_SENHA_OTA_AQUI";
