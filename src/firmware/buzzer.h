#pragma once
#include "buzzer_logic.h"
void initBuzzer();
void updateBuzzer();
bool buzzerReady();
bool buzzerTest(buzzer::Effect effect);
void buzzerCancelTest();
// Espelha somente o comando de saida; nunca altera PWM ou direcao.
void buzzerObserveMotor(bool left, bool reverse);
