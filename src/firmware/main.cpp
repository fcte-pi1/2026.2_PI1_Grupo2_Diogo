#include <Arduino.h>
#include <Wire.h>
#include <WebServer.h>
#include "config.h"
#include "motors.h"
#include "encoders.h"
#include "battery.h"
#include "imu.h"
#include "i2c_diagnostics.h"
#include "lasers.h"
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
    // V4.4: manter os tres sensores em shutdown durante o scan inicial.
    // VL53L0X ainda não usados na V4: manter os 3 XSHUT em LOW.
    // Isso evita 3 sensores responderem simultaneamente no endereco 0x29.
    // Ao implementar os lasers, remover este isolamento e reendereçar um a um.
    const uint8_t xshut[] = {23, 18, 19};
    for (uint8_t gpio : xshut) {
        digitalWrite(gpio, LOW);
        pinMode(gpio, OUTPUT);
    }
    Wire.begin(PIN_SDA, PIN_SCL);
    // Diagnostico: 100 kHz e mais tolerante a cabos/capacitancia.
    // Subir para 400 kHz apenas apos validar o barramento fisico.
    Wire.setClock(100000);
    delay(150); // aguarda a alimentacao do modulo estabilizar
    imprimirDiagnosticoI2C();
    initBattery();
    iniciarIMU();
    initLasers(); // inicializacao sequencial U2/U3/U4, enderecos 0x30..0x32
    Serial.println("[LASER] I2C apos ativacao U2/U3/U4:");
    Serial.println(diagnosticoI2CJson().c_str());
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
    atualizarIMU();
    atualizarEncoders();
    if (!testeMotorAtivo()) atualizarLasers(); // prioriza controle de motores; diagnose lasers somente com motores parados
    updateBattery();
    loopTelemetry();   // deadlines de PWM e watchdog não bloqueantes
    server.handleClient();
    handleOTA();
    while (Serial.available()) {
        char c = static_cast<char>(Serial.read());
        if (c == 's' || c == 'S') pararTesteDeEmergencia("parado_por_usb");
    }
}
