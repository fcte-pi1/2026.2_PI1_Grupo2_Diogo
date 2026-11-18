#include "navigation.h"
#include "web_telemetry.h"
#include "maze_state.h"
#include "maze_planner.h"
#include "lasers.h"
#include "nav_laser_policy.h"
#include "nav_opening.h"
#include "nav_wall_arrival.h"
#include "battery.h"
#include "imu.h"
#include "config.h"
#include <math.h>

namespace {
enum class Stage { IDLE, SENSE, TURN_START, TURN_WAIT, MOVE_START, MOVE_WAIT, WALL_SETTLE, SETTLE, DONE, FAULT };
Stage stage=Stage::IDLE;
const char* errorText="parado";
uint32_t startMs=0,stageMs=0,lastDriverId=0;
uint32_t sensorCounts[3]={0,0,0};
HistoricoLaserNav sensorHistorico[3];
uint16_t steps=0,reverseSteps=0;
uint8_t x=0,y=0,targetX=0,targetY=0,turnsRemaining=0;
char direction='N',targetDirection='N';
bool turnRight=true, reversing=false, poseCertain=false;
bool lastMotionWasMove=false, adjustmentApplied=false;
uint32_t afterSettleMs=0;
uint16_t inferredFaces=0;
uint8_t frontReplans=0; // replanejamentos seguidos sem sair da celula
float yawStart=0, centerRef=-1, plannedMm=0, openingEstimate=NAN, edgeLeft=NAN,edgeRight=NAN;
float centerRefEsq=-1,centerRefDir=-1,wallStoppedMm=NAN,wallStoppedSync=NAN;
uint16_t wallArrivals=0;
// V4.9.2: rumo absoluto esperado (yaw da IMU) e manobras auxiliares.
float expectedYaw=0, lastTurnTarget=NAN, lastAlignDeg=NAN;
bool aligning=false, nudging=false, pendingBackoff=false;
uint8_t alignTries=0;
uint16_t alignCount=0, backoffCount=0;
// V4.9.3: geometria do corredor APRENDIDA durante a missao e desvio lateral.
float learnedHalf=-1, learnedWidth=-1, lateralOffset=NAN, lastSteerDeg=NAN;
uint16_t steerCount=0;
NavOpening openingLeft,openingRight;
NavigationOptions options;
// Diagnostico do ultimo evento frontal do deslocamento. Nao altera o laser.
struct FrontMoveEvidence {
 bool measured=false;
 uint32_t measuredMs=0;
 float measuredMm=NAN, measuredProgress=NAN;
 int raw=LASER_SEM_LEITURA;
 int range=255;
 uint32_t eventAge=0, validAge=0;
 float progress=NAN, remaining=NAN;
 const char* decision="sem_movimento";
} frontMove;

const char* stageText(){
 switch(stage){case Stage::IDLE:return "PARADO";case Stage::SENSE:return "OBSERVANDO";
 case Stage::TURN_START:return "PREPARANDO_GIRO";case Stage::TURN_WAIT:return "GIRANDO";
 case Stage::MOVE_START:return reversing?"PREPARANDO_RE":"PREPARANDO_AVANCO";
 case Stage::MOVE_WAIT:return reversing?"RECUANDO":"AVANCANDO";
 case Stage::WALL_SETTLE:return "VERIFICANDO_PARADA_FRONTAL";
 case Stage::SETTLE:return "ASSENTANDO";case Stage::DONE:return "OBJETIVO_ATINGIDO";
 default:return "FALHA";}
}
uint8_t dirValue(char d){return d=='N'?0:d=='L'?1:d=='S'?2:3;}
char dirChar(uint8_t d){const char dirs[]={'N','L','S','O'};return dirs[d&3];}
void invalidateIfMoving(){
 if(stage==Stage::MOVE_WAIT||stage==Stage::TURN_WAIT||stage==Stage::WALL_SETTLE||stage==Stage::SETTLE){
  poseCertain=false;mazeInvalidatePose();
 }
}
void beginSensing(){
 stage=Stage::SENSE;stageMs=millis();
 for(uint8_t i=0;i<3;++i){sensorCounts[i]=lerLaser(i).total;sensorHistorico[i].reiniciar(sensorCounts[i]);}
}
void fail(const char* why){
 if(stage==Stage::FAULT || stage==Stage::DONE || stage==Stage::IDLE)return;
 invalidateIfMoving();errorText=why;stage=Stage::FAULT;
 pararTesteDeEmergencia(why);
 Serial.printf("[NAV] PARADA SEGURA: %s\n",why);
}
bool eventFresh(const LeituraLaser& d,uint32_t now){return d.inicializado&&d.total>0&&d.ultimoEventoMs>0&&now-d.ultimoEventoMs<=NAV_IDADE_SENSOR_MS;}
bool validFresh(const LeituraLaser& d,uint32_t now){return eventFresh(d,now)&&d.brutoMm>=0&&d.ultimaValidaMs>0&&now-d.ultimaValidaMs<=NAV_IDADE_SENSOR_MS;}
EvidenciaParedeNav classificarSensorNav(uint8_t indice,uint32_t now){
 const auto d=lerLaser(indice);
 // Um evento anterior a mudanca de pose/rumo nao serve como observacao da nova celula.
 if(stage==Stage::SENSE && (uint32_t)(d.total-sensorCounts[indice])<3)return EvidenciaParedeNav::INDEFINIDA;
 return sensorHistorico[indice].classificar(d.brutoMm,d.statusVL,d.parede,eventFresh(d,now),validFresh(d,now),
    NAV_SEM_ALVO_COMO_PASSAGEM,NAV_SEM_ALVO_AMOSTRAS_MINIMAS);
}
void registrarEventosLaser(){for(uint8_t i=0;i<3;++i){const auto d=lerLaser(i);sensorHistorico[i].novaAmostra(d.total,d.brutoMm,d.statusVL,d.parede);}}
// Registra apenas as faces observadas. Uma face indefinida nao impede o uso de
// OUTRA passagem comprovada, e nunca vira automaticamente uma parede aberta.
bool inspect(uint8_t& known,uint8_t& wall){
 known=wall=0;
 const uint32_t now=millis();const uint8_t h=dirValue(direction);
 const uint8_t absolute[3]={(uint8_t)((h+3)&3),h,(uint8_t)((h+1)&3)};
 for(uint8_t i=0;i<3;++i){const auto e=classificarSensorNav(i,now);
  if(e==EvidenciaParedeNav::INDEFINIDA)continue;
  known|=1u<<absolute[i];if(e==EvidenciaParedeNav::PRESENTE)wall|=1u<<absolute[i];
 }
 return true;
}
float dangerLimitMm(){
 const float v=NAV_RPM_RETO*3.14159265f*DIAMETRO_RODA_MM/60.0f;
 return NAV_U3_ATE_NARIZ_MM+NAV_FOLGA_FRONTAL_MM+v*FRENTE_REACAO_S+v*v/(2.0f*FRENTE_DESACEL_ESTIMADA_MM_S2);
}
// So usa uma distancia medida nesta mesma reta, dentro do prazo original.
// A distancia restante desconta o avanco dos encoders e uma margem adicional.
// Nao libera erro I2C, timeout, fase/sigma invalida ou leitura perto demais.
void rememberFront(const LeituraLaser& f,uint32_t now,float progress){
 if(!validFresh(f,now)||!isfinite(progress)||progress<0)return;
 if(isfinite(frontMove.measuredMm) && frontMove.measuredMs==f.ultimaValidaMs)return;
 frontMove.measured= f.statusVL==0 && f.parede==PAREDE_AUSENTE && f.brutoMm>LASER_PAREDE_AUSENTE_MM;
 frontMove.measuredMs=f.ultimaValidaMs;
 frontMove.measuredMm=(float)f.brutoMm;
 frontMove.measuredProgress=progress;
}
const char* checkFrontMove(const LeituraLaser& f,uint32_t now,float progress){
 frontMove.raw=f.brutoMm;frontMove.range=f.statusVL;
 frontMove.eventAge=f.ultimoEventoMs?now-f.ultimoEventoMs:UINT32_MAX;
 frontMove.validAge=f.ultimaValidaMs?now-f.ultimaValidaMs:UINT32_MAX;
 frontMove.progress=progress;frontMove.remaining=NAN;
 if(!eventFresh(f,now)){frontMove.decision="evento_antigo_ou_indisponivel";return "U3_sem_evidencia_durante_avanco";}
 if(f.brutoMm==LASER_PERTO_DEMAIS || (f.brutoMm>=0&&f.brutoMm<=dangerLimitMm())){
  frontMove.decision="obstaculo";return "obstaculo_frontal_limite_dinamico";
 }
 if(validFresh(f,now)&&f.statusVL==0){
  rememberFront(f,now,progress);
  frontMove.decision="distancia_valida";return nullptr;
 }
 // Somente para sinal fraco (status 2); demais erros continuam bloqueando.
 if(f.brutoMm==LASER_SINAL_FRACO&&f.statusVL==2){
  if(!isfinite(progress)||progress<0){frontMove.decision="odometria_invalida";return "U3_sem_evidencia_durante_avanco";}
  if(isfinite(frontMove.measuredMm)&&isfinite(frontMove.measuredProgress)){
   if(progress<frontMove.measuredProgress){frontMove.decision="odometria_inconsistente";return "U3_sem_evidencia_durante_avanco";}
   frontMove.remaining=frontMove.measuredMm-(progress-frontMove.measuredProgress);
   // Uma parede medida que se aproximou do limite nao deve ser mascarada,
   // mesmo quando o perfil experimental ja existente esta ativo.
   if(frontMove.remaining<=dangerLimitMm()+20.0f){
    frontMove.decision="ultima_distancia_sem_folga";return "U3_sem_evidencia_durante_avanco";
   }
   if(frontMove.measured&&now-frontMove.measuredMs<=NAV_IDADE_SENSOR_MS){
    frontMove.decision="folga_medida_recente";return nullptr;
   }
  }
  if(classificarSensorNav(LASER_U3,now)==EvidenciaParedeNav::LIVRE){frontMove.decision="sem_alvo_perfil_existente";return nullptr;}
 }
 frontMove.decision="evidencia_insuficiente";return "U3_sem_evidencia_durante_avanco";
}
float openingWidth(){return learnedWidth>0?learnedWidth:options.corridorWidth;}
// Mede, parado na celula, o desvio lateral em relacao ao centro do corredor.
// Positivo = robo deslocado para a DIREITA. Duas paredes: (E-D)/2 e aprende a
// distancia de cada sensor a parede com o robo centrado (meia largura medida).
// Uma parede: usa essa meia largura aprendida. Nenhuma: desconhecido.
void measureLateral(uint32_t now){
 lateralOffset=NAN;
 float v[2];bool ok[2];const uint8_t ids[2]={LASER_U2,LASER_U4};
 for(uint8_t i=0;i<2;++i){const auto d=lerLaser(ids[i]);
  const int mm=d.filtradoMm>=0?d.filtradoMm:d.brutoMm;
  ok[i]=validFresh(d,now)&&d.statusVL==0&&d.parede==PAREDE_PRESENTE&&mm>=0&&mm<=CENTRO_PAREDE_MAX_MM;
  v[i]=(float)mm;
 }
 if(ok[0]&&ok[1]){
  const float half=0.5f*(v[0]+v[1]);
  learnedHalf=learnedHalf<0?half:learnedHalf+0.3f*(half-learnedHalf);
  if(NAV_SENSOR_LATERAL_AO_CENTRO_MM>0){
   const float w=2.0f*(learnedHalf+NAV_SENSOR_LATERAL_AO_CENTRO_MM);
   if(w>=100.0f&&w<=NAV_CELULA_MM)learnedWidth=w;
  }
  lateralOffset=0.5f*(v[0]-v[1]);
 } else if(ok[0]&&learnedHalf>0) lateralOffset=v[0]-learnedHalf;
 else if(ok[1]&&learnedHalf>0) lateralOffset=learnedHalf-v[1];
}
void trackOpenings(uint32_t now){
 const float p=(reversing?-1.0f:1.0f)*navigationDriveProgressMm();
 if(!isfinite(p)||p<0)return;
 const uint8_t h=dirValue(direction);
 const uint8_t ids[2]={LASER_U2,LASER_U4};
 NavOpening* trackers[2]={&openingLeft,&openingRight};
 const float offsets[2]={options.sensorLeftToAxle,options.sensorRightToAxle};
 const char dirs[2]={dirChar((h+3)&3),dirChar((h+1)&3)};
 for(uint8_t i=0;i<2;++i){
  const auto d=lerLaser(ids[i]);auto& tracker=*trackers[i];
  // V4.9.3: "sem alvo" (corredor aberto sem retorno optico) conta como abertura;
  // antes zerava o rastreador e a borda nunca era encontrada. Timeout/I2C nao.
  const bool noTarget=eventFresh(d,now)&&laserSemAlvoNav(d.brutoMm);
  if(tracker.update(d.total,validFresh(d,now)||noTarget,d.brutoMm>=0&&d.brutoMm<=LASER_PAREDE_PRESENTE_MM,
        noTarget||d.brutoMm>=LASER_PAREDE_AUSENTE_MM,p)){
   if(i==0)edgeLeft=tracker.edgeMm();else edgeRight=tracker.edgeMm();
  }
  const float wOpen=openingWidth();
  if(!tracker.found() || !isfinite(offsets[i]) || fabsf(offsets[i])>NAV_CELULA_MM/2 ||
     wOpen<=0||wOpen>NAV_CELULA_MM)continue;
  const float est=tracker.centerMm(wOpen,(reversing?-1.0f:1.0f)*offsets[i]);
  openingEstimate=est;
  // Ajuste somente em abertura mapeada do destino, e perto da meta nominal.
  // O retorno sem sensor traseiro nao pode ser estendido por uma estimativa.
  if(!options.opening || adjustmentApplied || !mazeOpenAt(targetX,targetY,dirs[i]) ||
     // Na re so passa do nominal se o trecho de tras ja foi percorrido (sem sensor traseiro).
     (reversing&&est>NAV_CELULA_MM&&!mazeTraversedAt(targetX,targetY,dirChar((h+2)&3))) || !navOpeningAdjustmentOk(est,NAV_CELULA_MM,p,NAV_AJUSTE_MAX_MM,NAV_AJUSTE_RESTANTE_MIN_MM))continue;
  if(navigationDriveAdjustStraight((reversing?-1.0f:1.0f)*est)){plannedMm=est;adjustmentApplied=true;}
 }
}
String nOrNull(float f){return isfinite(f)?String(f,1):String("null");}
}
bool navigationRunning(){return stage!=Stage::IDLE && stage!=Stage::DONE && stage!=Stage::FAULT;}
bool navigationNeedsFrontPoll(){return stage==Stage::MOVE_WAIT && !reversing;}
void navigationNotifyStop(const char* reason){if(navigationRunning()){invalidateIfMoving();stage=Stage::FAULT;errorText=reason;}}
NavigationOptions navigationDefaultOptions(){
 NavigationOptions o;
 o.reverse=NAV_RE_HABILITADA;o.center=NAV_CENTRALIZAR_HABILITADA;o.opening=NAV_AJUSTE_ABERTURA_HABILITADO;
 o.corridorWidth=NAV_ABERTURA_LARGURA_MM;o.sensorLeftToAxle=NAV_U2_A_EIXO_MM;o.sensorRightToAxle=NAV_U4_A_EIXO_MM;return o;
}
bool navigationReverseAllowed(){return navigationRunning()&&options.reverse;}
bool navigationCenterEnabled(){return navigationRunning()&&options.center;}
const char* navigationStart(uint8_t w,uint8_t h,uint8_t gx,uint8_t gy){return navigationStartWithOptions(w,h,gx,gy,navigationDefaultOptions());}
const char* navigationStartWithOptions(uint8_t w,uint8_t h,uint8_t gx,uint8_t gy,const NavigationOptions& requested){
 if(navigationRunning())return "missao_ja_em_execucao";
 if(requested.initialDirection!='N'&&requested.initialDirection!='L'&&requested.initialDirection!='S'&&requested.initialDirection!='O')return "orientacao_inicial_invalida";
 if(!NAV_AUTONOMIA_HABILITADA)return "configure_NAV_AUTONOMIA_HABILITADA_apos_checklist";
 if(!NAV_LATERAIS_CALIBRADOS)return "sensores_laterais_nao_calibrados";
 if((requested.opening||requested.corridorWidth!=0) && (!isfinite(requested.corridorWidth)||requested.corridorWidth<=0||requested.corridorWidth>NAV_CELULA_MM||
    !isfinite(requested.sensorLeftToAxle)||fabsf(requested.sensorLeftToAxle)>NAV_CELULA_MM/2||
    !isfinite(requested.sensorRightToAxle)||fabsf(requested.sensorRightToAxle)>NAV_CELULA_MM/2))return "geometria_abertura_nao_validada";
 if(NAV_U3_ATE_NARIZ_MM<0)return "medir_NAV_U3_ATE_NARIZ_MM";
 if(!(NAV_CELULA_MM>=150 && NAV_CELULA_MM<=200))return "passo_de_celula_invalido";
 if(!navigationDriveReady())return "CPR_driver_ou_freio_indisponivel";
 if(!batteryAvailable()||bateriaBaixa()||bateriaCritica())return "bateria_indisponivel_ou_baixa";
 if(!lerIMU().confiavel)return "IMU_nao_calibrada";
 for(uint8_t i=0;i<3;++i)if(!lerLaser(i).inicializado)return "laser_nao_inicializado";
 if(!mazeConfigure(w,h,gx,gy))return "dimensoes_ou_objetivo_invalidos";
 if(requested.startX>=w||requested.startY>=h)return "posicao_inicial_invalida";
 if(requested.startX==gx&&requested.startY==gy)return "inicio_igual_ao_objetivo";
 x=targetX=requested.startX;y=targetY=requested.startY;direction=targetDirection=requested.initialDirection;steps=reverseSteps=0;inferredFaces=0;frontReplans=0;
 reversing=false;centerRef=centerRefEsq=centerRefDir=-1;wallStoppedMm=wallStoppedSync=NAN;wallArrivals=0;edgeLeft=edgeRight=openingEstimate=NAN;plannedMm=0;adjustmentApplied=false;frontMove=FrontMoveEvidence{};
 if(!mazeSetPose(x,y,direction))return "erro_ao_inicializar_pose";
 expectedYaw=lerIMU().yawDireitaGraus;lastTurnTarget=lastAlignDeg=NAN;
 aligning=nudging=pendingBackoff=false;alignTries=0;alignCount=backoffCount=0;
 learnedHalf=learnedWidth=-1;lateralOffset=lastSteerDeg=NAN;steerCount=0;
 options=requested;poseCertain=true;mazeMarkVisited(x,y);startMs=millis();errorText="";beginSensing();
 Serial.printf("[NAV] INICIADA %ux%u inicio=(%u,%u,%c) objetivo=(%u,%u)\n",w,h,x,y,direction,gx,gy);return nullptr;
}
void navigationTick(){
 if(!navigationRunning())return;
 const uint32_t now=millis();
 if(now-startMs>NAV_TEMPO_MISSAO_MS){fail("timeout_missao");return;}
 if(steps>=NAV_MAX_PASSOS){fail("limite_passos");return;}
 registrarEventosLaser();
 if(!batteryAvailable()||bateriaCritica()||bateriaBaixa()){fail("bateria_sem_leitura_ou_baixa");return;}
 if(!lerIMU().confiavel){fail("IMU_sem_referencia_confiavel");return;}
 if((stage==Stage::MOVE_WAIT||stage==Stage::TURN_WAIT)&&!navigationDriveIsActive()&&navigationDriveResultId()==lastDriverId){fail("controle_encerrou_sem_resultado");return;}
 if(stage==Stage::MOVE_WAIT && navigationDriveIsActive()){
  if(fabsf(lerIMU().yawDireitaGraus-yawStart)>NAV_YAW_RETO_MAX_GRAUS){fail("desvio_angular_reto");return;}
  if(!reversing){
   const auto f=lerLaser(LASER_U3);
   if(const char* reason=checkFrontMove(f,now,navigationDriveProgressMm())){
    // O corte ocorre no MESMO instante e no MESMO limiar de antes.
    // So uma distancia valida (nao "perto demais" nem erro) pode iniciar a
    // verificacao de chegada. Nenhum movimento enquanto a pose e verificada.
    if(f.brutoMm>=0&&f.statusVL==0&&validFresh(f,now)&&f.brutoMm<=dangerLimitMm()){
     if(!navigationDriveStopAtWall()){fail(reason);return;}
     stage=Stage::WALL_SETTLE;afterSettleMs=now;return;
    }
    fail(reason);return;
   }
  }
  for(uint8_t i=LASER_U2;i<=LASER_U4;i+=2){const auto l=lerLaser(i);
   if(!eventFresh(l,now)||l.brutoMm==LASER_ERRO_COMUNICACAO||l.brutoMm==LASER_TIMEOUT||
      l.brutoMm==LASER_PERTO_DEMAIS||(l.brutoMm>=0&&l.brutoMm<10)){fail("lateral_sem_comunicacao_ou_folga");return;}
  }
  trackOpenings(now);
 }
 switch(stage){
 case Stage::SENSE: {
  if(mazeIsGoal()){stage=Stage::DONE;errorText="objetivo_atingido";return;}
  bool enough=true;for(uint8_t i=0;i<3;++i)if((uint32_t)(lerLaser(i).total-sensorCounts[i])<3u)enough=false;
  if(!enough && now-stageMs<NAV_TEMPO_SENSORES_MS)return;
  uint8_t known,wall;inspect(known,wall);
  const uint8_t hd=dirValue(direction);
  const uint8_t faces=(uint8_t)((1u<<hd)|(1u<<((hd+3)&3))|(1u<<((hd+1)&3)));
  // Espera evidencias adicionais quando uma face ainda e desconhecida.
  if(known!=faces && now-stageMs<NAV_TEMPO_SENSORES_MS)return;
  // V4.9.1: face que continuou sem evidencia apos o prazo e tratada como
  // BLOQUEADA por precaucao (o robo nunca entra nela), mas so se o mapa ainda
  // nao a conhecia; uma leitura valida futura pode reabri-la. Antes isso
  // deixava o mapa sem nenhuma passagem e a missao falhava.
  const uint8_t inferred=(uint8_t)(faces&~known);
  if(inferred){++inferredFaces;Serial.printf("[NAV] (%u,%u) faces sem leitura tratadas como parede: 0x%X\n",x,y,inferred);}
  // V4.9.1: leitura que discorda do mapa nao trava mais a missao; a mais
  // recente prevalece (borda externa e passagens percorridas sao preservadas).
  if(!mazeUpdate(x,y,known,wall,inferred)){fail("posicao_fora_do_mapa");return;}
  measureLateral(now); // V4.9.3: inclusive logo depois de um giro
  mazeMarkVisited(x,y);
  uint8_t nx,ny;if(!mazeNextDirection(targetDirection,nx,ny)){fail("sem_passagem_confirmada_ate_objetivo");return;}
  targetX=nx;targetY=ny;
  const uint8_t delta=(dirValue(targetDirection)+4u-dirValue(direction))&3u;
  reversing=delta==2&&mazeTraversedFromPose(targetDirection);
  if(reversing&&!options.reverse){fail("retorno_re_pendente_validacao_traseira");return;}
  turnsRemaining=reversing?0:(delta==2?2:(delta==0?0:1));turnRight=delta!=3;
  // V4.9.2: recuo curto antes de um giro de 90, so se a celula de tras ja foi
  // percorrida (espaco livre comprovado). Ver NAV_GIRO_RECUO_MM no config.h.
  pendingBackoff=turnsRemaining==1 && NAV_GIRO_RECUO_MM>0.5f && NAV_GIRO_RECUO_MM<=40.0f &&
                 mazeTraversedFromPose(dirChar((dirValue(direction)+2)&3));
  stage=turnsRemaining?Stage::TURN_START:Stage::MOVE_START;stageMs=now;return;
 }
 case Stage::TURN_START:
  if(!navigationDriveReady()){if(now-stageMs>3000)fail("freio_nao_liberou_para_giro");return;}
  if(pendingBackoff){
   pendingBackoff=false;
   if(!navigationDriveStartNudge(-NAV_GIRO_RECUO_MM)){fail("nao_iniciou_recuo_antes_do_giro");return;}
   nudging=true;++backoffCount;
   lastDriverId=navigationDriveResultId();stage=Stage::TURN_WAIT;stageMs=now;return;
  }
  yawStart=lerIMU().yawDireitaGraus;
  // V4.9.2: alvo = rumo esperado depois do giro - yaw atual. Inclui o erro
  // acumulado, que a correcao pela IMU elimina ao final do giro.
  expectedYaw+=turnRight?90.0f:-90.0f;
  lastTurnTarget=expectedYaw-yawStart;
  if(!navigationDriveStartTurn(turnRight,NAV_RPM_GIRO,lastTurnTarget)){fail("nao_iniciou_giro");return;}
  lastDriverId=navigationDriveResultId();stage=Stage::TURN_WAIT;stageMs=now;return;
 case Stage::TURN_WAIT:
  if(navigationDriveResultId()==lastDriverId)return;
  if(!navigationDriveResultOk()){fail(navigationDriveResultReason());return;}
  lastMotionWasMove=false;afterSettleMs=now;stage=Stage::SETTLE;return;
 case Stage::MOVE_START: {
  if(!mazeOpenFromPose(targetDirection)){fail("parede_desconhecida_ou_presente");return;}
  if(reversing){if(!options.reverse||!mazeTraversedFromPose(targetDirection)){fail("re_sem_caminho_percorrido");return;}}
  else {
   const auto f=lerLaser(LASER_U3);
   const auto ev=classificarSensorNav(LASER_U3,now);
   const bool semFolga=f.brutoMm>=0&&f.brutoMm<=dangerLimitMm()+20;
   if(ev==EvidenciaParedeNav::PRESENTE||semFolga){
    // V4.9.1: o sensor ve parede onde o mapa dizia passagem: corrige o mapa
    // e replaneja a partir desta celula, em vez de abortar a missao.
    Serial.println("[NAV] U3 ve parede a frente: mapa corrigido, replanejando");
    mazeUpdate(x,y,(uint8_t)(1u<<dirValue(direction)),(uint8_t)(1u<<dirValue(direction)),0);
    if(++frontReplans>4){fail("parede_a_frente_em_passagem_ja_percorrida");return;}
    beginSensing();return;
   }
   if(ev!=EvidenciaParedeNav::LIVRE){
    if(now-stageMs<NAV_TEMPO_SENSORES_MS)return; // aguarda nova leitura frontal
    Serial.println("[NAV] U3 sem evidencia antes do avanco: tratando como parede e replanejando");
    mazeUpdate(x,y,(uint8_t)(1u<<dirValue(direction)),(uint8_t)(1u<<dirValue(direction)),0);
    if(++frontReplans>4){fail("parede_a_frente_em_passagem_ja_percorrida");return;}
    beginSensing();return;
   }
  }
  if(!navigationDriveReady()){if(now-stageMs>3000)fail("freio_nao_liberou_para_deslocamento");return;}
  // V4.9.2: antes de andar, alinha o rumo com o esperado (erro de giros,
  // escorregamento ou do posicionamento inicial). Nao vale para a re.
  if(!reversing && NAV_GIRO_CORRIGIR_IMU && alignTries<NAV_ALINHAR_TENTATIVAS){
   // V4.9.3: com desvio lateral medido, sai da celula levemente inclinado para
   // o centro: na reta de uma celula o robo volta ao meio do corredor. A
   // proxima reta (ou giro) volta ao rumo esperado.
   float steer=0;
   if(isfinite(lateralOffset)&&fabsf(lateralOffset)>NAV_DESVIO_MIN_MM){
    const float smax=sinf(NAV_DESVIO_MAX_GRAUS*0.0174533f);
    float r=lateralOffset/NAV_CELULA_MM;if(r>smax)r=smax;if(r<-smax)r=-smax;
    steer=asinf(r)*57.29578f;
   }
   const float err=(expectedYaw-steer)-lerIMU().yawDireitaGraus;
   if(fabsf(err)>NAV_ALINHAR_GRAUS && fabsf(err)<=NAV_ALINHAR_MAX_GRAUS){
    if(!navigationDriveStartAlign(err)){fail("nao_iniciou_alinhamento");return;}
    aligning=true;++alignTries;++alignCount;lastAlignDeg=err;
    lastDriverId=navigationDriveResultId();stage=Stage::TURN_WAIT;stageMs=now;return;
   }
  }
  alignTries=0;
  if(!reversing&&isfinite(lateralOffset)&&fabsf(lateralOffset)>NAV_DESVIO_MIN_MM){++steerCount;lastSteerDeg=expectedYaw-lerIMU().yawDireitaGraus;}
  lateralOffset=NAN; // medida vale so para a saida desta celula
  yawStart=lerIMU().yawDireitaGraus;plannedMm=NAV_CELULA_MM;adjustmentApplied=false;
  edgeLeft=edgeRight=openingEstimate=NAN;openingLeft.reset(lerLaser(LASER_U2).total);openingRight.reset(lerLaser(LASER_U4).total);
  frontMove=FrontMoveEvidence{};
  if(!reversing)rememberFront(lerLaser(LASER_U3),now,0.0f);
  if(!navigationDriveStartStraight(reversing?-plannedMm:plannedMm,reversing?NAV_RPM_RE:NAV_RPM_RETO)){fail("nao_iniciou_deslocamento");return;}
  if(!reversing&&centerRef>0)navigationDriveSetCenterRef(centerRef);
  if(!reversing&&centerRefEsq>0&&centerRefDir>0)navigationDriveSetCenterRefs(centerRefEsq,centerRefDir);
  lastDriverId=navigationDriveResultId();stage=Stage::MOVE_WAIT;stageMs=now;return;
 }
 case Stage::MOVE_WAIT:
  if(navigationDriveResultId()==lastDriverId)return;
  if(!navigationDriveResultOk()){fail(navigationDriveResultReason());return;}
  if(!reversing&&navigationDriveCenterRefMm()>0)centerRef=navigationDriveCenterRefMm();
  if(!reversing&&navigationDriveCenterRefEsqMm()>0&&navigationDriveCenterRefDirMm()>0){
   centerRefEsq=navigationDriveCenterRefEsqMm();centerRefDir=navigationDriveCenterRefDirMm();
  }
  lastMotionWasMove=true;afterSettleMs=now;stage=Stage::SETTLE;return;
 case Stage::WALL_SETTLE: {
  if(now-afterSettleMs>4000){fail("obstaculo_frontal_pose_nao_confirmada");return;}
  if(now-afterSettleMs<350||!navigationDriveReady())return;
  if(navigationDriveResultId()==lastDriverId || navigationDriveResultOk()){fail("parada_frontal_sem_resultado");return;}
  if(!navigationDriveSettledStraight(wallStoppedMm,wallStoppedSync))return;
  // Mesma tolerancia de posicao/sincronismo da chegada normal. A leitura
  // frontal so provoca a parada; nao converte progresso incerto em celula.
  if(!navWallArrivalAtTarget(wallStoppedMm,plannedMm,wallStoppedSync,
          lerIMU().yawDireitaGraus-yawStart,RETO_META_TOL_MM,
          RETO_SYNC_LIMITE_CHEGADA_MM,NAV_YAW_RETO_MAX_GRAUS)){
   fail("obstaculo_frontal_antes_da_celula_pose_incerta");return;
  }
  if(!mazeMarkTraversedFromPose(targetDirection)){fail("erro_registro_travessia");return;}
  x=targetX;y=targetY;
  if(!mazeSetPose(x,y,direction)){fail("erro_pose_apos_parada_frontal");return;}
  mazeMarkVisited(x,y);++steps;++wallArrivals;frontReplans=0;
  if(navigationDriveCenterRefMm()>0)centerRef=navigationDriveCenterRefMm();
  if(navigationDriveCenterRefEsqMm()>0&&navigationDriveCenterRefDirMm()>0){
   centerRefEsq=navigationDriveCenterRefEsqMm();centerRefDir=navigationDriveCenterRefDirMm();
  }
  // Observa as faces NOVAMENTE ja na celula confirmada; o BFS encontra a
  // alternativa ou a volta pela passagem percorrida, se habilitada.
  beginSensing();return;
 }
 case Stage::SETTLE:
  if(now-afterSettleMs>4000){fail("timeout_assentamento");return;}
  if(now-afterSettleMs<350||!navigationDriveReady())return;
  if(lastMotionWasMove){
   if(fabsf(lerIMU().yawDireitaGraus-yawStart)>NAV_YAW_RETO_MAX_GRAUS){fail("desvio_angular_apos_parada");return;}
   if(!mazeMarkTraversedFromPose(targetDirection)){fail("erro_registro_travessia");return;}
   x=targetX;y=targetY;mazeSetPose(x,y,direction);mazeMarkVisited(x,y);++steps;frontReplans=0;if(reversing)++reverseSteps;
   if(mazeIsGoal()){stage=Stage::DONE;errorText="objetivo_atingido";return;}
   beginSensing();return;
  }
  if(nudging){nudging=false;stage=Stage::TURN_START;stageMs=now;return;}
  if(aligning){aligning=false;stage=Stage::MOVE_START;stageMs=now;return;}
  // V4.9.2: confere contra o rumo ABSOLUTO esperado, nao so o giro relativo.
  if(fabsf(lerIMU().yawDireitaGraus-expectedYaw)>NAV_YAW_GIRO_TOL_GRAUS){fail("giro_IMU_fora_tolerancia");return;}
  direction=dirChar((dirValue(direction)+(turnRight?1:3))&3);mazeSetPose(x,y,direction);
  if(--turnsRemaining){stage=Stage::TURN_START;stageMs=now;return;}
  beginSensing();return;
 default:return;
 }
}
String navigationStatusJson(){
 String s="{\"estado\":\""+String(stageText())+"\",\"ativo\":"+String(navigationRunning()?"true":"false")+
  ",\"motivo\":\""+String(errorText)+"\",\"passos\":"+String(steps)+",\"x\":"+String(x)+",\"y\":"+String(y)+
  ",\"direcao\":\""+String(direction)+"\",\"proximaDirecao\":\""+String(targetDirection)+"\",\"estrategia\":\"goal\",\"retorno\":\"reverse\""+
  ",\"direcaoInicial\":\""+String(options.initialDirection)+"\""+
  ",\"poseConfiavel\":"+String(poseCertain?"true":"false")+",\"passosRe\":"+String(reverseSteps)+
  ",\"reHabilitada\":"+String(options.reverse?"true":"false")+",\"centralizacaoHabilitada\":"+String(options.center?"true":"false")+
  ",\"ajusteAberturaHabilitado\":"+String(options.opening?"true":"false")+",\"ajusteAplicado\":"+String(adjustmentApplied?"true":"false")+
  ",\"bordaEsqMm\":"+nOrNull(edgeLeft)+",\"bordaDirMm\":"+nOrNull(edgeRight)+",\"centroEstimadoMm\":"+nOrNull(openingEstimate)+
  ",\"metaDeslocamentoMm\":"+String(plannedMm,1)+",\"referenciaLateralMm\":"+nOrNull(centerRef>0?centerRef:NAN)+
  ",\"referenciaEsqAprendidaMm\":"+nOrNull(centerRefEsq>0?centerRefEsq:NAN)+
  ",\"referenciaDirAprendidaMm\":"+nOrNull(centerRefDir>0?centerRefDir:NAN)+
  ",\"paradasFrontaisNaCelula\":"+String(wallArrivals)+
  ",\"posicaoParadaFrontalMm\":"+nOrNull(wallStoppedMm)+",\"syncParadaFrontalMm\":"+nOrNull(wallStoppedSync)+
  ",\"limiteFrontalMm\":"+String(dangerLimitMm(),1)+",\"autonomiaLiberada\":"+String(NAV_AUTONOMIA_HABILITADA?"true":"false")+
  ",\"lateraisCalibrados\":"+String(NAV_LATERAIS_CALIBRADOS?"true":"false")+",\"laserU3AteNarizMm\":"+String(NAV_U3_ATE_NARIZ_MM,1)+
  ",\"modoSemAlvoHabilitado\":"+String(NAV_SEM_ALVO_COMO_PASSAGEM?"true":"false")+",\"semAlvoU3Seguidas\":"+String(sensorHistorico[LASER_U3].semAlvoSeguidas())+
  ",\"celulasComFaceInferida\":"+String(inferredFaces)+
  ",\"rumoEsperadoGraus\":"+String(expectedYaw,1)+",\"erroRumoGraus\":"+String(expectedYaw-lerIMU().yawDireitaGraus,1)+
  ",\"alvoUltimoGiroGraus\":"+nOrNull(lastTurnTarget)+",\"alinhamentos\":"+String(alignCount)+
  ",\"ultimoAlinhamentoGraus\":"+nOrNull(lastAlignDeg)+
  ",\"desvioLateralMm\":"+nOrNull(lateralOffset)+",\"correcoesDeDesvio\":"+String(steerCount)+
  ",\"ultimaInclinacaoCorrecaoGraus\":"+nOrNull(lastSteerDeg)+
  ",\"meiaLarguraAprendidaMm\":"+nOrNull(learnedHalf>0?learnedHalf:NAN)+
  ",\"larguraCorredorUsadaMm\":"+nOrNull(openingWidth()>0?openingWidth():NAN)+
  ",\"larguraCorredorAprendida\":"+String(learnedWidth>0?"true":"false")+",\"recuosAntesDoGiro\":"+String(backoffCount)+
  ",\"u3CodigoAvanco\":"+String(frontMove.raw)+",\"u3RangeStatusAvanco\":"+String(frontMove.range)+
  ",\"u3IdadeEventoAvancoMs\":"+String(frontMove.eventAge)+",\"u3IdadeValidaAvancoMs\":"+String(frontMove.validAge)+
  ",\"u3ProgressoAvancoMm\":"+nOrNull(frontMove.progress)+",\"u3FolgaEstimadaMm\":"+nOrNull(frontMove.remaining)+
  ",\"u3DecisaoAvanco\":\""+String(frontMove.decision)+"\""+
  ",\"referenciaLateralUltimaMm\":"+nOrNull(navigationDriveCenterRefMm()>0?navigationDriveCenterRefMm():NAN)+"}";
 return s;
}
