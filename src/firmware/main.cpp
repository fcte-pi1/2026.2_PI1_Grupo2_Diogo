#include <Arduino.h>
#include <Wire.h>
#include <WebServer.h>
#include "config.h"
#include "motors.h"
#include "encoders.h"
#include "battery.h"
#include "ota_manager.h"
#include "web_telemetry.h"

WebServer server(80);

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== MICROMOUSE | BANCADA DE CALIBRACAO ===");
    setupMotors(); // mantém motores parados desde a inicialização
    if (!motorHardwareReady()) {
        Serial.println("FALHA FATAL NO PWM. Desabilitando drivers.");
        while (true) { delay(1000); }
    }
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);
    initBattery();
    bool ok = iniciarEncoders();
    if (!ok) {
        Serial.println("FALHA FATAL NO PCNT. Reinicie após conferir os pinos.");
        while (true) { frearMotoresTudo(); delay(1000); }
    }
    initOTA();
    setupTelemetry(); // TODAS as rotas antes do begin()
    server.begin();
    Serial.println("Painel: http://192.168.4.1/");
    Serial.println("Serial: digite 's' para frear imediatamente.");
}

void loop() {
    atualizarEncoders();
    updateBattery();
    loopTelemetry();   // deadlines de PWM e watchdog não bloqueantes
    server.handleClient();
    handleOTA();
    while (Serial.available()) {
        char c = static_cast<char>(Serial.read());
        if (c == 's' || c == 'S') pararTesteDeEmergencia("parado_por_usb");
    }
}
