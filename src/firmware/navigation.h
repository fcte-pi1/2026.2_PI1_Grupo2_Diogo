#pragma once
#include <Arduino.h>
#include <stdint.h>
// Maquina de estados cooperativa. Nunca mover a partir de dados de parede indefinidos.
const char* navigationStart(uint8_t width,uint8_t height,uint8_t goalX,uint8_t goalY);
void navigationTick();
void navigationNotifyStop(const char* reason); // so altera estado; NAO mexe no driver
bool navigationRunning();
bool navigationNeedsFrontPoll();
String navigationStatusJson();
