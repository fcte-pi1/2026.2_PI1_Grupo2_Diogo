#pragma once
#include <Arduino.h>
#include <stdint.h>
// Maquina de estados cooperativa. Nunca mover a partir de dados de parede indefinidos.
struct NavigationOptions {
 bool reverse=false, center=false, opening=false;
 float corridorWidth=0, sensorLeftToAxle=-999, sensorRightToAxle=-999;
 char initialDirection='N'; // N/L/S/O no referencial do mapa
 // V4.9.1: celula inicial configuravel (antes sempre (0,0)); origem no canto sudoeste.
 uint8_t startX=0, startY=0;
 // V5.0: o robo e posto no labirinto com a TRASEIRA ENCOSTADA na parede, nao
 // no centro do quadrado. Com true a navegacao ja parte sabendo que o eixo
 // esta atras do centro e a primeira reta anda o tanto a mais.
 bool startBackToWall=true;
};
NavigationOptions navigationDefaultOptions();
bool navigationReverseAllowed();
bool navigationCenterEnabled();
const char* navigationStartWithOptions(uint8_t width,uint8_t height,uint8_t goalX,uint8_t goalY,const NavigationOptions& options);
const char* navigationStart(uint8_t width,uint8_t height,uint8_t goalX,uint8_t goalY);
void navigationTick();
void navigationNotifyStop(const char* reason); // so altera estado; NAO mexe no driver
bool navigationRunning();
bool navigationCompleted(); // observacao somente leitura para o buzzer
bool navigationNeedsFrontPoll();
String navigationStatusJson();
