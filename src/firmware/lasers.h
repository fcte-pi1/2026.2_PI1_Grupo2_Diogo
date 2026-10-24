#pragma once

#include "lasers_logic.h"

enum SensorLaser { LASER_ESQ, LASER_FTE, LASER_DIR };

void initLasers();
int ler_distancia_mm(SensorLaser id);
void readLasers(int &distEsq, int &distFte, int &distDir);
