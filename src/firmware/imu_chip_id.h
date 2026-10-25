#pragma once
#include <stdint.h>

// Identificacao estrita. Nao aceitar qualquer ID desconhecido como IMU valida.
// 0x68 = MPU-6050/6000; 0x74 = MPU-6515 (familia MPU-65xx).
inline bool imuIdentidadeSuportada(uint8_t who) {
    return who == 0x68 || who == 0x74;
}
inline const char* imuModelo(uint8_t who) {
    switch (who) {
        case 0x68: return "MPU-6050/6000";
        case 0x74: return "MPU-6515";
        default: return "DESCONHECIDO";
    }
}
