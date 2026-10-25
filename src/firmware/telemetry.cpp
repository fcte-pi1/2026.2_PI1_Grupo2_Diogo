#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "telemetry.h"
#include "config.h"

// Mutex de spinlock para sincronização thread-safe entre Core 0 (Task Telemetria) e Core 1 (Controle/Sensores)
static portMUX_TYPE telemetryMux = portMUX_INITIALIZER_UNLOCKED;
static TelemetryData currentTelemetry;
static TaskHandle_t telemetryTaskHandle = NULL;
static unsigned long tempoInicioMissao = 0;
static uint8_t falhasConsecutivas = 0;

// Buffer estático de serialização JSON para evitar alocações dinâmicas na heap durante o loop
static char jsonBuffer[1024];

void initTelemetry() {
  tempoInicioMissao = millis();

  // Inicializa estrutura de dados com valores padrão seguros
  portENTER_CRITICAL(&telemetryMux);
  currentTelemetry.execucao_id = 1;
  currentTelemetry.tempo_relativo_ms = 0;
  currentTelemetry.timestamp_envio_ms = 0;

  // Bateria e consumo elétrico (serão sobrescritos pelo INA219 via updateTelemetryBattery)
  currentTelemetry.bateria_nivel_pct = 0.0f;
  currentTelemetry.bateria_tensao_v = 0.0f;
  currentTelemetry.bateria_consumo_ma = 0.0f;

  // Cinemática e motores (N20)
  currentTelemetry.rps_esq = 0.0f;
  currentTelemetry.rps_dir = 0.0f;
  currentTelemetry.rpm_esq = 0;
  currentTelemetry.rpm_dir = 0;
  currentTelemetry.velocidade_cm_s = 0.0f;

  // IMU MPU-6050
  currentTelemetry.yaw_deg = 0.0f;
  currentTelemetry.taxa_deg_s = 0.0f;
  currentTelemetry.giro_x = 0.0f;
  currentTelemetry.giro_y = 0.0f;
  currentTelemetry.giro_z = 0.0f;

  // Lasers ToF VL53L0X (-1 indica sem leitura válida inicial)
  currentTelemetry.dist_esq_mm = -1;
  currentTelemetry.dist_fte_mm = -1;
  currentTelemetry.dist_dir_mm = -1;

  // Navegação no labirinto
  currentTelemetry.pos_x = 0;
  currentTelemetry.pos_y = 0;
  currentTelemetry.direcao = 'N';
  currentTelemetry.status_missao = "em_execucao";

  // Diagnóstico de enlace e comunicação
  currentTelemetry.latencia_rtt_ms = 0;
  currentTelemetry.pacotes_enviados = 0;
  currentTelemetry.pacotes_falhos = 0;
  currentTelemetry.ultimo_http_status = 0;
  currentTelemetry.status_servidor = "desconectado";
  currentTelemetry.conectado = false;
  portEXIT_CRITICAL(&telemetryMux);

  // Se houver rede Wi-Fi Station configurada em config.h, conecta em modo misto (AP + STA)
  if (strlen(WIFI_STA_SSID) > 0) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);
    Serial.printf("[TELEMETRIA] Conectando ao Wi-Fi Station: %s...\n", WIFI_STA_SSID);
  }

  // Cria a Task FreeRTOS dedicada no Core 0 (enquanto o controle dos motores e sensores roda no Core 1)
  xTaskCreatePinnedToCore(
      [](void* param) {
        TickType_t xLastWakeTime = xTaskGetTickCount();
        const TickType_t xFrequency = pdMS_TO_TICKS(TELEMETRY_INTERVAL_MS);

        HTTPClient http;
        http.setReuse(true); // Reutiliza socket TCP (Keep-Alive) para minimizar latência de conexão

        String url = String("http://") + TELEMETRY_SERVER_HOST + ":" + TELEMETRY_SERVER_PORT + TELEMETRY_ENDPOINT;

        while (true) {
          vTaskDelayUntil(&xLastWakeTime, xFrequency);

          // Verifica se há enlace de rede disponível (estação conectada ao SoftAP ou robô conectado ao roteador)
          bool redeDisponivel = (WiFi.status() == WL_CONNECTED) || (WiFi.softAPgetStationNum() > 0);
          if (!redeDisponivel) {
            portENTER_CRITICAL(&telemetryMux);
            currentTelemetry.conectado = false;
            currentTelemetry.status_servidor = "desconectado";
            portEXIT_CRITICAL(&telemetryMux);
            continue;
          }

          // Serializa o snapshot atual da telemetria em formato JSON compatível com o backend
          size_t len = serializeTelemetryJSON(jsonBuffer, sizeof(jsonBuffer));
          if (len == 0) continue;

          unsigned long tInicioEnvio = millis();

          http.begin(url);
          http.setTimeout(TELEMETRY_TIMEOUT_MS);
          http.addHeader("Content-Type", "application/json");

          int httpResponseCode = http.POST((uint8_t*)jsonBuffer, len);
          unsigned long rtt = millis() - tInicioEnvio;

          portENTER_CRITICAL(&telemetryMux);
          currentTelemetry.ultimo_http_status = httpResponseCode;

          if (httpResponseCode == 200 || httpResponseCode == 201) {
            // Sucesso: pacote recebido e associado à execução ativa no backend
            currentTelemetry.latencia_rtt_ms = rtt;
            currentTelemetry.pacotes_enviados++;
            currentTelemetry.conectado = true;
            currentTelemetry.status_servidor = "ativo";
            falhasConsecutivas = 0;

            // Lê opcionalmente a resposta JSON do servidor para sincronizar o execucao_id
            String resp = http.getString();
            if (resp.length() > 0 && resp.length() < 256) {
              JsonDocument respDoc;
              if (deserializeJson(respDoc, resp) == DeserializationError::Ok) {
                if (respDoc["execucao_id"].is<uint32_t>()) {
                  currentTelemetry.execucao_id = respDoc["execucao_id"].as<uint32_t>();
                }
              }
            }
          } else if (httpResponseCode == 409) {
            // HTTP 409 Conflict: O enlace de rede e o servidor FastAPI estão 100% operacionais,
            // mas o backend não possui uma execução ativa com status = 'em_execucao' no banco de dados.
            // O robô trata como conectado e medindo latência RTT, aguardando início no frontend.
            currentTelemetry.latencia_rtt_ms = rtt;
            currentTelemetry.pacotes_enviados++;
            currentTelemetry.conectado = true;
            currentTelemetry.status_servidor = "aguardando_sessao";
            falhasConsecutivas = 0;
          } else if (httpResponseCode > 0) {
            // Erro HTTP retornado pelo servidor (404 rota inexistente, 422 validação Pydantic, 500 erro interno)
            currentTelemetry.latencia_rtt_ms = rtt;
            currentTelemetry.pacotes_falhos++;
            currentTelemetry.status_servidor = "erro_http";
            falhasConsecutivas++;
            if (falhasConsecutivas >= TELEMETRY_MAX_RETRIES) {
              currentTelemetry.conectado = false;
            }
          } else {
            // httpResponseCode < 0: Falha de transporte de rede (timeout, conexão recusada, socket quebrado)
            currentTelemetry.pacotes_falhos++;
            currentTelemetry.status_servidor = "erro_rede";
            falhasConsecutivas++;
            if (falhasConsecutivas >= TELEMETRY_MAX_RETRIES) {
              currentTelemetry.conectado = false;
            }
          }
          portEXIT_CRITICAL(&telemetryMux);

          http.end();
        }
      },
      "TelemetryTask",
      4096,
      NULL,
      1,
      &telemetryTaskHandle,
      0 // Core 0 (Core de Rede/Protocolos do ESP32)
  );

  Serial.println("[TELEMETRIA] Subsistema de telemetria inicializado no Core 0.");
}

