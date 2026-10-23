#include "web_telemetry.h"
#include "web_page.h"
#include "motors.h"
#include "encoders.h"
#include "battery.h"
#include "config.h"
#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>
#include <math.h>
#include <stdlib.h>

extern WebServer server;
static Preferences prefs;
static bool prefsOk = false;

enum class Modo { PARADO, MANUAL, PWM };
static Modo modo = Modo::PARADO;
static String ultimoMotivo = "pronto";

static float cprE = 0, cprD = 0; // não assumir 210 ou 1400!

// Medição MANUAL: operador conta as voltas físicas e encerra pelo site.
static String ladoManual = "ambos";
static int64_t manualInicioE = 0, manualInicioD = 0;
static int64_t manualDeltaE = 0, manualDeltaD = 0;
static float ultimaMedidaE = 0, ultimaMedidaD = 0;
static float ultimaQuantidadeVoltas = 0;
// Estatísticas por roda: online / método de Welford (uma sessão, RAM).
struct Estatisticas {
    uint32_t n = 0;
    float media = 0;
    float m2 = 0;
    void adicionar(float x) {
        ++n;
        float delta = x - media;
        media += delta / n;
        m2 += delta * (x - media);
    }
    float desvio() const { return n >= 2 ? sqrtf(m2 / (n - 1)) : 0; }
};
static Estatisticas estatE;
static Estatisticas estatD;

// Ensaio de PWM: acionamento de duração limitada e heartbeat obrigatório.
static uint32_t testeInicio = 0;
static uint32_t duracaoTeste = 0;
static uint32_t ultimoHeartbeat = 0;
static int64_t testeInicioE = 0, testeInicioD = 0;
static int64_t ultimoTesteDeltaE = 0, ultimoTesteDeltaD = 0;
static int pwmE = 0, pwmD = 0;
static bool frenteE = true, frenteD = true;

static void respostaErro(int codigo, const char* msg) {
    server.send(codigo, "application/json", String("{\"erro\":\"") + msg + "\"}");
}

static void respostaOk(const char* msg) {
    server.send(200, "application/json", String("{\"mensagem\":\"") + msg + "\"}");
}

static String inteiro64(int64_t valor) {
    char buf[30];
    snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(valor));
    return String(buf);
}

static String numeroOuNull(float valor, int casas = 2) {
    return isfinite(valor) ? String(valor, casas) : String("null");
}

static bool lateralValida(const String& lado) {
    return lado == "esq" || lado == "dir" || lado == "ambos";
}

