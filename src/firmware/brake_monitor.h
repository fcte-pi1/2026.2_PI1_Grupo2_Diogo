#pragma once
#include <Arduino.h>
#include <stdint.h>

struct DadosFreio {
    bool valido = false;
    bool tem300 = false;
    bool tem1000 = false;
    uint32_t tempoMs = 0;
    int64_t inicialE = 0, inicialD = 0;
    int64_t em300E = 0, em300D = 0;
    int64_t em1000E = 0, em1000D = 0;
};

// Registrar imediatamente ANTES de aplicar o curto-freio no TB6612.
void iniciarMedicaoFreio();
void atualizarMedicaoFreio();
DadosFreio lerMedicaoFreio();
bool aguardandoMedicaoFreio();