void updateTelemetryData(
    float rpsA, float rpsB,
    float yaw, float yawRate, float gx, float gy, float gz,
    int dEsq, int dFte, int dDir,
    int posX, int posY, char dir,
    const char* status
) {
  // Cálculo de velocidade linear média em cm/s
  float rpsMedio = (rpsA + rpsB) / 2.0f;
  float circRoda = 3.14159265f * DIAMETRO_RODA_CM;
  float velCmS = rpsMedio * circRoda;

  portENTER_CRITICAL(&telemetryMux);
  currentTelemetry.tempo_relativo_ms = millis() - tempoInicioMissao;
  currentTelemetry.timestamp_envio_ms = millis();

  currentTelemetry.rps_esq = rpsA;
  currentTelemetry.rps_dir = rpsB;
  currentTelemetry.rpm_esq = (int)(rpsA * 60.0f);
  currentTelemetry.rpm_dir = (int)(rpsB * 60.0f);
  currentTelemetry.velocidade_cm_s = velCmS;

  currentTelemetry.yaw_deg = yaw;
  currentTelemetry.taxa_deg_s = yawRate;
  currentTelemetry.giro_x = gx;
  currentTelemetry.giro_y = gy;
  currentTelemetry.giro_z = gz;

  currentTelemetry.dist_esq_mm = dEsq;
  currentTelemetry.dist_fte_mm = dFte;
  currentTelemetry.dist_dir_mm = dDir;

  currentTelemetry.pos_x = posX;
  currentTelemetry.pos_y = posY;
  currentTelemetry.direcao = dir;
  currentTelemetry.status_missao = status;
  portEXIT_CRITICAL(&telemetryMux);
}

