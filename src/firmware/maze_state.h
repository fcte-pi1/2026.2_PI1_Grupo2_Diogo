#pragma once
#include <Arduino.h>
#include <stdint.h>
// Contrato futuro do modulo de mapeamento: 16x16, N=1 E=2 S=4 O=8.
// A posicao so e publicada quando explicitamente informada pela navegacao.
// Nao inferir paredes de leituras VL53L0X nao calibradas.
void mazeReset();
bool mazeSetPose(uint8_t x, uint8_t y, char direcao);
bool mazeObserve(uint8_t x, uint8_t y, uint8_t paredesConhecidas, uint8_t paredesPresentes);
bool mazeMarkVisited(uint8_t x, uint8_t y);
String mazeEstadoJson();
