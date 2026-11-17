#pragma once
#include <Arduino.h>
#include <stdint.h>
// Maquina de estados cooperativa. Nunca mover a partir de dados de parede indefinidos.
struct NavigationOptions {
 bool reverse=false, center=false, opening=false;
 float corridorWidth=0, sensorLeftToAxle=-999, sensorRightToAxle=-999;
 char initialDirection='N'; // N/L/S/O no referencial do mapa
};
NavigationOptions navigationDefaultOptions();
bool navigationReverseAllowed();
bool navigationCenterEnabled();
const char* navigationStartWithOptions(uint8_t width,uint8_t height,uint8_t goalX,uint8_t goalY,const NavigationOptions& options);
const char* navigationStart(uint8_t width,uint8_t height,uint8_t goalX,uint8_t goalY);
void navigationTick();
void navigationNotifyStop(const char* reason); // so altera estado; NAO mexe no driver
bool navigationRunning();
bool navigationNeedsFrontPoll();
String navigationStatusJson();
