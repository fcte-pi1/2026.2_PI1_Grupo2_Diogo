#include "web_telemetry.h"
#include "web_page.h"
#include "motors.h"
#include "encoders.h"
#include "battery.h"
#include "config.h"
#include "motion_control.h"
#include "brake_monitor.h"
#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>
#include <math.h>
#include <stdlib.h>

extern WebServer server;
static Preferences prefs;
static bool prefsOk = false;

enum class Modo { PARADO, MANUAL, PWM, RPM, POSICAO };
static Modo modo = Modo::PARADO;
static bool modoMotorizado() { return modo == Modo::PWM || modo == Modo::RPM || modo == Modo::POSICAO; }
static const char* nomeModo() { switch (modo) { case Modo::MANUAL: return "MANUAL"; case Modo::PWM: return "PWM"; case Modo::RPM: return "RPM"; case Modo::POSICAO: return "POSICAO"; default: return "PARADO"; } }
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

static void encerrarTeste(const char* motivo) {
    bool estavaMotorizado = modoMotorizado();
    if (estavaMotorizado) {
        // PRIMEIRO captura contagens; depois manda o TB6612 frear.
        iniciarMedicaoFreio();
    }
    frearMotoresTudo();
    if (modo == Modo::PWM) {
        DadosEncoder d = lerEncoders();
        ultimoTesteDeltaE = d.esq - testeInicioE;
        ultimoTesteDeltaD = d.dir - testeInicioD;
    }
    desativarControleMovimento();
    pwmE = pwmD = 0;
    modo = Modo::PARADO;
    if (estavaMotorizado || ultimoMotivo == "pronto") ultimoMotivo = motivo;
    if (estavaMotorizado) Serial.printf("PARADA: %s\n", motivo);
}

void pararTesteDeEmergencia(const char* motivo) {
    encerrarTeste(motivo);
}

