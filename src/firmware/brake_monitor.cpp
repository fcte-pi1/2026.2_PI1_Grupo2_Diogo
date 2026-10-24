#include "brake_monitor.h"
#include "encoders.h"
#include "config.h"

static DadosFreio ultimo;

void iniciarMedicaoFreio() {
    DadosEncoder d = lerEncoders();
    ultimo = DadosFreio{};
    ultimo.valido = true;
    ultimo.tempoMs = millis();
    ultimo.inicialE = d.esq;
    ultimo.inicialD = d.dir;
}

void atualizarMedicaoFreio() {
    if (!ultimo.valido || ultimo.tem1000) return;
    uint32_t dt = millis() - ultimo.tempoMs;
    if (!ultimo.tem300 && dt >= 300) {
        DadosEncoder d = lerEncoders();
        ultimo.em300E = d.esq;
        ultimo.em300D = d.dir;
        ultimo.tem300 = true;
    }
    if (dt >= FREIO_MEDICAO_FINAL_MS) {
        DadosEncoder d = lerEncoders();
        // Caso o loop tenha atrasado mais de 1s, 300ms deixa de ser exato;
        // informar no README que o instante e uma amostra aproximada.
        if (!ultimo.tem300) {
            ultimo.em300E = d.esq;
            ultimo.em300D = d.dir;
            ultimo.tem300 = true;
        }
        ultimo.em1000E = d.esq;
        ultimo.em1000D = d.dir;
        ultimo.tem1000 = true;
    }
}

DadosFreio lerMedicaoFreio() { return ultimo; }
bool aguardandoMedicaoFreio() { return ultimo.valido && !ultimo.tem1000; }
