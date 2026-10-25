#pragma once
#include <Arduino.h>
#include "lasers_logic.h"

enum SensorLaser : uint8_t { LASER_U2=0, LASER_U3=1, LASER_U4=2 };
struct LeituraLaser {
    bool inicializado=false;
    int brutoMm=LASER_SEM_LEITURA;          // negative means invalid
    int filtradoMm=LASER_SEM_LEITURA;       // negative means invalid
    uint8_t statusVL=255;
    EstadoParede parede=PAREDE_INDEFINIDA;
    uint32_t ultimoEventoMs=0;
    uint32_t ultimaValidaMs=0;
    uint32_t total=0,validas=0,invalidas=0;
    uint32_t intervaloRealMs=0;
    uint16_t periodoConfiguradoMs=0;
    uint32_t validadeMs=0;
};
struct ResumoCapturaLaser {
    bool ativa=false,pronta=false;
    uint8_t indiceSensor=0;
    int referenciaMm=0;
    uint16_t objetivo=0,amostras=0,validas=0,invalidas=0;
    float mediaBruta=0, desvioBruta=0, mediaFiltrada=0;
    uint32_t inicioMs=0, fimMs=0;
    String motivo;
};
void initLasers();          // init all three via XSHUT, call once after Wire.begin()
void atualizarLasers();     // called by loop (no HTTP inside)
LeituraLaser lerLaser(uint8_t indice);
String jsonLasers();
bool iniciarCapturaLaser(uint8_t indice,int referenciaMm,uint16_t quantidade);
bool capturaLaserAtiva();
ResumoCapturaLaser resumoCapturaLaser();
void limparCapturaLaser();
String csvCapturaLaser();
