#pragma once
#include <Arduino.h>
struct DadosIMU {
    bool presente = false;
    bool calibrando = false;
    bool calibrada = false;
    bool confiavel = false;
    float yawDireitaGraus = 0; // direita positivo, somente relativo
    float taxaDireitaDps = 0;
    float biasDps = 0;
    float ruidoDps = 0;
    uint32_t ultimaLeituraMs = 0;
    uint32_t falhasI2C = 0;
    uint8_t enderecoI2C = 0;
    uint8_t whoAmI = 0;
    uint8_t falhaInit = 0; // 0 ok; 1 sem ACK; 2 WHO nao lido; 3 WHO inesperado; 4 configuracao falhou

    int amostrasBias = 0;
};
void iniciarIMU();
void atualizarIMU();
bool iniciarCalibracaoIMU(); // somente com o robo imovel e motores parados
bool zerarYawIMU();        // zerar a referencia imediatamente antes do giro
DadosIMU lerIMU();
