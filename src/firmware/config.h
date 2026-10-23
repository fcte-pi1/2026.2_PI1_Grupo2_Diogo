#pragma once
#include <Arduino.h>

// === PINOS ORIGINAIS DO SEU MICROMOUSE (mantidos) ===
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

// Ajuste para -1 se um encoder contar ao contrário do desejado.
// Isso não modifica os fios nem o sentido físico do motor.
#define SINAL_ENCODER_ESQ 1
#define SINAL_ENCODER_DIR 1

// Valores geométricos NOMINAIS: NÃO são calibração concluída.
constexpr float DIAMETRO_RODA_MM = 34.0f;
constexpr float ENTRE_RODAS_MM = 100.0f;

constexpr unsigned int PWM_FREQUENCIA_HZ = 10000;
constexpr unsigned int PWM_RESOLUCAO_BITS = 8;
constexpr int PWM_LIMITE_TESTE = 180;  // teto provisório de segurança
constexpr uint32_t AMOSTRA_ENCODER_MS = 50;
constexpr uint32_t AMOSTRA_BATERIA_MS = 750;
constexpr uint32_t TESTE_DURACAO_MAX_MS = 10000;
constexpr uint32_t TESTE_DURACAO_MIN_MS = 250;
constexpr uint32_t WATCHDOG_WEB_MS = 1200;

constexpr char WIFI_SSID[] = "Xaropinho-WiFi";
constexpr char WIFI_PASSWORD[] = "pi12pi34";

// Por segurança, o OTA ficará DESLIGADO enquanto esta senha não for alterada.
// Coloque uma senha forte e diferente da senha Wi-Fi para habilitar o OTA.
constexpr char OTA_PASSWORD[] = "DEFINA_SENHA_OTA_AQUI";
