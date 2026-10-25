#pragma once

#include <Arduino.h>

/**
 * @file telemetry.h
 * @brief Subsistema unificado de telemetria do Micromouse via Wi-Fi / HTTP.
 *
 * Arquitetura de integração com a estação base:
 * 1. O firmware executa uma task FreeRTOS dedicada no Core 0 (espaço de rede),
 *    enquanto o loop de controle de motores e sensores roda no Core 1.
 * 2. O payload é compatível diretamente com o backend FastAPI (rota POST /api/telemetry),
 *    alimentando o schema EspTelemetryInput (campos planos: bateria, velocidade, rpm_motor, connected)
 *    e disponibilizando estruturas detalhadas para diagnóstico, persistência relacional e visualização.
 * 3. Monitoramento contínuo de latência RTT (Round-Trip Time) e qualidade de enlace
 *    (atendendo a RNF-SW01 e RF-EL06).
 */
struct TelemetryData {
  uint32_t execucao_id;
  uint32_t tempo_relativo_ms;
  uint64_t timestamp_envio_ms;

  // Bateria e Alimentação (RF-EN02, RNF-EL01)
  float bateria_nivel_pct;
  float bateria_tensao_v;
  float bateria_consumo_ma;

  // Motores e Odometria (RF-EL02, RF-EL04)
  float rps_esq;
  float rps_dir;
  int rpm_esq;
  int rpm_dir;
  float velocidade_cm_s;

  // Unidade Inercial IMU (RF-EL04)
  float yaw_deg;
  float taxa_deg_s;
  float giro_x;
  float giro_y;
  float giro_z;

  // Sensores Ópticos ToF (RF-EL03)
  int dist_esq_mm;
  int dist_fte_mm;
  int dist_dir_mm;

  // Posição no Labirinto (RF-SW02)
  int pos_x;
  int pos_y;
  char direcao; // 'N', 'S', 'L', 'O'

  // Status da Missão
  const char* status_missao; // "em_execucao", "concluido", "interrompido"

  // Monitoramento de Latência, HTTP e Qualidade de Enlace (RNF-SW01, RF-EL06)
  uint32_t latencia_rtt_ms;
  uint32_t pacotes_enviados;
  uint32_t pacotes_falhos;
  int ultimo_http_status;      // Código HTTP recebido (ex.: 200 gravado, 409 aguardando sessão, etc.)
  const char* status_servidor; // "ativo", "aguardando_sessao", "erro_http", "desconectado"
  bool conectado;
};

/**
 * @brief Inicializa o subsistema de telemetria e cria a task FreeRTOS
 * dedicada no Core 0 do ESP32 para transmissão assíncrona não-bloqueante.
 */
void initTelemetry();

/**
 * @brief Atualiza os valores do estado de telemetria de forma thread-safe.
 * Deve ser chamado periodicamente no loop principal do Core 1.
 */
void updateTelemetryData(
    float rpsA, float rpsB,
    float yaw, float yawRate, float gx, float gy, float gz,
    int dEsq, int dFte, int dDir,
    int posX = 0, int posY = 0, char dir = 'N',
    const char* status = "em_execucao"
);

/**
 * @brief Atualiza leituras de monitoramento elétrico da bateria (INA219).
 */
void updateTelemetryBattery(float tensao_v, float consumo_ma, float nivel_pct);

/**
 * @brief Serializa os dados atuais em JSON padronizado compatível com o backend.
 * @param buffer Ponteiro para o buffer de caracteres de saída.
 * @param bufferSize Tamanho máximo do buffer.
 * @return Número de bytes escritos no buffer.
 */
size_t serializeTelemetryJSON(char* buffer, size_t bufferSize);

/**
 * @brief Métricas de diagnóstico de latência e enlace.
 */
uint32_t getTelemetryLatency();
uint32_t getTelemetryPacketsSent();
uint32_t getTelemetryPacketsLost();
int getTelemetryLastHttpStatus();
const char* getTelemetryServerStatus();
bool isTelemetryConnected();
