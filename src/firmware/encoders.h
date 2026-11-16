#pragma once
#include <Arduino.h>
#include <stdint.h>

struct DadosEncoder {
    int64_t esq;
    int64_t dir;
    float cpsEsq;
    float cpsDir;
};

bool iniciarEncoders();
void atualizarEncoders();
DadosEncoder lerEncoders();
void zerarEncoders();     // use APENAS com motores parados
