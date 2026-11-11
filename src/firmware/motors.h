#pragma once
#include <Arduino.h>

void setupMotors();
bool motorHardwareReady();
bool ponteHabilitada();      // status do comando STBY do firmware
void prepararMotoresControle(); // zera PWM/direcao e coloca STBY HIGH (sem movimento)
void setMotorEsq(int pwm, bool frente);
void setMotorDir(int pwm, bool frente);
void pararMotores();          // DESABILITA STBY (standby / coast)
void frearMotorEsq();         // PWM 0, roda esquerda em coast
void frearMotorDir();         // PWM 0, roda direita em coast
void frearMotoresTudo();      // ambas desabilitadas (STBY LOW)
