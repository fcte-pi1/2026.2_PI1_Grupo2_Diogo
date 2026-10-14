#include <Arduino.h>
#include <Wire.h>
#include <WebServer.h> // Adicionado para controlo via rede
#include "config.h"
#include "motors.h"
#include "imu.h"
#include "lasers.h"
#include "ota_manager.h"

WebServer server(80); // Servidor HTTP na porta padrão 80

void setup() {
    Serial.begin(115200);
    delay(1000);

    Wire.begin();
    Wire.setClock(400000);

    initMotors();
    initIMU();
    initLasers();
    initOTA(); // Cria o AP Xaropinho-WiFi no IP 192.168.4.1

    // Configuração dos endpoints de comando via Wi-Fi
    server.on("/direita", []() {
        server.send(200, "text/plain", "Girando 90 graus a DIREITA");
        girar90Graus(true);
    });

    server.on("/esquerda", []() {
        server.send(200, "text/plain", "Girando 90 graus a ESQUERDA");
        girar90Graus(false);
    });

    server.on("/parar", []() {
        stopMotors();
        server.send(200, "text/plain", "Motores parados");
    });

    server.begin();
    Serial.println("[HTTP] Servidor Web ativo em http://192.168.4.1");
}

void loop() {
    handleOTA();
    server.handleClient(); // Processa pedidos recebidos por Wi-Fi

    // Mantém a escuta por cabo USB ativa em paralelo
    if (Serial.available() > 0) {
        char comando = Serial.read();
        if (comando == 'd' || comando == 'D') girar90Graus(true);
        if (comando == 'e' || comando == 'E') girar90Graus(false);
        if (comando == 's' || comando == 'S') stopMotors();
    }
}