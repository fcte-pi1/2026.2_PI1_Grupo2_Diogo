#pragma once
#include <Arduino.h>

void setupMotors();
bool motorHardwareReady();
void setMotorEsq(int pwm, bool frente);
void setMotorDir(int pwm, bool frente);
void pararMotores();          // modo livre / coast
void frearMotorEsq();         // short brake TB6612
void frearMotorDir();
void frearMotoresTudo();
