#pragma once
void initBattery();
void updateBattery();
bool batteryAvailable();
float getVoltage();   // V na bateria: bus + shunt, se leitura valida
float getCurrent();   // corrente mA (calibracao INA219 2A, verificar no hardware)
float getPowerMilliwatt();
float getBatteryPercent(); // estimativa por tensao; NAN sem leitura

// Protecao da bateria (regras em power_logic.h).
bool bateriaCritica();   // corte travado
bool bateriaBaixa();
bool sobrecorrente();    // somente aviso
const char* estadoBateriaTexto();
float limiteCorteBateriaV();
float limiteSobrecorrenteMa();
// Limites temporarios para ensaio em bancada, sem descarregar a bateria.
// Recria o monitor. Retorna nullptr se aplicou, ou o motivo da recusa.
const char* definirLimitesBateria(float corteV, float sobrecorrenteMa);
