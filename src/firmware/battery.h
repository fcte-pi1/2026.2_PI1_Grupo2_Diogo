#pragma once
void initBattery();
void updateBattery();
bool batteryAvailable();
float getVoltage();  // tensão do barramento do INA219 [V]
float getCurrent();  // corrente calibrada pelo INA219 [mA]
