#include "battery.h"
#include "config.h"
#include "power_logic.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <math.h>

static Adafruit_INA219 ina219(INA219_ADDR);
static bool presente = false;
static bool primeira = true;
static float tensao = NAN, corrente = NAN, potencia = NAN;
static uint32_t ultimaLeitura = 0;

static const LimitesBateria LIMITES_PADRAO = {BAT_V_ALERTA, BAT_V_CORTE, BAT_TEMPO_CONFIRMACAO_MS,
                                              I_SOBRECORRENTE_MA, I_SOBRECORRENTE_TEMPO_MS};
static LimitesBateria limites = LIMITES_PADRAO;
static MonitorBateria monitor(LIMITES_PADRAO);
static EstadoBateria estadoAvisado = BATERIA_NORMAL;
static bool sobrecorrenteAvisada = false;

static float filtrar(float antigo, float novo) {
    return primeira || !isfinite(antigo) ? novo : antigo + 0.20f * (novo - antigo);
}

void initBattery() {
    presente = ina219.begin();
    if (!presente) {
        Serial.println("INA219 ausente: telemetria de energia e protecao da bateria indisponiveis.");
        return;
    }
    ina219.setCalibration_32V_2A();
    Serial.println("INA219: 0x40; atencao para limite de leitura de 2A e shunt instalado.");
    ultimaLeitura = millis() - AMOSTRA_BATERIA_MS;
    updateBattery();
}

void updateBattery() {
    if (!presente || millis() - ultimaLeitura < AMOSTRA_BATERIA_MS) return;
    ultimaLeitura = millis();
    // Uma leitura com falha no I2C nao entra no filtro: fica a ultima valida.
    const float vb = ina219.getBusVoltage_V();
    bool ok = ina219.success();
    const float vs = ina219.getShuntVoltage_mV();
    ok = ok && ina219.success();
    const float ia = ina219.getCurrent_mA();
    ok = ok && ina219.success();
    const float pw = ina219.getPower_mW();
    ok = ok && ina219.success();
    if (!ok || !isfinite(vb) || !isfinite(vs) || !isfinite(ia) || !isfinite(pw) || vb < 0 || vb > 32) return;
    tensao = filtrar(tensao, vb + vs * 0.001f);
    corrente = filtrar(corrente, ia);
    potencia = filtrar(potencia, pw);
    primeira = false;

    monitor.atualizar(tensao, corrente, ultimaLeitura);

    // Avisa na serial so na mudanca de estado.
    if (monitor.estado() != estadoAvisado) {
        estadoAvisado = monitor.estado();
        if (estadoAvisado == BATERIA_BAIXA) {
            Serial.printf("[ENERGIA] Bateria baixa: %.2f V\n", tensao);
        } else if (estadoAvisado == BATERIA_CRITICA) {
            Serial.printf("[ENERGIA] Bateria critica: %.2f V, corte acionado\n", tensao);
        }
    }
    if (monitor.sobrecorrente() != sobrecorrenteAvisada) {
        sobrecorrenteAvisada = monitor.sobrecorrente();
        if (sobrecorrenteAvisada) Serial.printf("[ENERGIA] Sobrecorrente: %.0f mA\n", corrente);
    }
}

bool batteryAvailable() { return presente; }
float getVoltage() { return tensao; }
float getCurrent() { return corrente; }
float getPowerMilliwatt() { return potencia; }
float getBatteryPercent() { return isfinite(tensao) ? bateriaPercentual(tensao, BAT_CELULAS) : NAN; }

bool bateriaCritica() { return monitor.estado() == BATERIA_CRITICA; }
bool bateriaBaixa() { return monitor.estado() == BATERIA_BAIXA; }
bool sobrecorrente() { return monitor.sobrecorrente(); }
const char* estadoBateriaTexto() {
    if (!presente || !isfinite(tensao)) return "SEM_LEITURA";
    switch (monitor.estado()) {
        case BATERIA_CRITICA: return "CRITICA";
        case BATERIA_BAIXA: return "BAIXA";
        default: return "NORMAL";
    }
}
float limiteCorteBateriaV() { return limites.vCorte; }
float limiteSobrecorrenteMa() { return limites.iSobrecorrenteMA; }

const char* definirLimitesBateria(float corteV, float sobrecorrenteMa) {
    if (!limitesDeEnsaioValidos(corteV, sobrecorrenteMa, LIMITES_PADRAO,
                                BAT_V_CORTE_MAX_ENSAIO, I_SOBRECORRENTE_MIN_ENSAIO_MA))
        return "Corte entre 6.6 e 9.0 V; sobrecorrente entre 50 e 2000 mA";
    if (!podeRearmarCorte(monitor.estado(), tensao, BAT_V_ALERTA))
        return "Corte travado com bateria abaixo de 7.0 V: recarregue a bateria";
    limites = LIMITES_PADRAO;
    limites.vCorte = corteV;
    limites.iSobrecorrenteMA = sobrecorrenteMa;
    monitor = MonitorBateria(limites);
    estadoAvisado = BATERIA_NORMAL;
    sobrecorrenteAvisada = false;
    Serial.printf("[ENERGIA] Limites: corte %.2f V, sobrecorrente %.0f mA\n", corteV, sobrecorrenteMa);
    return nullptr;
}
