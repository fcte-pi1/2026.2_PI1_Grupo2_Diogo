#include "lasers.h"
#include "config.h"
#include <Adafruit_VL53L0X.h>
#include <Wire.h>
#include <math.h>

namespace {
const uint8_t XSHUT[3]={LASER_XSHUT_U2,LASER_XSHUT_U3,LASER_XSHUT_U4};
const uint8_t ADDR[3]={LASER_ADDR_U2,LASER_ADDR_U3,LASER_ADDR_U4};
const char* NOMES[3]={"U2","U3","U4"};
struct Sensor {
    Adafruit_VL53L0X hw;
    LeituraLaser dado;
    FiltroLaser filtro=FiltroLaser(LASER_EMA_ALFA);
    DetectorParede detector=DetectorParede(LASER_PAREDE_PRESENTE_MM,LASER_PAREDE_AUSENTE_MM);
    uint32_t ultimoPollMs=0;
};
Sensor sensors[3];
uint8_t proximoPoll=0;
uint32_t ultimoPollGlobalMs=0;
struct LinhaCaptura { uint32_t tempoMs; int bruto,filtrado; uint8_t vlStatus; };
LinhaCaptura linhas[LASER_CAPTURE_MAX_AMOSTRAS];
ResumoCapturaLaser captura;
float m2=0;

String numeroOuNull(int n) { return n>=0?String(n):"null"; }
void capturaEvento(uint8_t id, uint32_t agora, const LeituraLaser& d) {
    if(!captura.ativa || captura.indiceSensor!=id)return;
    if(captura.amostras >= captura.objetivo)return;
    linhas[captura.amostras] = {agora-captura.inicioMs,d.brutoMm,d.filtradoMm,d.statusVL};
    ++captura.amostras;
    if(d.brutoMm>=0) {
        ++captura.validas;
        float delta=(float)d.brutoMm-captura.mediaBruta;
        captura.mediaBruta+=delta/captura.validas;
        m2+=delta*((float)d.brutoMm-captura.mediaBruta);
        if(d.filtradoMm>=0) captura.mediaFiltrada+=((float)d.filtradoMm-captura.mediaFiltrada)/captura.validas;
    } else ++captura.invalidas;
    if(captura.amostras >= captura.objetivo) {
        captura.ativa=false; captura.pronta=true;
        captura.fimMs=agora; captura.motivo="CONCLUIDA";
        captura.desvioBruta=captura.validas>=2?sqrtf(m2/(captura.validas-1)):0;
    }
}
void invalidar(uint8_t i,int codigo,uint8_t status,uint32_t agora,bool contaEvento) {
    auto& s=sensors[i];
    auto& d=s.dado;
    d.brutoMm=codigo;
    d.filtradoMm=s.filtro.atualizar(codigo);
    d.parede=s.detector.atualizar(codigo);
    d.statusVL=status;
    if(contaEvento) {
        d.ultimoEventoMs=agora;
        ++d.total; ++d.invalidas;
        capturaEvento(i,agora,d);
    }
}
void iniciarUm(uint8_t i) {
    auto& s=sensors[i];
    s.filtro.reiniciar(); s.detector.reiniciar();
    s.dado=LeituraLaser();
    digitalWrite(XSHUT[i],HIGH);
    delay(20);
    // Adafruit.begin(new_address) changes the factory address 0x29 for this sensor.
    bool ok=s.hw.begin(ADDR[i]);
    if(ok)ok=s.hw.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_DEFAULT);
    if(ok)ok=s.hw.setMeasurementTimingBudgetMicroSeconds(LASER_ORCAMENTO_MEDICAO_US);
    if(ok) {
        uint32_t budget=s.hw.getMeasurementTimingBudgetMicroSeconds();
        uint16_t periodo=periodoMedicaoMs(budget,LASER_FOLGA_PERIODO_MS);
        ok=s.hw.startRangeContinuous(periodo);
        if(ok) {
            s.dado.inicializado=true;
            s.dado.periodoConfiguradoMs=periodo;
            s.dado.validadeMs=(uint32_t)periodo*LASER_CICLOS_VALIDADE;
            s.dado.ultimoEventoMs=millis();
            s.ultimoPollMs=millis();
            Serial.printf("[LASER] %s XSHUT=%u 0x%02X OK periodo=%u ms\n",NOMES[i],XSHUT[i],ADDR[i],periodo);
        }
    }
    if(!ok) {
        digitalWrite(XSHUT[i],LOW);
        Serial.printf("[LASER] %s FALHOU; desativado em XSHUT GPIO %u\n",NOMES[i],XSHUT[i]);
    }
}
} // namespace