static bool inteiroEmFaixa(const String& nome, int minimo, int maximo, int& saida) {
    if (!server.hasArg(nome)) return false;
    String s = server.arg(nome);
    if (s.isEmpty()) return false;
    for (unsigned int i = 0; i < s.length(); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    long val = s.toInt();
    if (val < minimo || val > maximo) return false;
    saida = static_cast<int>(val);
    return true;
}

static bool floatValido(const String& nome, float& saida) {
    if (!server.hasArg(nome)) return false;
    String texto = server.arg(nome);
    char* fim = nullptr;
    float valor = strtof(texto.c_str(), &fim);
    if (fim == texto.c_str() || *fim != '\0' || !isfinite(valor)) return false;
    saida = valor;
    return true;
}

static void encerrarTestePWM(const char* motivo) {
    if (modo == Modo::PWM) {
        // Primeiro manda frear; registra as contagens naquele instante.
        frearMotoresTudo();
        DadosEncoder d = lerEncoders();
        ultimoTesteDeltaE = d.esq - testeInicioE;
        ultimoTesteDeltaD = d.dir - testeInicioD;
    } else {
        frearMotoresTudo();
    }
    pwmE = pwmD = 0;
    modo = Modo::PARADO;
    ultimoMotivo = motivo;
    Serial.printf("PARADA: %s\n", motivo);
}

void pararTesteDeEmergencia(const char* motivo) {
    encerrarTestePWM(motivo);
}

static void handleStatus() {
    DadosEncoder d = lerEncoders();
    uint32_t agora = millis();
    int64_t deltaManualE = (modo == Modo::MANUAL) ? d.esq - manualInicioE : 0;
    int64_t deltaManualD = (modo == Modo::MANUAL) ? d.dir - manualInicioD : 0;
    uint32_t decorrido = modo == Modo::PWM ? agora - testeInicio : 0;
    uint32_t restante = modo == Modo::PWM && decorrido < duracaoTeste ? duracaoTeste - decorrido : 0;

    String json;
    json.reserve(900);
    json += "{\"ms\":" + String(agora);
    json += ",\"modo\":\"" + String(modo == Modo::PWM ? "PWM" : (modo == Modo::MANUAL ? "MANUAL" : "PARADO")) + "\"";
    json += ",\"motivo\":\"" + ultimoMotivo + "\"";
    json += ",\"contE\":" + inteiro64(d.esq);
    json += ",\"contD\":" + inteiro64(d.dir);
    json += ",\"cpsE\":" + numeroOuNull(d.cpsEsq);
    json += ",\"cpsD\":" + numeroOuNull(d.cpsDir);
    json += ",\"cprE\":" + numeroOuNull(cprE);
    json += ",\"cprD\":" + numeroOuNull(cprD);
    json += ",\"rpmE\":" + (cprE > 0 ? numeroOuNull(d.cpsEsq * 60.0f / cprE) : String("null"));
    json += ",\"rpmD\":" + (cprD > 0 ? numeroOuNull(d.cpsDir * 60.0f / cprD) : String("null"));
    json += ",\"pwmE\":" + String(pwmE);
    json += ",\"pwmD\":" + String(pwmD);
    json += ",\"frenteE\":" + String(frenteE ? "true" : "false");
    json += ",\"frenteD\":" + String(frenteD ? "true" : "false");
    json += ",\"tensao\":" + numeroOuNull(getVoltage());
    json += ",\"corrente\":" + numeroOuNull(getCurrent());
    json += ",\"inaOk\":" + String(batteryAvailable() ? "true" : "false");
    json += ",\"manualLado\":\"" + ladoManual + "\"";
    json += ",\"manualDeltaE\":" + inteiro64(deltaManualE);
    json += ",\"manualDeltaD\":" + inteiro64(deltaManualD);
    json += ",\"ultimaMedidaE\":" + numeroOuNull(ultimaMedidaE, 4);
    json += ",\"ultimaMedidaD\":" + numeroOuNull(ultimaMedidaD, 4);
    json += ",\"ultimaQuantidadeVoltas\":" + numeroOuNull(ultimaQuantidadeVoltas, 1);
    json += ",\"nE\":" + String(estatE.n);
    json += ",\"nD\":" + String(estatD.n);
    json += ",\"mediaE\":" + numeroOuNull(estatE.n ? estatE.media : NAN, 4);
    json += ",\"mediaD\":" + numeroOuNull(estatD.n ? estatD.media : NAN, 4);
    json += ",\"desvioE\":" + numeroOuNull(estatE.n > 1 ? estatE.desvio() : NAN, 4);
    json += ",\"desvioD\":" + numeroOuNull(estatD.n > 1 ? estatD.desvio() : NAN, 4);
    json += ",\"manualUltDeltaE\":" + inteiro64(manualDeltaE);
    json += ",\"manualUltDeltaD\":" + inteiro64(manualDeltaD);
    json += ",\"duracaoMs\":" + String(duracaoTeste);
    json += ",\"decorridoMs\":" + String(decorrido);
    json += ",\"restanteMs\":" + String(restante);
    json += ",\"testeDeltaE\":" + inteiro64(ultimoTesteDeltaE);
    json += ",\"testeDeltaD\":" + inteiro64(ultimoTesteDeltaD);
    json += ",\"meta90E\":" + (cprE > 0 ? numeroOuNull(ENTRE_RODAS_MM * cprE / (4 * DIAMETRO_RODA_MM)) : String("null"));
    json += ",\"meta90D\":" + (cprD > 0 ? numeroOuNull(ENTRE_RODAS_MM * cprD / (4 * DIAMETRO_RODA_MM)) : String("null"));
    json += "}";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", json);
}

static void handleManualStart() {
    if (modo != Modo::PARADO) return respostaErro(409, "Encerre o teste anterior primeiro");
    String lado = server.arg("lado");
    if (!lateralValida(lado)) return respostaErro(400, "Lado invalido");
    pararMotores(); // libera as rodas; sem PWM
    DadosEncoder d = lerEncoders();
    manualInicioE = d.esq;
    manualInicioD = d.dir;
    ladoManual = lado;
    modo = Modo::MANUAL;
    ultimoMotivo = "medicao_manual_em_andamento";
    respostaOk("Gire as rodas fisicamente, depois finalize a medicao");
}

static void handleManualFinish() {
    if (modo != Modo::MANUAL) return respostaErro(409, "Nenhuma medicao manual iniciada");
    int voltas = 0;
    if (!inteiroEmFaixa("voltas", 1, 100, voltas)) {
        return respostaErro(400, "Use um numero inteiro de voltas entre 1 e 100");
    }
    DadosEncoder d = lerEncoders();
    manualDeltaE = d.esq - manualInicioE;
    manualDeltaD = d.dir - manualInicioD;
    ultimaQuantidadeVoltas = static_cast<float>(voltas);
    if (ladoManual == "esq" || ladoManual == "ambos") {
        ultimaMedidaE = fabsf(static_cast<float>(manualDeltaE)) / voltas;
        if (ultimaMedidaE > 0) estatE.adicionar(ultimaMedidaE);
    }
    if (ladoManual == "dir" || ladoManual == "ambos") {
        ultimaMedidaD = fabsf(static_cast<float>(manualDeltaD)) / voltas;
        if (ultimaMedidaD > 0) estatD.adicionar(ultimaMedidaD);
    }
    modo = Modo::PARADO;
    ultimoMotivo = "medicao_manual_concluida";
    respostaOk("Medicao concluida; confira contagens e CPR estimado");
}

static void handleLimparEstatisticas() {
    if (modo != Modo::PARADO) return respostaErro(409, "Finalize a medicao antes de limpar");
    estatE = Estatisticas{};
    estatD = Estatisticas{};
    ultimaMedidaE = ultimaMedidaD = ultimaQuantidadeVoltas = 0;
    manualDeltaE = manualDeltaD = 0;
    respostaOk("Estatisticas de CPR da sessao zeradas (CPR salvo mantido)");
}

static void handleReset() {
    if (modo != Modo::PARADO) return respostaErro(409, "Pare antes de zerar os contadores");
    // Motores podem estar frenados ou em coast, mas devem estar IMÓVEIS.
    zerarEncoders();
    ultimoMotivo = "contadores_zerados";
    respostaOk("Contadores zerados; assegure-se de que as rodas estao paradas");
}

static void handleSalvarCpr() {
    if (modo != Modo::PARADO) return respostaErro(409, "Pare antes de salvar CPR");
    float e, d;
    if (!floatValido("esq", e) || !floatValido("dir", d) ||
        e < 1 || e > 100000 || d < 1 || d > 100000) {
        return respostaErro(400, "CPR deve estar entre 1 e 100000 para cada roda");
    }
    if (!prefsOk) return respostaErro(500, "NVS indisponivel: nao foi possivel salvar CPR");
    cprE = e;
    cprD = d;
    prefs.putFloat("cprE", cprE);
    prefs.putFloat("cprD", cprD);
    ultimoMotivo = "cpr_salvo";
    respostaOk("CPR salvo na memoria; RPM e referencia teorica de 90 graus ativados");
}

static void handlePWMStart() {
    if (!motorHardwareReady()) return respostaErro(503, "Driver PWM nao inicializado");
    if (modo != Modo::PARADO) return respostaErro(409, "Encerre o teste anterior primeiro");
    String lado = server.arg("lado");
    if (!lateralValida(lado)) return respostaErro(400, "Lado invalido");
    int e = 0, d = 0, duracao = 0;
    if (!inteiroEmFaixa("pwmE", 0, PWM_LIMITE_TESTE, e) ||
        !inteiroEmFaixa("pwmD", 0, PWM_LIMITE_TESTE, d) ||
        !inteiroEmFaixa("duracao", TESTE_DURACAO_MIN_MS, TESTE_DURACAO_MAX_MS, duracao)) {
        return respostaErro(400, "PWM ou duracao fora dos limites de seguranca");
    }
    String sentidoE = server.arg("sentidoE");
    String sentidoD = server.arg("sentidoD");
    if ((sentidoE != "frente" && sentidoE != "tras") ||
        (sentidoD != "frente" && sentidoD != "tras")) {
        return respostaErro(400, "Sentido invalido");
    }
    frenteE = sentidoE == "frente";
    frenteD = sentidoD == "frente";
    pwmE = lado == "dir" ? 0 : e;
    pwmD = lado == "esq" ? 0 : d;
    pararMotores();
    DadosEncoder antes = lerEncoders();
    testeInicioE = antes.esq;
    testeInicioD = antes.dir;
    ultimoTesteDeltaE = ultimoTesteDeltaD = 0;
    duracaoTeste = static_cast<uint32_t>(duracao);
    testeInicio = millis();
    ultimoHeartbeat = testeInicio;
    modo = Modo::PWM;
    ultimoMotivo = "teste_pwm_em_andamento";
    setMotorEsq(pwmE, frenteE);
    setMotorDir(pwmD, frenteD);
    respostaOk("Teste PWM iniciado; mantenha esta pagina aberta para heartbeat");
}

static void handleHeartbeat() {
    if (modo == Modo::PWM) ultimoHeartbeat = millis();
    respostaOk("heartbeat");
}

void setupTelemetry() {
    prefsOk = prefs.begin("mousecal", false);
    if (prefsOk) {
        cprE = prefs.getFloat("cprE", 0);
        cprD = prefs.getFloat("cprD", 0);
    }
    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html; charset=utf-8", PAGINA_HTML);
    });
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/manual/start", HTTP_POST, handleManualStart);
    server.on("/api/manual/finish", HTTP_POST, handleManualFinish);
    server.on("/api/encoders/reset", HTTP_POST, handleReset);
    server.on("/api/manual/clear", HTTP_POST, handleLimparEstatisticas);
    server.on("/api/cpr", HTTP_POST, handleSalvarCpr);
    server.on("/api/pwm/start", HTTP_POST, handlePWMStart);
    server.on("/api/pwm/stop", HTTP_POST, []() {
        pararTesteDeEmergencia("parado_pela_pagina");
        respostaOk("Motores frenados");
    });
    server.on("/api/heartbeat", HTTP_POST, handleHeartbeat);
    server.onNotFound([]() { respostaErro(404, "Rota nao encontrada"); });
}

void loopTelemetry() {
    if (modo != Modo::PWM) return;
    uint32_t agora = millis();
    if (agora - ultimoHeartbeat > WATCHDOG_WEB_MS) {
        encerrarTestePWM("watchdog_conexao_perdida");
    } else if (agora - testeInicio >= duracaoTeste) {
        encerrarTestePWM("teste_pwm_tempo_concluido");
    }
}
