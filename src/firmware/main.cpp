#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "motors.h"
#include "imu.h"
#include "lasers.h"
#include "ota_manager.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- [TESTE DE GIRO 90 GRAUS INICIADO] ---");
    Serial.println("Comandos via Serial:");
    Serial.println("  'd' -> Virar 90 graus para a DIREITA");
    Serial.println("  'e' -> Virar 90 graus para a ESQUERDA");
    Serial.println("  's' -> Parar motores de imediato");

    // Inicializa I2C padrão do ESP32 (SDA=21, SCL=22) a 400kHz
    Wire.begin();
    Wire.setClock(400000);

    initMotors();
    initIMU();
    initLasers();
    initOTA();
}

void loop() {
    handleOTA();

    if (Serial.available() > 0) {
        char comando = Serial.read();

        if (comando == '\n' || comando == '\r') return;

        if (comando == 'd' || comando == 'D') {
            Serial.println("[COMANDO] Executando giro de 90 graus a DIREITA...");
            girar90Graus(true);
        } 
        else if (comando == 'e' || comando == 'E') {
            Serial.println("[COMANDO] Executando giro de 90 graus a ESQUERDA...");
            girar90Graus(false);
        } 
        else if (comando == 's' || comando == 'S') {
            Serial.println("[COMANDO] Parada forcada dos motores.");
            stopMotors();
        } 
        else {
            Serial.printf("[AVISO] Comando desconhecido: '%c'. Use 'd', 'e' ou 's'.\n", comando);
        }
    }
}