#pragma once
void setupTelemetry();
void loopTelemetry();
void pararTesteDeEmergencia(const char* motivo);

// Prevent diagnostic VL53L0X polling from affecting timing of motor control.
bool testeMotorAtivo();
// Protecao U3 em ensaio reto, ainda nao homologada.
bool frenteEmMonitoramento();
bool lateraisEmMonitoramento();
