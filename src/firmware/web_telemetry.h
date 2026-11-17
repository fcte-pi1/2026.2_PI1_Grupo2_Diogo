#pragma once
#include <stdint.h>
void setupTelemetry();
void loopTelemetry();
void pararTesteDeEmergencia(const char* motivo);

// Prevent diagnostic VL53L0X polling from affecting timing of motor control.
bool testeMotorAtivo();
// Protecao U3 em ensaio reto, ainda nao homologada.
bool frenteEmMonitoramento();
bool lateraisEmMonitoramento();

// Ponte de controle para o modulo de navegacao. Preserva o PI, o freio e os CPRs
// atualmente validados; comandos manuais ficam bloqueados durante a missao.
bool navigationDriveReady();
bool navigationDriveStartStraight(float millimeters,float rpm);
float navigationDriveProgressMm();
float navigationDriveCenterRefMm();
bool navigationDriveAdjustStraight(float millimeters);
void navigationDriveSetCenterRef(float mm);
float navigationDriveCenterRefEsqMm();
float navigationDriveCenterRefDirMm();
void navigationDriveSetCenterRefs(float esquerdaMm,float direitaMm);
// Para por parede sem notificar uma falha externa; navegacao verifica a pose depois.
bool navigationDriveStopAtWall();
bool navigationDriveSettledStraight(float& progressMm,float& syncMm);
bool navigationDriveStartTurn(bool right,float rpm);
uint32_t navigationDriveResultId();
bool navigationDriveResultOk();
const char* navigationDriveResultReason();
bool navigationDriveIsActive();
