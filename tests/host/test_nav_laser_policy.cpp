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
    // RangeStatus=1/4 e timeout nunca liberam passagem.
    h.novaAmostra(114,-7,1,PAREDE_INDEFINIDA);
    assert(decisao(-7,1)==EvidenciaParedeNav::INDEFINIDA);
    h.novaAmostra(115,-4,4,PAREDE_INDEFINIDA);
    assert(decisao(-4,4)==EvidenciaParedeNav::INDEFINIDA);
    h.novaAmostra(116,LASER_TIMEOUT,255,PAREDE_INDEFINIDA);
    assert(decisao(-3,255)==EvidenciaParedeNav::INDEFINIDA);
    printf("PASS: 11 casos de classificacao para navegacao\n");
}
