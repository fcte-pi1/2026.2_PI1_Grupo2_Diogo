#include "navigation.h"
#include "web_telemetry.h"
#include "maze_state.h"
#include "maze_planner.h"
#include "lasers.h"
#include "nav_laser_policy.h"
#include "battery.h"
#include "imu.h"
#include "config.h"
#include <math.h>

namespace {
enum class Stage { IDLE, SENSE, TURN_START, TURN_WAIT, MOVE_START, MOVE_WAIT, SETTLE, DONE, FAULT };
Stage stage=Stage::IDLE;
const char* errorText="parado";
uint32_t startMs=0,stageMs=0,lastDriverId=0;
uint32_t sensorCounts[3]={0,0,0};
HistoricoLaserNav sensorHistorico[3];
uint16_t steps=0;
uint8_t x=0,y=0,targetX=0,targetY=0,turnsRemaining=0;
char direction='N',targetDirection='N';
bool turnRight=true;
bool lastMotionWasMove=false;
uint32_t afterSettleMs=0;
const char* stageText(){
 switch(stage){case Stage::IDLE:return "PARADO";case Stage::SENSE:return "OBSERVANDO";
 case Stage::TURN_START:return "PREPARANDO_GIRO";case Stage::TURN_WAIT:return "GIRANDO";
 case Stage::MOVE_START:return "PREPARANDO_AVANCO";case Stage::MOVE_WAIT:return "AVANCANDO";
 case Stage::SETTLE:return "ASSENTANDO";case Stage::DONE:return "OBJETIVO_ATINGIDO";
 default:return "FALHA";}
}
uint8_t dirValue(char d){return d=='N'?0:d=='L'?1:d=='S'?2:3;}
char dirChar(uint8_t d){const char dirs[]={'N','L','S','O'};return dirs[d&3];}
void beginSensing(){
 stage=Stage::SENSE;stageMs=millis();
 for(uint8_t i=0;i<3;++i){
   sensorCounts[i]=lerLaser(i).total;
   sensorHistorico[i].reiniciar(sensorCounts[i]);
 }
}
void fail(const char* why){
 if(stage==Stage::FAULT || stage==Stage::DONE || stage==Stage::IDLE)return;
 errorText=why;stage=Stage::FAULT;
 // A parada de emergencia também notifica o navegador (sem recursao).
 pararTesteDeEmergencia(why);
 Serial.printf("[NAV] PARADA SEGURA: %s\n",why);
}
// Classifica somente evidencia NOVA e recente da posicao atual.
// "SINAL_FRACO" nunca e aceito por falta de comunicacao ou evento vencido;
// somente em modo opt-in, status 2 e 3 amostras consecutivas.
EvidenciaParedeNav classificarSensorNav(uint8_t indice, uint32_t now) {
 const auto d=lerLaser(indice);
 const bool eventoRecente=d.inicializado && d.total>0 && d.ultimoEventoMs>0 &&
   (uint32_t)(now-d.ultimoEventoMs)<=NAV_IDADE_SENSOR_MS;
 const bool validoRecente=d.ultimaValidaMs>0 &&
   (uint32_t)(now-d.ultimaValidaMs)<=NAV_IDADE_SENSOR_MS;
 return sensorHistorico[indice].classificar(d.brutoMm,d.statusVL,d.parede,
    eventoRecente,validoRecente,NAV_SEM_ALVO_COMO_PASSAGEM,
    NAV_SEM_ALVO_AMOSTRAS_MINIMAS);
}
void registrarEventosLaser() {
 for(uint8_t i=0;i<3;++i) {
   const auto d=lerLaser(i);
   sensorHistorico[i].novaAmostra(d.total,d.brutoMm,d.statusVL,d.parede);
 }
}
// Observa somente o que pode ser classificado; nunca registra parede
// desconhecida como livre por simples erro de timeout/comunicacao.
bool inspect(){
 const uint32_t now=millis();
 const auto left=classificarSensorNav(LASER_U2,now);
 const auto front=classificarSensorNav(LASER_U3,now);
 const auto right=classificarSensorNav(LASER_U4,now);
 if(left==EvidenciaParedeNav::INDEFINIDA || front==EvidenciaParedeNav::INDEFINIDA ||
    right==EvidenciaParedeNav::INDEFINIDA)return false;
 const uint8_t h=dirValue(direction);
 const uint8_t bits=(uint8_t)((1u<<h)|(1u<<((h+3)&3))|(1u<<((h+1)&3)));
 uint8_t walls=0;
 if(front==EvidenciaParedeNav::PRESENTE)walls|=1u<<h;
 if(left==EvidenciaParedeNav::PRESENTE)walls|=1u<<((h+3)&3);
 if(right==EvidenciaParedeNav::PRESENTE)walls|=1u<<((h+1)&3);
 return mazeObserve(x,y,bits,walls);
}
float dangerLimitMm(){
 // Conversao RPM -> mm/s. Margem de reacao e freio sao conservadoras,
 // mas sao apenas estimativas: validar distancia real antes da autonomia.
 const float v=NAV_RPM_RETO*3.14159265f*DIAMETRO_RODA_MM/60.0f;
 return NAV_U3_ATE_NARIZ_MM+NAV_FOLGA_FRONTAL_MM+
        v*FRENTE_REACAO_S+v*v/(2.0f*FRENTE_DESACEL_ESTIMADA_MM_S2);
}
}
bool navigationRunning(){return stage!=Stage::IDLE && stage!=Stage::DONE && stage!=Stage::FAULT;}
bool navigationNeedsFrontPoll(){return stage==Stage::MOVE_WAIT;}
void navigationNotifyStop(const char* reason){
 if(navigationRunning()){stage=Stage::FAULT;errorText=reason;}
}
const char* navigationStart(uint8_t w,uint8_t h,uint8_t gx,uint8_t gy){
 if(navigationRunning())return "missao_ja_em_execucao";
 if(!NAV_AUTONOMIA_HABILITADA)return "configure_NAV_AUTONOMIA_HABILITADA_apos_checklist";
 if(!NAV_LATERAIS_CALIBRADOS)return "sensores_laterais_nao_calibrados";
 if(NAV_U3_ATE_NARIZ_MM<0)return "medir_NAV_U3_ATE_NARIZ_MM";
 if(!(NAV_CELULA_MM>=150 && NAV_CELULA_MM<=200))return "passo_de_celula_invalido";
 if(!navigationDriveReady())return "CPR_driver_ou_freio_indisponivel";
 if(!batteryAvailable() || bateriaBaixa() || bateriaCritica())return "bateria_indisponivel_ou_baixa";
 if(!lerIMU().confiavel)return "IMU_nao_calibrada";
 if(!mazeConfigure(w,h,gx,gy))return "dimensoes_ou_objetivo_invalidos";
 const auto now=millis();
 // A inicializacao nao exige uma parede no alcance de cada laser.
 // A observacao (apos 3 amostras novas) decide se existe evidencia de passagem.
 for(uint8_t i=0;i<3;++i)
   if(!lerLaser(i).inicializado)return "laser_nao_inicializado";
 x=0;y=0;direction='N';targetDirection='N';steps=0;
 if(!mazeSetPose(x,y,direction))return "erro_ao_inicializar_pose";
 mazeMarkVisited(x,y);startMs=now;errorText="";beginSensing();
 Serial.printf("[NAV] INICIADA %ux%u objetivo=(%u,%u)\n",w,h,gx,gy);
 return nullptr;
}
void navigationTick(){
 if(!navigationRunning())return;
 const uint32_t now=millis();
 if(now-startMs>NAV_TEMPO_MISSAO_MS){fail("timeout_missao");return;}
 if(steps>=NAV_MAX_PASSOS){fail("limite_passos");return;}
 registrarEventosLaser();
 if(!batteryAvailable() || bateriaCritica() || bateriaBaixa()){fail("bateria_sem_leitura_ou_baixa");return;}
 // Em qualquer etapa, se o controle se encontrar em estado inesperado, parar.
 if((stage==Stage::MOVE_WAIT || stage==Stage::TURN_WAIT) && !navigationDriveIsActive()){
    if(navigationDriveResultId()==lastDriverId){fail("controle_encerrou_sem_resultado");return;}
 }
 if(stage==Stage::MOVE_WAIT){
   const auto f=lerLaser(LASER_U3);
   const auto evidencia=classificarSensorNav(LASER_U3,now);
   if(evidencia==EvidenciaParedeNav::INDEFINIDA){
      fail("U3_sem_evidencia_durante_avanco");return;
   }
   // So uma distancia REAL e recente pode determinar a folga dinamica.
   if(f.brutoMm>=0 && f.brutoMm<=dangerLimitMm()){
     fail("obstaculo_frontal_limite_dinamico");return;
   }
 }
 switch(stage){
 case Stage::SENSE: {
   if(now-stageMs>NAV_TEMPO_SENSORES_MS){fail("sensores_sem_tres_amostras_validas");return;}
   bool enough=true;
   for(uint8_t i=0;i<3;++i)if((uint32_t)(lerLaser(i).total-sensorCounts[i])<3u)enough=false;
   if(!enough)return;
   // Leitura intermitente: esperar amostras adicionais ate o prazo configurado.
   if(!inspect()){
      if(now-stageMs<NAV_TEMPO_SENSORES_MS)return;
      fail("parede_indefinida_ou_leitura_contraditoria");return;
   }
   mazeMarkVisited(x,y);
   if(mazeIsGoal()){stage=Stage::DONE;errorText="objetivo_atingido";Serial.println("[NAV] OBJETIVO ATINGIDO");return;}
   uint8_t nx,ny;
   if(!mazeNextDirection(targetDirection,nx,ny)){fail("sem_caminho_conhecido_seguro");return;}
   targetX=nx;targetY=ny;
   const uint8_t delta=(dirValue(targetDirection)+4u-dirValue(direction))&3u;
   turnsRemaining=(delta==2)?2:((delta==0)?0:1);
   turnRight=delta!=3;
   stage=turnsRemaining?Stage::TURN_START:Stage::MOVE_START;
   stageMs=now;
   return;
 }
 case Stage::TURN_START:
   if(!navigationDriveReady()) {
      if(now-stageMs>3000)fail("freio_nao_liberou_para_giro");
      return;
   }
   if(!navigationDriveStartTurn(turnRight,NAV_RPM_GIRO)){fail("nao_iniciou_giro");return;}
   lastDriverId=navigationDriveResultId();stage=Stage::TURN_WAIT;stageMs=now;return;
 case Stage::TURN_WAIT:
   if(navigationDriveResultId()==lastDriverId)return;
   if(!navigationDriveResultOk()){fail(navigationDriveResultReason());return;}
   direction=dirChar((dirValue(direction)+(turnRight?1:3))&3);
   mazeSetPose(x,y,direction);
   --turnsRemaining;
   lastMotionWasMove=false;afterSettleMs=now;
   stage=Stage::SETTLE;return;
 case Stage::MOVE_START: {
   if(!mazeOpenFromPose(targetDirection)){fail("parede_desconhecida_ou_presente");return;}
   const auto f=lerLaser(LASER_U3);
   const auto evidencia=classificarSensorNav(LASER_U3,now);
   if(evidencia==EvidenciaParedeNav::INDEFINIDA ||
      (f.brutoMm>=0 && f.brutoMm<=dangerLimitMm()+20.0f)){
      fail("U3_sem_folga_ou_evidencia_para_avancar");return;
   }
   if(!navigationDriveReady()){
      if(now-stageMs>3000)fail("freio_nao_liberou_para_avanco");
      return;
   }
   if(!navigationDriveStartStraight(NAV_CELULA_MM,NAV_RPM_RETO)){fail("nao_iniciou_avanco");return;}
   lastDriverId=navigationDriveResultId();stage=Stage::MOVE_WAIT;stageMs=now;return;
 }
 case Stage::MOVE_WAIT:
   if(navigationDriveResultId()==lastDriverId)return;
   if(!navigationDriveResultOk()){fail(navigationDriveResultReason());return;}
   // Pose atualizada APENAS apos o controlador atestar a meta.
   x=targetX;y=targetY;mazeSetPose(x,y,direction);++steps;
   lastMotionWasMove=true;afterSettleMs=now;stage=Stage::SETTLE;return;
 case Stage::SETTLE:
   if(now-afterSettleMs<350 || !navigationDriveReady())return;
   if(lastMotionWasMove){beginSensing();return;}
   // Depois do giro, observar novamente a cena antes de autorizar o avanco.
   if(turnsRemaining){stage=Stage::TURN_START;stageMs=now;return;}
   // Exigir nova leitura frontal no novo rumo, e atualizar mapa de modo seguro.
   beginSensing();return;
 default:return;
 }
}
String navigationStatusJson(){
 String s="{\"estado\":\""+String(stageText())+"\",\"ativo\":"+
  String(navigationRunning()?"true":"false")+",\"motivo\":\""+String(errorText)+
  "\",\"passos\":"+String(steps)+",\"x\":"+String(x)+",\"y\":"+String(y)+
  ",\"direcao\":\""+String(direction)+"\",\"proximaDirecao\":\""+String(targetDirection)+
  "\",\"limiteFrontalMm\":"+String(dangerLimitMm(),1)+
  ",\"autonomiaLiberada\":"+String(NAV_AUTONOMIA_HABILITADA?"true":"false")+
  ",\"lateraisCalibrados\":"+String(NAV_LATERAIS_CALIBRADOS?"true":"false")+
  ",\"laserU3AteNarizMm\":"+String(NAV_U3_ATE_NARIZ_MM,1)+
  ",\"modoSemAlvoHabilitado\":"+String(NAV_SEM_ALVO_COMO_PASSAGEM?"true":"false")+
  ",\"semAlvoU3Seguidas\":"+String(sensorHistorico[LASER_U3].semAlvoSeguidas())+"}";
 return s;
}
