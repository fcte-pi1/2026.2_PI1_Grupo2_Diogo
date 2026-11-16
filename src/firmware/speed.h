#pragma once
#include <Arduino.h>

// Velocidade do robo pelos encoders. Exige CPR calibrado (painel, aba de encoders).
struct DadosVelocidade {
    bool valida = false;           // falso enquanto nao houver CPR salvo
    float rpmE = 0, rpmD = 0;      // com sinal; positivo = frente
    float instantaneaCmS = 0;      // centro do robo, com sinal
    // Trecho = ultimo teste com motor (ou o que esta em andamento).
    float mediaTrechoCmS = 0;
    float distanciaTrechoMm = 0;
    uint32_t tempoTrechoMs = 0;
    // Total = todos os testes com motor desde que o robo foi ligado.
    float mediaTotalCmS = 0;
    float distanciaTotalMm = 0;
    uint32_t tempoTotalMs = 0;
};

// Chamar no loop. emMovimento = ha um teste com motor em andamento.
void atualizarVelocidade(float cprE, float cprD, bool emMovimento);
DadosVelocidade lerVelocidade();