void updateTelemetryBattery(float tensao_v, float consumo_ma, float nivel_pct) {
  portENTER_CRITICAL(&telemetryMux);
  currentTelemetry.bateria_tensao_v = tensao_v;
  currentTelemetry.bateria_consumo_ma = consumo_ma;
  currentTelemetry.bateria_nivel_pct = nivel_pct;
  portEXIT_CRITICAL(&telemetryMux);
}

size_t serializeTelemetryJSON(char* buffer, size_t bufferSize) {
  TelemetryData snap;
  portENTER_CRITICAL(&telemetryMux);
  snap = currentTelemetry;
  portEXIT_CRITICAL(&telemetryMux);

  JsonDocument doc;

  // =========================================================================
  // 1. CAMPOS CONSUMIDOS DIRETAMENTE PELO BACKEND (Schema: EspTelemetryInput)
  // Definidos em src/backend/app/schemas/telemetry.py:
  // - "bateria": float (alias battery_pct, restrição Pydantic: 0.0 <= x <= 100.0)
  // - "velocidade": float (alias speed_cm_s, restrição Pydantic: x >= 0.0)
  // - "rpm_motor": int (alias rpm, restrição Pydantic: x >= 0)
  // - "connected": bool (informa estado de conexão do dispositivo)
  // =========================================================================
  float nivelPct = snap.bateria_nivel_pct < 0.0f ? 0.0f : snap.bateria_nivel_pct;
  doc["bateria"] = round(constrain(nivelPct, 0.0f, 100.0f) * 10.0f) / 10.0f;
  doc["velocidade"] = round(fabsf(snap.velocidade_cm_s) * 10.0f) / 10.0f;
  doc["rpm_motor"] = (abs(snap.rpm_esq) + abs(snap.rpm_dir)) / 2;
  doc["connected"] = snap.conectado;

  // =========================================================================
  // 2. METADADOS E ESTRUTURAS AUXILIARES (ENERGIA, MOTORES, IMU, SENSORES)
  // Campos detalhados que enriquecem o diagnóstico e suportam persistência estendida:
  // =========================================================================
  doc["execucao_id"] = snap.execucao_id;
  doc["tempo_relativo_ms"] = snap.tempo_relativo_ms;
  doc["timestamp_envio_ms"] = snap.timestamp_envio_ms;

  // Energia e Alimentação (INA219 - RF-EN02, RNF-EL01)
  JsonObject energia = doc["energia"].to<JsonObject>();
  energia["tensao_v"] = round(snap.bateria_tensao_v * 100.0f) / 100.0f;
  energia["consumo_ma"] = round(snap.bateria_consumo_ma * 10.0f) / 10.0f;

  // Cinemática dos motores individuais (RF-EL02, RF-EL04)
  JsonObject mot = doc["motores"].to<JsonObject>();
  mot["rpm_esq"] = snap.rpm_esq;
  mot["rpm_dir"] = snap.rpm_dir;
  mot["velocidade_cm_s"] = round(snap.velocidade_cm_s * 10.0f) / 10.0f;

  // Unidade de Medição Inercial IMU MPU-6050 (RF-EL04)
  JsonObject imu = doc["imu"].to<JsonObject>();
  imu["yaw_deg"] = round(snap.yaw_deg * 10.0f) / 10.0f;
  imu["taxa_deg_s"] = round(snap.taxa_deg_s * 10.0f) / 10.0f;
  imu["giro_x"] = round(snap.giro_x * 10.0f) / 10.0f;
  imu["giro_y"] = round(snap.giro_y * 10.0f) / 10.0f;
  imu["giro_z"] = round(snap.giro_z * 10.0f) / 10.0f;

  // Sensores Ópticos ToF VL53L0X (RF-EL03)
  JsonObject dist = doc["sensores_distancia_mm"].to<JsonObject>();
  dist["esq"] = snap.dist_esq_mm;
  dist["frente"] = snap.dist_fte_mm;
  dist["dir"] = snap.dist_dir_mm;

  // Posição Estimada no Labirinto (RF-SW02)
  JsonObject pos = doc["posicao_estimada"].to<JsonObject>();
  pos["x"] = snap.pos_x;
  pos["y"] = snap.pos_y;
  char dirStr[2] = {snap.direcao, '\0'};
  pos["direcao"] = dirStr;

  doc["status_missao"] = snap.status_missao;

  // Diagnóstico de Enlace e Latência RTT (RNF-SW01, RF-EL06)
  JsonObject diag = doc["diagnostico"].to<JsonObject>();
  diag["latencia_ms"] = snap.latencia_rtt_ms;
  diag["pacotes_enviados"] = snap.pacotes_enviados;
  diag["pacotes_falhos"] = snap.pacotes_falhos;
  diag["ultimo_http_status"] = snap.ultimo_http_status;
  diag["status_servidor"] = snap.status_servidor;
  diag["conectado"] = snap.conectado;

  return serializeJson(doc, buffer, bufferSize);
}

