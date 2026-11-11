#pragma once
#include <Arduino.h>
// I2C compartilha GPIO21 (SDA) e GPIO22 (SCL).
// Chamar a varredura com motores parados e fora de calibração.
void imprimirDiagnosticoI2C();
String diagnosticoI2CJson();
