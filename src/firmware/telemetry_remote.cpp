#include "telemetry_remote.h"
#include "config.h"
#include "encoders.h"
#include "speed.h"
#include "battery.h"
#include "imu.h"
#include "lasers.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
struct Snapshot {
 float tensao=NAN,corrente=NAN, velCmS=0,yaw=NAN;
 float pct=NAN, velMediaCmS=0, distanciaMm=0;
 int rpmMedio=0,rpmE=0,rpmD=0;
 int esq=-1,frente=-1,dir=-1;
 uint32_t ms=0, enviados=0, falhos=0, latencia=0;
 int http=0;
 bool ativo=false;
};
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
Snapshot dados;
TaskHandle_t tarefa=nullptr;
uint32_t ultimoSnapshot=0;
const char* enderecoServidor() {return TELEMETRIA_HOST;}
void tarefaRede(void*) {
    const String url=String("http://")+enderecoServidor()+":"+String(TELEMETRIA_PORTA)+TELEMETRIA_CAMINHO;
    WiFiClient client;
    while(true){
        vTaskDelay(pdMS_TO_TICKS(TELEMETRIA_PERIODO_MS));
        const bool disponivel=(WiFi.status()==WL_CONNECTED || WiFi.softAPgetStationNum()>0);
        if(!disponivel) continue;
        Snapshot copia;
        portENTER_CRITICAL(&mux);copia=dados;portEXIT_CRITICAL(&mux);
        const float pct=copia.pct;
        if(!isfinite(pct)) continue; // sem leitura do INA219: evita informar carga 0% ficticia ao site
        // Campos planos exigidos pelo backend FastAPI. Nao reportar pose inexistente.
        // Estimativa grosseira por tensao, nao SOC exato; validar com bateria real.
        String corpo;corpo.reserve(600);
        corpo="{\"bateria\":"+String(pct,1)+",\"velocidade\":"+String(fabsf(copia.velCmS),2)+
          ",\"rpm_motor\":"+String(copia.rpmMedio)+",\"connected\":true";
        corpo+=",\"tempo_relativo_ms\":"+String(copia.ms);
        // Media de todos os trechos com motor desde que o robo foi ligado.
        corpo+=",\"velocidade_media\":"+String(copia.velMediaCmS,2)+",\"distancia_mm\":"+String(copia.distanciaMm,0);
        corpo+=",\"energia\":{\"tensao_v\":"+(isfinite(copia.tensao)?String(copia.tensao,3):String("null"))+
         ",\"consumo_ma\":"+(isfinite(copia.corrente)?String(copia.corrente,1):String("null"))+"}";
        corpo+=",\"motores\":{\"rpm_esq\":"+String(copia.rpmE)+",\"rpm_dir\":"+String(copia.rpmD)+"}";
        corpo+=",\"imu\":{\"yaw_deg\":"+(isfinite(copia.yaw)?String(copia.yaw,2):String("null"))+"}";
        corpo+=",\"sensores_distancia_mm\":{\"esq\":"+String(copia.esq)+",\"frente\":"+String(copia.frente)+",\"dir\":"+String(copia.dir)+"}";
        corpo+=",\"posicao_estimada\":null,\"mapa_disponivel\":false,\"status_missao\":\"calibracao\"}";
        const uint32_t t=millis();
        HTTPClient http;
        http.setConnectTimeout(TELEMETRIA_TIMEOUT_MS); // sem backend no ar, desiste rapido
        http.setTimeout(TELEMETRIA_TIMEOUT_MS);
        int codigo=-1;
        if(http.begin(client,url)) {
           http.addHeader("Content-Type","application/json");
           codigo=http.POST((uint8_t*)corpo.c_str(),corpo.length());
           http.end();
        }
        const uint32_t duracao=millis()-t;
        portENTER_CRITICAL(&mux);
        dados.http=codigo;dados.latencia=duracao;
        if(codigo>=200 && codigo<300)++dados.enviados;
        else ++dados.falhos;
        portEXIT_CRITICAL(&mux);
    }
}
}
void iniciarTelemetriaRemota(){
  if(!TELEMETRIA_REMOTA_HABILITADA)return;
  const BaseType_t ok=xTaskCreatePinnedToCore(tarefaRede,"telemetria_http",6144,nullptr,1,&tarefa,0);
  if(ok!=pdPASS)Serial.println("[TELEMETRIA] Erro ao iniciar task HTTP");
}
void atualizarSnapshotRemoto(){
  if(!TELEMETRIA_REMOTA_HABILITADA || millis()-ultimoSnapshot<100)return;
  ultimoSnapshot=millis();
  const DadosVelocidade v=lerVelocidade();
  DadosIMU im=lerIMU();
  LeituraLaser l=lerLaser(LASER_U2),f=lerLaser(LASER_U3),r=lerLaser(LASER_U4);
  // Velocidade medida pelos encoders (speed.cpp). Sem CPR calibrado no painel,
  // os valores ficam em zero: nao estimar velocidade sem a calibracao.
  const float rpmE=v.rpmE, rpmD=v.rpmD;
  const float vel=v.instantaneaCmS;
  portENTER_CRITICAL(&mux);
  dados.tensao=getVoltage();dados.corrente=getCurrent();dados.pct=getBatteryPercent();
  dados.velMediaCmS=v.mediaTotalCmS;dados.distanciaMm=v.distanciaTotalMm;
  dados.velCmS=vel;dados.rpmE=(int)lroundf(rpmE);dados.rpmD=(int)lroundf(rpmD);
  dados.rpmMedio=(abs(dados.rpmE)+abs(dados.rpmD))/2;
  dados.yaw=im.confiavel?im.yawDireitaGraus:NAN;
  dados.esq=l.brutoMm;dados.frente=f.brutoMm;dados.dir=r.brutoMm;
  dados.ms=millis();dados.ativo=true;
  portEXIT_CRITICAL(&mux);
}
String remotoDiagnosticoJson(){
 Snapshot s;portENTER_CRITICAL(&mux);s=dados;portEXIT_CRITICAL(&mux);
 return String("{\"habilitada\":")+(TELEMETRIA_REMOTA_HABILITADA?"true":"false")+
  ",\"celulasConfiguradas\":"+String(BAT_CELULAS)+
  ",\"taskCriada\":"+(tarefa?"true":"false")+
  ",\"enviados\":"+String(s.enviados)+",\"falhos\":"+String(s.falhos)+
  ",\"ultimoHttp\":"+String(s.http)+",\"latenciaMs\":"+String(s.latencia)+"}";
}