void initLasers() {
    for(uint8_t i=0;i<3;++i){
        // Assert shutdown immediately; do not let all devices answer 0x29.
        digitalWrite(XSHUT[i],LOW);
        pinMode(XSHUT[i],OUTPUT);
    }
    delay(70);
    for(uint8_t i=0;i<3;++i) iniciarUm(i);
    ultimoPollGlobalMs=millis();
}

void atualizarLasers() {
    const uint32_t agora=millis();
    if(captura.ativa && (uint32_t)(agora-captura.inicioMs)>LASER_CAPTURE_TIMEOUT_MS) {
        captura.ativa=false;captura.pronta=true;captura.motivo="TIMEOUT_CAPTURA";
        captura.fimMs=agora;
        captura.desvioBruta=captura.validas>=2?sqrtf(m2/(captura.validas-1)):0;
    }
    // Poll ONE device at a time to reduce I2C load and keep the 200 Hz IMU service responsive.
    if((uint32_t)(agora-ultimoPollGlobalMs)<5)return;
    ultimoPollGlobalMs=agora;
    const uint8_t i=proximoPoll;
    proximoPoll=(proximoPoll+1)%3;
    auto& s=sensors[i]; auto& d=s.dado;
    if(!d.inicializado)return;
    if(s.hw.isRangeComplete()) {
        uint16_t mm=s.hw.readRangeResult();
        uint8_t status=s.hw.readRangeStatus();
        bool erro=s.hw.Status!=VL53L0X_ERROR_NONE;
        int bruto=classificarLeitura(erro,status,mm,LASER_MIN_DIAGNOSTICO_MM,LASER_MAX_DIAGNOSTICO_MM);
        if(d.total>0)d.intervaloRealMs=agora-d.ultimoEventoMs;
        d.ultimoEventoMs=agora;
        d.brutoMm=bruto;
        d.statusVL=status;
        d.filtradoMm=s.filtro.atualizar(bruto);
        d.parede=s.detector.atualizar(d.filtradoMm);
        ++d.total;
        if(bruto>=0){++d.validas;d.ultimaValidaMs=agora;} else ++d.invalidas;
        capturaEvento(i,agora,d);
    } else if(leituraVencida(agora,d.ultimoEventoMs,d.validadeMs) && d.brutoMm!=LASER_TIMEOUT) {
        // Timeout is an EVENT, not new distance evidence; do not add synthetic
        // samples to a 100-real-measurement capture.
        invalidar(i,LASER_TIMEOUT,255,agora,false);
    }
}

