#pragma once

#include "lasers_logic.h"

enum SensorLaser { LASER_ESQ, LASER_FTE, LASER_DIR };

void initLasers();
int ler_distancia_mm(SensorLaser id);
int ler_distancia_bruta_mm(SensorLaser id);
EstadoParede obter_parede(SensorLaser id);
void readLasers(int &distEsq, int &distFte, int &distDir);
