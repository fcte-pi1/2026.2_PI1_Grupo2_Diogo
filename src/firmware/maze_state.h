#pragma once
#include <Arduino.h>
#include <stdint.h>
#include "maze_planner.h"
// N=1 L=2 S=4 O=8; y aumenta em direcao ao norte.
void mazeReset();
bool mazeConfigure(uint8_t width,uint8_t height,uint8_t goalX,uint8_t goalY);
bool mazeSetPose(uint8_t x, uint8_t y, char direcao);
bool mazeGetPose(uint8_t& x,uint8_t& y,char& direction);
bool mazeObserve(uint8_t x, uint8_t y, uint8_t conhecidas, uint8_t presentes);
// Versao tolerante para a navegacao: leitura nova prevalece (ver Grid::update).
// 'inferidas' sao faces sem evidencia do sensor que so sao gravadas como
// parede se ainda forem desconhecidas no mapa (nunca apagam uma observacao real).
bool mazeUpdate(uint8_t x, uint8_t y, uint8_t conhecidas, uint8_t presentes, uint8_t inferidas);
bool mazeMarkVisited(uint8_t x, uint8_t y);
bool mazeIsGoal();
bool mazeNextDirection(char& direction,uint8_t& nx,uint8_t& ny);
bool mazeOpenFromPose(char direction);
bool mazeTraversedFromPose(char direction);
bool mazeMarkTraversedFromPose(char direction);
bool mazeOpenAt(uint8_t x,uint8_t y,char direction);
void mazeInvalidatePose();
String mazeEstadoJson();
