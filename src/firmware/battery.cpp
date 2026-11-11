#include "battery.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <math.h>
static Adafruit_INA219 ina219(0x40);
static bool presente=false;
static bool primeira=true;
static float tensao=NAN,corrente=NAN,potencia=NAN;
static uint32_t ultimaLeitura=0;
static float filtrar(float antigo,float novo){return primeira||!isfinite(antigo)?novo:antigo+0.20f*(novo-antigo);}
void initBattery(){
 presente=ina219.begin();
 if(!presente){Serial.println("INA219 ausente: telemetria de energia indisponivel.");return;}
 ina219.setCalibration_32V_2A();
 Serial.println("INA219: 0x40; atencao para limite de leitura de 2A e shunt instalado.");
 ultimaLeitura=millis()-AMOSTRA_BATERIA_MS;updateBattery();
}
void updateBattery(){
 if(!presente || millis()-ultimaLeitura<AMOSTRA_BATERIA_MS)return;
 ultimaLeitura=millis();
 const float vb=ina219.getBusVoltage_V();
 const float vs=ina219.getShuntVoltage_mV();
 const float ia=ina219.getCurrent_mA();
 const float pw=ina219.getPower_mW();
 if(!isfinite(vb)||!isfinite(vs)||!isfinite(ia)||!isfinite(pw) || vb<0 || vb>32)return;
 tensao=filtrar(tensao,vb+vs*0.001f);
 corrente=filtrar(corrente,ia);
 potencia=filtrar(potencia,pw);
 primeira=false;
}
bool batteryAvailable(){return presente;}
float getVoltage(){return tensao;}
float getCurrent(){return corrente;}
float getPowerMilliwatt(){return potencia;}