static void handleStatus() {
    DadosEncoder d = lerEncoders();
    uint32_t agora = millis();
    int64_t deltaManualE = (modo == Modo::MANUAL) ? d.esq - manualInicioE : 0;
    int64_t deltaManualD = (modo == Modo::MANUAL) ? d.dir - manualInicioD : 0;
    uint32_t decorrido = modoMotorizado() ? agora - testeInicio : 0;
    uint32_t restante = modoMotorizado() && decorrido < duracaoTeste ? duracaoTeste - decorrido : 0;
    ControleInfo ctrl = lerControleMovimento();
    DadosFreio f = lerMedicaoFreio();
    const bool emControle = modo == Modo::RPM || modo == Modo::POSICAO;

    String json;
    json.reserve(1850);
    json += "{\"ms\":" + String(agora);
    json += ",\"modo\":\"" + String(nomeModo()) + "\"";
    json += ",\"motivo\":\"" + ultimoMotivo + "\"";
    json += ",\"contE\":" + inteiro64(d.esq);
    json += ",\"contD\":" + inteiro64(d.dir);
    json += ",\"cpsE\":" + numeroOuNull(d.cpsEsq);
    json += ",\"cpsD\":" + numeroOuNull(d.cpsDir);
    json += ",\"cprE\":" + numeroOuNull(cprE);
    json += ",\"cprD\":" + numeroOuNull(cprD);
    json += ",\"rpmE\":" + (cprE > 0 ? numeroOuNull(d.cpsEsq * 60.0f / cprE) : String("null"));
    json += ",\"rpmD\":" + (cprD > 0 ? numeroOuNull(d.cpsDir * 60.0f / cprD) : String("null"));
    json += ",\"pwmE\":" + String(emControle ? ctrl.pwmE : pwmE);
    json += ",\"pwmD\":" + String(emControle ? ctrl.pwmD : pwmD);
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
    json += ",\"alvoE\":" + String(ctrl.alvoE);
    json += ",\"alvoD\":" + String(ctrl.alvoD);
    json += ",\"progressoE\":" + inteiro64(ctrl.progressoE);
    json += ",\"progressoD\":" + inteiro64(ctrl.progressoD);
    json += ",\"referenciaRpmE\":" + numeroOuNull(ctrl.desejadoE);
    json += ",\"referenciaRpmD\":" + numeroOuNull(ctrl.desejadoD);
    json += ",\"realControleRpmE\":" + numeroOuNull(ctrl.realE);
    json += ",\"realControleRpmD\":" + numeroOuNull(ctrl.realD);
    json += ",\"controleLimiteRpm\":" + numeroOuNull(ctrl.limiteRpm);
    json += ",\"chegouE\":" + String(ctrl.finalizouE ? "true" : "false");
    json += ",\"chegouD\":" + String(ctrl.finalizouD ? "true" : "false");
    json += ",\"freioValido\":" + String(f.valido ? "true" : "false");
    json += ",\"freio300Pronto\":" + String(f.tem300 ? "true" : "false");
    json += ",\"freio1000Pronto\":" + String(f.tem1000 ? "true" : "false");
    json += ",\"freioInicioE\":" + inteiro64(f.inicialE);
    json += ",\"freioInicioD\":" + inteiro64(f.inicialD);
    json += ",\"freio300E\":" + inteiro64(f.em300E);
    json += ",\"freio300D\":" + inteiro64(f.em300D);
    json += ",\"freio1000E\":" + inteiro64(f.em1000E);
    json += ",\"freio1000D\":" + inteiro64(f.em1000D);
    json += ",\"freioDelta300E\":" + inteiro64(f.tem300 ? f.em300E - f.inicialE : 0);
    json += ",\"freioDelta300D\":" + inteiro64(f.tem300 ? f.em300D - f.inicialD : 0);
    json += ",\"freioDelta1000E\":" + inteiro64(f.tem1000 ? f.em1000E - f.inicialE : 0);
    json += ",\"freioDelta1000D\":" + inteiro64(f.tem1000 ? f.em1000D - f.inicialD : 0);
    json += ",\"meta90E\":" + (cprE > 0 ? numeroOuNull(ENTRE_RODAS_MM * cprE / (4 * DIAMETRO_RODA_MM)) : String("null"));
    json += ",\"meta90D\":" + (cprD > 0 ? numeroOuNull(ENTRE_RODAS_MM * cprD / (4 * DIAMETRO_RODA_MM)) : String("null"));
    json += "}";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", json);
}

static void handleManualStart() {
    if (modo != Modo::PARADO || aguardandoMedicaoFreio()) return respostaErro(409, "Finalize teste e aguarde 1 s apos o freio");
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
    if (!prefs.putFloat("cprE", e) || !prefs.putFloat("cprD", d)) {
        return respostaErro(500, "Falha ao gravar CPR na memoria NVS");
    }
    cprE = e;
    cprD = d;
    ultimoMotivo = "cpr_salvo";
    respostaOk("CPR salvo na memoria; RPM e referencia teorica de 90 graus ativados");
}

static void handlePWMStart() {
    if (!motorHardwareReady()) return respostaErro(503, "Driver PWM nao inicializado");
    if (modo != Modo::PARADO || aguardandoMedicaoFreio()) return respostaErro(409, "Finalize teste e aguarde 1 s apos o freio");
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
    if (modoMotorizado()) ultimoHeartbeat = millis();
    respostaOk("heartbeat");
}

static bool inteiroComSinal(const String& nome, int minimo, int maximo, int& saida) {
    if (!server.hasArg(nome)) return false;
    String s = server.arg(nome);
    if (s.isEmpty()) return false;
    char* fim = nullptr;
    long v = strtol(s.c_str(), &fim, 10);
    if (fim == s.c_str() || *fim != '\0' || v < minimo || v > maximo) return false;
    saida = static_cast<int>(v);
    return true;
}

static bool podeIniciarControle() {
    return motorHardwareReady() && cprE > 0 && cprD > 0 &&
           modo == Modo::PARADO && !aguardandoMedicaoFreio();
}

