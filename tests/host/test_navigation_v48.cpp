// V4.9: labirinto ficticio, sensores e movimento totalmente simulados.
#include "navigation.h"
#include "maze_state.h"
#include "lasers.h"
#include "imu.h"
#include "web_telemetry.h"
#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <string>
#include <math.h>
using namespace mazeplan;
static uint32_t nowMs=1000;
uint32_t millis(){return nowMs;}
FakeSerial Serial;
static Grid truth;
static uint32_t resultId=0, startDriveMs=0;
static bool moving=false,turning=false,rightTurn=true,resultOk=true;
static int moves=0,turns=0,reverses=0,adjustments=0;
static uint8_t px=0,py=0;
static Direction heading=NORTH;
static float yaw=0,targetMm=0,progress=0;
static bool weakFront=false,missingFront=false,forceMotionFail=false,timeoutFront=false,leftLiesOpen=false;
static bool trackingCase=false,openNoTarget=false;
static std::string status(){return navigationStatusJson().c_str();}
static bool has(const char* s){return status().find(s)!=std::string::npos;}
static Direction sensorDir(uint8_t s){return s==LASER_U2?Grid::left(heading):s==LASER_U4?Grid::right(heading):heading;}
LeituraLaser lerLaser(uint8_t sensor){
 LeituraLaser l;l.inicializado=!(missingFront&&sensor==LASER_U3);l.total=nowMs/60;
 l.ultimoEventoMs=(nowMs/60)*60;l.ultimaValidaMs=l.ultimoEventoMs;
 auto d=sensorDir(sensor);bool present=!truth.open(px,py,d);
 if(moving&&!turning&&sensor==LASER_U3&&targetMm>0)present=false;
 // Modelo controlado da abertura na bifurcacao de retorno, com leitura VALIDA.
 if(trackingCase&&moving&&!turning&&targetMm<0&&px==0&&py==2&&sensor==LASER_U4&&fabsf(progress)>=90)present=false;
 l.brutoMm=present?60:300;l.filtradoMm=l.brutoMm;l.statusVL=0;
 l.parede=present?PAREDE_PRESENTE:PAREDE_AUSENTE;
 // Corredor aberto real: o VL53L0X nao ve alvo e devolve sinal fraco.
 if(!present&&!moving&&sensor!=LASER_U3&&openNoTarget){l.brutoMm=LASER_SINAL_FRACO;l.statusVL=2;l.parede=PAREDE_INDEFINIDA;}
 if(weakFront&&sensor==LASER_U3){l.brutoMm=LASER_SINAL_FRACO;l.statusVL=2;l.parede=PAREDE_INDEFINIDA;}
 if(timeoutFront&&sensor==LASER_U3){l.brutoMm=LASER_TIMEOUT;l.statusVL=255;l.parede=PAREDE_INDEFINIDA;l.ultimaValidaMs=0;}
 // Sensor esquerdo "mente" que a borda externa esta aberta.
 if(leftLiesOpen&&sensor==LASER_U2&&px==0&&heading==NORTH){l.brutoMm=300;l.parede=PAREDE_AUSENTE;}
 return l;
}
bool batteryAvailable(){return true;}
bool bateriaBaixa(){return false;}
bool bateriaCritica(){return false;}
DadosIMU lerIMU(){DadosIMU d;d.confiavel=true;d.yawDireitaGraus=yaw;return d;}
bool navigationDriveReady(){return !moving;}
bool navigationDriveStartStraight(float mm,float rpm){
 assert(fabsf(mm)==NAV_CELULA_MM&&rpm>0);
 const auto d=mm>0?heading:Grid::opposite(heading);assert(truth.open(px,py,d));
 moving=true;turning=false;startDriveMs=nowMs;targetMm=mm;progress=0;++moves;if(mm<0)++reverses;return true;
}
float navigationDriveProgressMm(){return progress;}
float navigationDriveCenterRefMm(){return 38;}
void navigationDriveSetCenterRef(float mm){assert(mm>0);}
bool navigationDriveAdjustStraight(float mm){assert(moving&&!turning&&fabsf(mm-targetMm)<=NAV_AJUSTE_MAX_MM);targetMm=mm;++adjustments;return true;}
bool navigationDriveStartTurn(bool right,float rpm){assert(rpm>0);moving=true;turning=true;rightTurn=right;startDriveMs=nowMs;++turns;return true;}
uint32_t navigationDriveResultId(){return resultId;}
bool navigationDriveResultOk(){return resultOk;}
const char* navigationDriveResultReason(){return "simulacao_falhou";}
bool navigationDriveIsActive(){return moving;}
void pararTesteDeEmergencia(const char* why){moving=false;navigationNotifyStop(why);}
static void tick(){
 if(moving){
  if(forceMotionFail&&nowMs-startDriveMs>200){moving=false;resultOk=false;++resultId;}
  else if(turning&&nowMs-startDriveMs>350){heading=rightTurn?Grid::right(heading):Grid::left(heading);yaw+=rightTurn?90:-90;moving=false;++resultId;}
  else if(!turning){
   const float p=0.14f*(nowMs-startDriveMs);progress=targetMm>0?p:-p;
   if(p>=fabsf(targetMm)){const auto d=targetMm>0?heading:Grid::opposite(heading);px+=Grid::dx(d);py+=Grid::dy(d);moving=false;++resultId;}
  }
 }
 navigationTick();nowMs+=20;
}
static void reset(uint8_t w,uint8_t h,uint8_t gx,uint8_t gy){
 assert(truth.init(w,h,gx,gy));moves=turns=reverses=adjustments=0;moving=turning=false;resultOk=true;weakFront=missingFront=forceMotionFail=trackingCase=timeoutFront=leftLiesOpen=openNoTarget=false;px=py=0;heading=NORTH;yaw=0;nowMs+=10000;
}
static void run(){for(int i=0;i<30000&&navigationRunning();++i)tick();assert(!navigationRunning());}
static void expectGoal(){if(!has("OBJETIVO_ATINGIDO"))fprintf(stderr,"Unexpected nav status: %s\n",status().c_str());assert(has("OBJETIVO_ATINGIDO"));assert(px==truth.goalX&&py==truth.goalY);}
int main(){
 reset(2,2,0,1);assert(truth.observe(0,0,NORTH,false));assert(!navigationStart(2,2,0,1));run();expectGoal();assert(moves==1&&turns==0);puts("PASS: direto uma celula");
 reset(2,2,1,0);assert(truth.observe(0,0,EAST,false));assert(!navigationStart(2,2,1,0));run();expectGoal();assert(moves==1&&turns==1);puts("PASS: giro e avanco");
 reset(2,2,1,0);heading=EAST;assert(truth.observe(0,0,EAST,false));auto east=navigationDefaultOptions();east.initialDirection='L';assert(!navigationStartWithOptions(2,2,1,0,east));assert(has("\"direcaoInicial\":\"L\""));run();expectGoal();assert(moves==1&&turns==0);puts("PASS: orientacao inicial Leste sem giro desnecessario");
 reset(2,2,0,1);auto badDirection=navigationDefaultOptions();badDirection.initialDirection='X';assert(navigationStartWithOptions(2,2,0,1,badDirection));assert(!navigationRunning());puts("PASS: orientacao inicial invalida rejeitada");
 reset(8,4,7,3);trackingCase=true;
 for(int y=0;y<3;++y)assert(truth.observe(0,y,NORTH,false)); // beco sem saida em (0,3)
 for(int x=0;x<7;++x)assert(truth.observe(x,1,EAST,false)); // bifurcacao em (0,1)
 for(int y=1;y<3;++y)assert(truth.observe(7,y,NORTH,false));
 assert(!navigationStart(8,4,7,3));run();expectGoal();assert(reverses==2&&turns==2&&moves==14);assert(adjustments>=1);puts("PASS: 8x4, beco, dois recuos, abertura e giro na bifurcacao, canto oposto");
 reset(2,2,1,0);assert(truth.observe(0,0,EAST,false));weakFront=true;assert(!navigationStart(2,2,1,0)); // start does not require distance
 for(int i=0;i<130;++i){tick();}weakFront=false;run();expectGoal();puts("PASS: sem alvo frontal nao impede observar e escolher alternativa conhecida");
 // V4.9.1: corredor sem retorno optico (sinal fraco repetido) e passagem.
 reset(2,2,0,1);assert(truth.observe(0,0,NORTH,false));weakFront=true;assert(!navigationStart(2,2,0,1));run();expectGoal();puts("PASS: sem alvo a frente repetido = passagem livre");
 // Timeout/erro do sensor NUNCA libera passagem: a face vira parede por precaucao.
 reset(2,2,0,1);assert(truth.observe(0,0,NORTH,false));timeoutFront=true;assert(!navigationStart(2,2,0,1));run();assert(moves==0&&has("FALHA")&&has("sem_passagem"));assert(has("\"celulasComFaceInferida\":1"));puts("PASS: timeout frontal nao libera movimento e nao trava esperando");
 // Laterais em corredor aberto devolvem sinal fraco: antes o robo travava.
 reset(2,2,1,0);assert(truth.observe(0,0,EAST,false));openNoTarget=true;assert(!navigationStart(2,2,1,0));run();expectGoal();assert(turns==1);puts("PASS: lateral aberta sem retorno optico e reconhecida");
 // Leitura que contradiz a borda externa nao aborta mais a missao.
 reset(2,2,0,1);assert(truth.observe(0,0,NORTH,false));leftLiesOpen=true;assert(!navigationStart(2,2,0,1));run();expectGoal();puts("PASS: leitura contraditoria na borda e ignorada");
 // Inicio fora do canto (0,0)/N.
 {reset(3,3,0,0);px=2;py=2;heading=SOUTH;
  assert(truth.observe(2,2,SOUTH,false));assert(truth.observe(2,1,WEST,false));assert(truth.observe(1,1,SOUTH,false));assert(truth.observe(1,0,WEST,false));
  auto o=navigationDefaultOptions();o.startX=2;o.startY=2;o.initialDirection='S';
  assert(!navigationStartWithOptions(3,3,0,0,o));run();expectGoal();puts("PASS: inicio em (2,2) voltado S chega em (0,0)");
  reset(3,3,0,0);o.startX=3;assert(navigationStartWithOptions(3,3,0,0,o));o.startX=0;o.startY=0;assert(navigationStartWithOptions(3,3,0,0,o));o.startY=1;o.initialDirection='X';assert(navigationStartWithOptions(3,3,0,0,o));puts("PASS: inicio invalido e recusado");}
 reset(2,2,0,1);auto invalid=navigationDefaultOptions();invalid.corridorWidth=0;assert(navigationStartWithOptions(2,2,0,1,invalid));puts("PASS: ajuste exige geometria valida");
 reset(2,2,0,1);missingFront=true;assert(navigationStart(2,2,0,1));puts("PASS: sensor nao inicializado bloqueia inicio");
 reset(2,2,0,1);assert(truth.observe(0,0,NORTH,false));forceMotionFail=true;assert(!navigationStart(2,2,0,1));run();assert(has("FALHA")&&has("\"poseConfiavel\":false"));assert(std::string(mazeEstadoJson().c_str()).find("\"pose\":null")!=std::string::npos);puts("PASS: falha no deslocamento invalida pose");
}
