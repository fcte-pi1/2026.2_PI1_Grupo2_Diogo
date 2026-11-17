#include "nav_laser_policy.h"
#include <assert.h>
#include <stdio.h>
int main() {
    HistoricoLaserNav h;
    h.reiniciar(100);
    auto decisao=[&](int raw, uint8_t status, bool recente=true, bool enabled=true){
        return h.classificar(raw,status,raw<0?PAREDE_INDEFINIDA:
            (raw<=120?PAREDE_PRESENTE:PAREDE_AUSENTE),recente,true,enabled,3);
    };
    // Uma leitura fraca nao e suficiente, nem duas.
    h.novaAmostra(101,LASER_SINAL_FRACO,2,PAREDE_INDEFINIDA);
    assert(decisao(-7,2)==EvidenciaParedeNav::INDEFINIDA);
    h.novaAmostra(102,LASER_SINAL_FRACO,2,PAREDE_INDEFINIDA);
    assert(decisao(-7,2)==EvidenciaParedeNav::INDEFINIDA);
    h.novaAmostra(103,LASER_SINAL_FRACO,2,PAREDE_INDEFINIDA);
    assert(decisao(-7,2)==EvidenciaParedeNav::LIVRE);
    assert(decisao(-7,2,true,false)==EvidenciaParedeNav::INDEFINIDA); // opt-in
    assert(decisao(-7,2,false,true)==EvidenciaParedeNav::INDEFINIDA); // evento obsoleto
    // Erro de I2C nunca sera considerado caminho livre.
    h.novaAmostra(104,LASER_ERRO_COMUNICACAO,255,PAREDE_INDEFINIDA);
    assert(decisao(-6,255)==EvidenciaParedeNav::INDEFINIDA);
    // Leitura valida proxima prevalece imediatamente.
    h.novaAmostra(105,75,0,PAREDE_PRESENTE);
    assert(decisao(75,0)==EvidenciaParedeNav::PRESENTE);
    for(uint32_t i=106;i<110;i++)h.novaAmostra(i,-7,2,PAREDE_INDEFINIDA);
    assert(decisao(-7,2)==EvidenciaParedeNav::INDEFINIDA);
    // Ao mudar de celula/orientacao, reseta historico de parede anterior.
    h.reiniciar(110);
    for(uint32_t i=111;i<114;i++)h.novaAmostra(i,-7,2,PAREDE_INDEFINIDA);
    assert(decisao(-7,2)==EvidenciaParedeNav::LIVRE);
    // V4.9.1: sinal fraco (status 1), falha de fase (4) e fora de escala
    // tambem significam "sem alvo" e, repetidos, contam como passagem.
    h.reiniciar(200);
    h.novaAmostra(201,LASER_SINAL_FRACO,1,PAREDE_INDEFINIDA);
    h.novaAmostra(202,LASER_FASE_INVALIDA,4,PAREDE_INDEFINIDA);
    assert(decisao(LASER_FASE_INVALIDA,4)==EvidenciaParedeNav::INDEFINIDA);
    h.novaAmostra(203,LASER_FORA_ESCALA,0,PAREDE_INDEFINIDA);
    assert(decisao(LASER_FORA_ESCALA,0)==EvidenciaParedeNav::LIVRE);
    // Timeout e erro de I2C nunca liberam passagem e zeram a contagem.
    h.novaAmostra(204,LASER_TIMEOUT,255,PAREDE_INDEFINIDA);
    assert(decisao(LASER_TIMEOUT,255)==EvidenciaParedeNav::INDEFINIDA);
    assert(h.semAlvoSeguidas()==0);
    // Abaixo do minimo do sensor: algo encostado = parede, nunca passagem.
    h.reiniciar(300);
    h.novaAmostra(301,LASER_PERTO_DEMAIS,0,PAREDE_INDEFINIDA);
    assert(decisao(LASER_PERTO_DEMAIS,0)==EvidenciaParedeNav::PRESENTE);
    for(uint32_t i=302;i<306;i++)h.novaAmostra(i,LASER_SINAL_FRACO,2,PAREDE_INDEFINIDA);
    assert(decisao(LASER_SINAL_FRACO,2)==EvidenciaParedeNav::INDEFINIDA);
    printf("PASS: 15 casos de classificacao para navegacao\n");
}
