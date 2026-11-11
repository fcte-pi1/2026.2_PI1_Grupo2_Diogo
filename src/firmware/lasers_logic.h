#pragma once
#include <stdint.h>

// Negative statuses are all INVALID for navigation. In particular, phase failure
// or out-of-range is NOT proof of a clear path.
constexpr int LASER_SEM_LEITURA = -1;
constexpr int LASER_FORA_ESCALA = -2;
constexpr int LASER_TIMEOUT = -3;
constexpr int LASER_FASE_INVALIDA = -4;
constexpr int LASER_PERTO_DEMAIS = -5;
constexpr int LASER_ERRO_COMUNICACAO = -6;

inline int classificarLeitura(bool erroComunicacao, uint8_t rangeStatus,
                            uint16_t distanciaMm, int minMm, int maxMm) {
    if (erroComunicacao) return LASER_ERRO_COMUNICACAO;
    if (rangeStatus == 4) return LASER_FASE_INVALIDA;
    if (rangeStatus != 0) return LASER_SEM_LEITURA;
    if (distanciaMm < minMm) return LASER_PERTO_DEMAIS;
    if (distanciaMm > maxMm) return LASER_FORA_ESCALA;
    return (int)distanciaMm;
}
inline uint16_t periodoMedicaoMs(uint32_t us, uint16_t margem) {
    return (uint16_t)((us+999u)/1000u + margem);
}
inline bool leituraVencida(uint32_t agora, uint32_t ultima, uint32_t prazo) {
    return (uint32_t)(agora-ultima) > prazo;
}

class FiltroLaser {
public:
    explicit FiltroLaser(float a): alfa(a) {}
    // Returns the invalid code immediately; history is retained for <=2 bad
    // samples, then discarded. Old estimates must never control the robot.
    int atualizar(int leitura) {
        if (leitura < 0) {
            if (++falhasSeguidas >= 3) reiniciar();
            return leitura;
        }
        falhasSeguidas=0;
        janela[proximo]=leitura;
        proximo=(proximo+1)%3;
        if (quantidade<3) ++quantidade;
        int a=janela[0],b=janela[1],c=janela[2];
        float mediana;
        if(quantidade==1) mediana=(float)a;
        else if(quantidade==2) mediana=(a+b)*0.5f;
        else {
            int menor=a<b?a:b, maior=a<b?b:a;
            int meio=c<maior?c:maior;
            mediana=(float)(menor>meio?menor:meio);
        }
        suave=temSuave ? alfa*mediana+(1.0f-alfa)*suave : mediana;
        temSuave=true;
        return (int)(suave+0.5f);
    }
    void reiniciar() { quantidade=0;proximo=0;temSuave=false;falhasSeguidas=0; }
private:
    float alfa;
    int janela[3]={0,0,0};
    uint8_t proximo=0,quantidade=0,falhasSeguidas=0;
    float suave=0;
    bool temSuave=false;
};

enum EstadoParede { PAREDE_INDEFINIDA, PAREDE_AUSENTE, PAREDE_PRESENTE };
class DetectorParede {
public:
    DetectorParede(int com,int sem):limiarCom(com),limiarSem(sem) {}
    EstadoParede atualizar(int leitura) {
        if (leitura<0) return PAREDE_INDEFINIDA; // NEVER use old evidence as fresh
        if (leitura<=limiarCom) conhecido=PAREDE_PRESENTE;
        else if(leitura>limiarSem) conhecido=PAREDE_AUSENTE;
        else if(conhecido==PAREDE_INDEFINIDA)
            conhecido=leitura<=(limiarCom+limiarSem)/2?PAREDE_PRESENTE:PAREDE_AUSENTE;
        return conhecido;
    }
    void reiniciar(){conhecido=PAREDE_INDEFINIDA;}
private:
    int limiarCom,limiarSem;
    EstadoParede conhecido=PAREDE_INDEFINIDA;
};
inline const char* textoParede(EstadoParede e) {
    if(e==PAREDE_PRESENTE)return "SIM";
    if(e==PAREDE_AUSENTE)return "NAO";
    return "INDEFINIDA";
}
inline const char* textoStatusLaser(int v) {
    switch(v) {
        case LASER_SEM_LEITURA:return "LEITURA_INVALIDA";
        case LASER_FORA_ESCALA:return "FORA_FAIXA";
        case LASER_TIMEOUT:return "TIMEOUT";
        case LASER_FASE_INVALIDA:return "FASE_INVALIDA";
        case LASER_PERTO_DEMAIS:return "ABAIXO_MINIMO";
        case LASER_ERRO_COMUNICACAO:return "ERRO_I2C";
        default:return "VALIDO";
    }
}
