#pragma once

#include "lasers_logic.h"

enum SensorLaser { LASER_ESQ, LASER_FTE, LASER_DIR };

void initLasers();

// Distância em mm (>= 0) ou um código negativo: LASER_SEM_LEITURA,
// LASER_FORA_ESCALA ou LASER_TIMEOUT (ver lasers_logic.h)
int ler_distancia_mm(SensorLaser id);

// Mantida para o main e a telemetria: qualquer código de erro vira -1
void readLasers(int &distEsq, int &distFte, int &distDir);