static void iniciarRelogioMotorizado(int duracao, Modo novoModo) {
    testeInicio = millis();
    ultimoHeartbeat = testeInicio;
    duracaoTeste = static_cast<uint32_t>(duracao);
    pwmE = pwmD = 0;
    modo = novoModo;
}

static void handlePosicaoStart() {
    if (!podeIniciarControle())
        return respostaErro(409, "Exige CPR salvo, motores prontos, PARADO e freio concluido");
    int e, d, duracao;
    float rpm;
    if (!inteiroComSinal("alvoE", -POSICAO_META_MAX_CONTAGENS, POSICAO_META_MAX_CONTAGENS, e) ||
        !inteiroComSinal("alvoD", -POSICAO_META_MAX_CONTAGENS, POSICAO_META_MAX_CONTAGENS, d) ||
        !inteiroEmFaixa("duracao", 500, TESTE_DURACAO_MAX_MS, duracao) ||
        !floatValido("rpm", rpm) || rpm < 20 || rpm > RPM_MAX_POSICAO ||
        (e == 0 && d == 0))
        return respostaErro(400, "Metas -2000..2000, RPM 20..75, duracao 500..10000");
    if (!iniciarControlePosicao(e, d, rpm, cprE, cprD))
        return respostaErro(409, "Controlador nao iniciou");
    iniciarRelogioMotorizado(duracao, Modo::POSICAO);
    ultimoMotivo = "controle_posicao_iniciado";
    respostaOk("Movimento iniciado; rodas SUSPENSAS; acompanhe progresso e erro");
}

static void handleRpmStart() {
    if (!podeIniciarControle())
        return respostaErro(409, "Exige CPR salvo, motores prontos, PARADO e freio concluido");
    int duracao;
    float e, d;
    if (!floatValido("rpmE", e) || !floatValido("rpmD", d) ||
        fabsf(e) > RPM_MAX_TESTE || fabsf(d) > RPM_MAX_TESTE ||
        !inteiroEmFaixa("duracao", 500, TESTE_DURACAO_MAX_MS, duracao) ||
        (fabsf(e) < 10.0f && fabsf(d) < 10.0f) ||
        (e != 0 && fabsf(e) < 10.0f) ||
        (d != 0 && fabsf(d) < 10.0f))
        return respostaErro(400, "RPM por roda entre -120 e 120; duracao 500..10000; pelo menos um motor >= 10 RPM");
    if (!iniciarControleRpm(e, d, cprE, cprD))
        return respostaErro(409, "Controlador nao iniciou");
    iniciarRelogioMotorizado(duracao, Modo::RPM);
    ultimoMotivo = "controle_rpm_iniciado";
    respostaOk("Teste RPM iniciado; compare RPM desejado e medido");
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
    server.on("/api/pos/start", HTTP_POST, handlePosicaoStart);
    server.on("/api/rpm/start", HTTP_POST, handleRpmStart);
    server.on("/api/pwm/stop", HTTP_POST, []() {
        pararTesteDeEmergencia("parado_pela_pagina");
        respostaOk("Motores frenados");
    });
    server.on("/api/heartbeat", HTTP_POST, handleHeartbeat);
    server.onNotFound([]() { respostaErro(404, "Rota nao encontrada"); });
}

void loopTelemetry() {
    atualizarMedicaoFreio();
    if (!modoMotorizado()) return;
    uint32_t agora = millis();
    // Segurança vem antes do cálculo de controle.
    if (agora - ultimoHeartbeat > WATCHDOG_WEB_MS) {
        encerrarTeste("watchdog_conexao_perdida");
    } else if (agora - testeInicio >= duracaoTeste) {
        encerrarTeste("limite_de_tempo");
    } else if (modo == Modo::RPM || modo == Modo::POSICAO) {
        const char* motivo = atualizarControleMovimento();
        if (motivo != nullptr) encerrarTeste(motivo);
    }
}
