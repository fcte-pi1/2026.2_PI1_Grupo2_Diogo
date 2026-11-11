#pragma once
void initBattery();
void updateBattery();
bool batteryAvailable();
float getVoltage();   // V na bateria: bus + shunt, se leitura valida
float getCurrent();   // corrente mA (calibracao INA219 2A, verificar no hardware)
float getPowerMilliwatt();