LeituraLaser lerLaser(uint8_t i) { return i<3?sensors[i].dado:LeituraLaser(); }
bool iniciarCapturaLaser(uint8_t indice,int distancia,uint16_t quantidade) {
    if(indice>=3 || !sensors[indice].dado.inicializado || captura.ativa ||
       distancia<20 || distancia>1000 || quantidade==0 || quantidade>LASER_CAPTURE_MAX_AMOSTRAS) return false;
    captura=ResumoCapturaLaser();
    captura.indiceSensor=indice; captura.referenciaMm=distancia;
    captura.objetivo=quantidade;captura.ativa=true;captura.inicioMs=millis();
    captura.motivo="EM_ANDAMENTO";
    m2=0;
    return true;
}
bool capturaLaserAtiva(){return captura.ativa;}
ResumoCapturaLaser resumoCapturaLaser(){return captura;}
void limparCapturaLaser(){captura=ResumoCapturaLaser();m2=0;}
String jsonLasers() {
    const uint32_t agora=millis();
    String r="{\"ms\":"+String(agora)+",\"larguraChassiAlvoMm\":"+String(LARGURA_CHASSI_ALVO_MM,0)+",\"sensores\":[";
    for(uint8_t i=0;i<3;++i) {
        if(i)r+=',';
        const auto& d=sensors[i].dado;
        bool fresco=d.brutoMm>=0 && d.ultimaValidaMs!=0 &&
          !leituraVencida(agora,d.ultimaValidaMs,d.validadeMs);
        r+="{\"nome\":\""+String(NOMES[i])+"\",\"xshut\":"+String(XSHUT[i])+",\"endereco\":\"0x";
        char hx[3];snprintf(hx,sizeof(hx),"%02X",ADDR[i]);r+=hx;
        r+="\",\"inicializado\":"+String(d.inicializado?"true":"false");
        r+=",\"valido\":"+String(fresco?"true":"false");
        r+=",\"brutoMm\":"+numeroOuNull(d.brutoMm);
        r+=",\"filtradoMm\":"+numeroOuNull(d.filtradoMm);
        r+=",\"codigo\":"+String(d.brutoMm);
        r+=",\"status\":\""+String(d.inicializado?textoStatusLaser(d.brutoMm):"NAO_INICIALIZADO")+"\"";
        r+=",\"rangeStatus\":"+String(d.statusVL);
        r+=",\"parede\":\""+String(fresco?textoParede(d.parede):"INDEFINIDA")+"\"";
        r+=",\"idadeValidaMs\":"+(d.ultimaValidaMs?String(agora-d.ultimaValidaMs):"null");
        r+=",\"idadeEventoMs\":"+String(agora-d.ultimoEventoMs);
        r+=",\"total\":"+String(d.total)+",\"validas\":"+String(d.validas)+",\"invalidas\":"+String(d.invalidas);
        r+=",\"intervaloRealMs\":"+String(d.intervaloRealMs);
        r+=",\"periodoMs\":"+String(d.periodoConfiguradoMs)+"}";
    }
    r+= "],\"captura\":{";
    r+="\"ativa\":"+String(captura.ativa?"true":"false")+",\"pronta\":"+String(captura.pronta?"true":"false");
    r+=",\"sensor\":"+String(captura.indiceSensor)+",\"referenciaMm\":"+String(captura.referenciaMm);
    r+=",\"objetivo\":"+String(captura.objetivo)+",\"amostras\":"+String(captura.amostras);
    r+=",\"validas\":"+String(captura.validas)+",\"invalidas\":"+String(captura.invalidas);
    r+=",\"mediaBruta\":"+String(captura.mediaBruta,3)+",\"desvioBruta\":"+String(captura.desvioBruta,3);
    r+=",\"mediaFiltrada\":"+String(captura.mediaFiltrada,3)+",\"motivo\":\""+captura.motivo+"\"";
    r+=",\"duracaoMs\":"+String((captura.ativa?agora:captura.fimMs)-captura.inicioMs);
    r+="}}";
    return r;
}
String csvCapturaLaser() {
    String r="sensor;referencia_mm;indice;tempo_ms;bruto_mm;filtrado_mm;range_status;valida\r\n";
    for(uint16_t i=0;i<captura.amostras;++i) {
        const auto& v=linhas[i];
        r+=String(NOMES[captura.indiceSensor])+";"+String(captura.referenciaMm)+";"+String(i+1)+";"+String(v.tempoMs)+";";
        r+=String(v.bruto)+";"+String(v.filtrado)+";"+String(v.vlStatus)+";"+String(v.bruto>=0?1:0)+"\r\n";
    }
    return r;
}
