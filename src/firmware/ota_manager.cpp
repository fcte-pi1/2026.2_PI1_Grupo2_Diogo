#include "ota_manager.h"
#include "config.h"
#include "motors.h"
#include "web_telemetry.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <cstring>

static bool otaAtivo = false;

void initOTA() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192,168,4,1),
                      IPAddress(192,168,4,1),
                      IPAddress(255,255,255,0));
    if (!WiFi.softAP(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("ERRO: nao foi possivel iniciar AP Wi-Fi.");
        return;
    }
    Serial.printf("AP %s, IP %s\n", WIFI_SSID,
                  WiFi.softAPIP().toString().c_str());

    if (strcmp(OTA_PASSWORD, "DEFINA_SENHA_OTA_AQUI") == 0 ||
        strlen(OTA_PASSWORD) < 12) {
        Serial.println("OTA DESABILITADO: defina OTA_PASSWORD (12+ caracteres) em config.h.");
        return;
    }
    ArduinoOTA.setHostname("micromouse-calibracao");
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.onStart([]() {
        pararTesteDeEmergencia("ota_iniciada");
        Serial.println("OTA: controle cancelado e motores frenados.");
    });
    ArduinoOTA.onEnd([]() { Serial.println("OTA concluido."); });
    ArduinoOTA.onError([](ota_error_t e) {
        Serial.printf("Falha no OTA: %u\n", static_cast<unsigned>(e));
    });
    ArduinoOTA.begin();
    otaAtivo = true;
    Serial.println("OTA habilitado com senha.");
}

void handleOTA() {
    if (otaAtivo) ArduinoOTA.handle();
}
