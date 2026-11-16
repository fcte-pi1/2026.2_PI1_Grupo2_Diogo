// Teste native V4.8: computador SEM hardware, simulando sensores e motor.
#include "navigation.h"
#include "maze_state.h"
#include "lasers.h"
#include "imu.h"
#include "web_telemetry.h"
#include <assert.h>
#include <stdio.h>
#include <string>
static uint32_t nowMs=1000;
uint32_t millis(){return nowMs;}
FakeSerial Serial;
static uint32_t resultId=0;
static bool moving=false,resultOk=true;
static uint32_t startDriveMs=0;
static int moves=0,turns=0;
static bool laserFail=false;
static bool scenarioEast=false;
LeituraLaser lerLaser(uint8_t sensor){
 LeituraLaser l;l.inicializado=true;l.total=nowMs/60;
 l.ultimaValidaMs=nowMs-10;
 uint8_t x=0,y=0;char h='N';mazeGetPose(x,y,h);
 bool present=false;
 if(!scenarioEast) {
   // inicio (0,0) rumo norte aberto, alvo (0,1) e paredes laterais.
   present = sensor!=LASER_U3 || y==1;
 } else {
   // rumo N no inicio: N=parede, D=passagem Leste, E=parede;
   // apos girar para Leste, frente=aberta, dois lados=parede.
   if(x==1)present=true;
   else if(h=='N') present= sensor!=LASER_U4;
   else present= sensor!=LASER_U3;
 }
 l.brutoMm=present?60:300;
 l.parede=present?PAREDE_PRESENTE:PAREDE_AUSENTE;
 // Durante movimento, mantem parede futura distante, como no centro da celula de partida.
 if(moving && sensor==LASER_U3){l.brutoMm=300;l.parede=PAREDE_AUSENTE;}
 if(laserFail && sensor==LASER_U3){l.brutoMm=-1;l.parede=PAREDE_INDEFINIDA;}
 return l;
}
bool batteryAvailable(){return true;}
bool bateriaBaixa(){return false;}
bool bateriaCritica(){return false;}
DadosIMU lerIMU(){DadosIMU d;d.confiavel=true;return d;}
bool navigationDriveReady(){return !moving;}
bool navigationDriveStartStraight(float mm,float rpm){assert(mm==180&&rpm>0);moving=true;startDriveMs=nowMs;++moves;return true;}
bool navigationDriveStartTurn(bool right,float rpm){assert(right&&rpm>0);moving=true;startDriveMs=nowMs;++turns;return true;}
uint32_t navigationDriveResultId(){return resultId;}
bool navigationDriveResultOk(){return resultOk;}
const char* navigationDriveResultReason(){return "simulacao_falhou";}
bool navigationDriveIsActive(){return moving;}
void pararTesteDeEmergencia(const char* why){moving=false;navigationNotifyStop(why);}
void tick(){
 if(moving&&nowMs-startDriveMs>350){moving=false;++resultId;}
 navigationTick();nowMs+=20;
}
void mission(uint8_t gx,uint8_t gy,int expectedMoves,int expectedTurns){
 moves=0;turns=0;laserFail=false;
 const char* err=navigationStart(2,2,gx,gy);assert(err==nullptr);
 for(int i=0;i<600 && navigationRunning();++i)tick();
 String status=navigationStatusJson();
 if(std::string(status.c_str()).find("OBJETIVO_ATINGIDO")==std::string::npos)
  fprintf(stderr,"Unexpected nav status: %s\n",status.c_str());
 assert(std::string(status.c_str()).find("OBJETIVO_ATINGIDO")!=std::string::npos);
 assert(moves==expectedMoves&&turns==expectedTurns);
 printf("PASS: objetivo (%u,%u), %d movimento(s), %d giro(s)\n",gx,gy,moves,turns);
}
int main(){
 scenarioEast=false;mission(0,1,1,0);
 scenarioEast=true;mission(1,0,1,1);
 laserFail=true;const char* err=navigationStart(2,2,0,1);assert(err!=nullptr);
 printf("PASS: falha de leitura frontal bloqueia inicio\n");
}