uint32_t getTelemetryLatency() {
  portENTER_CRITICAL(&telemetryMux);
  uint32_t lat = currentTelemetry.latencia_rtt_ms;
  portEXIT_CRITICAL(&telemetryMux);
  return lat;
}

uint32_t getTelemetryPacketsSent() {
  portENTER_CRITICAL(&telemetryMux);
  uint32_t qtd = currentTelemetry.pacotes_enviados;
  portEXIT_CRITICAL(&telemetryMux);
  return qtd;
}

uint32_t getTelemetryPacketsLost() {
  portENTER_CRITICAL(&telemetryMux);
  uint32_t qtd = currentTelemetry.pacotes_falhos;
  portEXIT_CRITICAL(&telemetryMux);
  return qtd;
}

int getTelemetryLastHttpStatus() {
  portENTER_CRITICAL(&telemetryMux);
  int st = currentTelemetry.ultimo_http_status;
  portEXIT_CRITICAL(&telemetryMux);
  return st;
}

const char* getTelemetryServerStatus() {
  portENTER_CRITICAL(&telemetryMux);
  const char* st = currentTelemetry.status_servidor;
  portEXIT_CRITICAL(&telemetryMux);
  return st;
}

bool isTelemetryConnected() {
  portENTER_CRITICAL(&telemetryMux);
  bool c = currentTelemetry.conectado;
  portEXIT_CRITICAL(&telemetryMux);
  return c;
}
