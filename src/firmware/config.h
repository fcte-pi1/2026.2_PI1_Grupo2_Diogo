#pragma once

// Driver TB6612FNG
#define PIN_PWMA  15
#define PIN_AIN1  2
#define PIN_AIN2  4
#define PIN_PWMB  25
#define PIN_BIN1  26
#define PIN_BIN2  27
#define PIN_STBY  5

// Encoders N20
#define ENC_A_C1  32
#define ENC_A_C2  33
#define ENC_B_C1  14
#define ENC_B_C2  13

// Sensores VL53L0X (Pinos XSHUT)
#define XSHUT_ESQ 23  // U2
#define XSHUT_FTE 18  // U3
#define XSHUT_DIR 19  // U4

// Endereços I2C dos lasers
#define ADDR_LASER_ESQ 0x30
#define ADDR_LASER_FTE 0x31
#define ADDR_LASER_DIR 0x32

// Constantes mecânicas
#define PULSOS_POR_VOLTA 210.0f
#define DIAMETRO_RODA_CM 3.4f

// Configurações de Telemetria e Rede
#define TELEMETRY_INTERVAL_MS   100   // Taxa de 10 Hz (intervalo <= 1s, atende RNF-SW01)
#define TELEMETRY_SERVER_HOST   "192.168.4.2" // IP padrão da estação base conectada ao SoftAP do robô
#define TELEMETRY_SERVER_PORT   8000
#define TELEMETRY_ENDPOINT      "/api/telemetria"
#define TELEMETRY_TIMEOUT_MS    80    // Timeout de envio HTTP para não sobrecarregar conexão
#define TELEMETRY_MAX_RETRIES   3     // Tentativas antes de marcar desconectado

// Opcional: Wi-Fi em modo Station (conectar a uma rede externa ou roteador)
#define WIFI_STA_SSID           ""
#define WIFI_STA_PASSWORD       ""