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
static bool trackingCase=false,openNoTarget=false,edgeNoTarget=false;static float lateralBias=0;
static int frontCase=0; // 1: perda breve; 2: distancia em reducao; 3: I2C; 4: obstaculo; 5: evento velho; 6: sigma
static bool usedMemory=false;
static int frontStoppedRead=60;static float lastNudgeMm=0;
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
 l.brutoMm=present?60:300;
 if(present&&!moving&&sensor==LASER_U3)l.brutoMm=frontStoppedRead; // U3 parado com parede a frente
 // Robo deslocado para a direita: esquerda le mais, direita le menos.
 if(present&&sensor==LASER_U2)l.brutoMm+=(int)lateralBias;
 if(present&&sensor==LASER_U4)l.brutoMm-=(int)lateralBias;
 l.filtradoMm=l.brutoMm;l.statusVL=0;
 l.parede=present?PAREDE_PRESENTE:PAREDE_AUSENTE;
 // Abertura lateral real durante a re: sem retorno optico.
 if(edgeNoTarget&&moving&&!turning&&!present&&sensor!=LASER_U3){l.brutoMm=LASER_SINAL_FRACO;l.statusVL=2;l.parede=PAREDE_INDEFINIDA;l.ultimaValidaMs=0;}
 // Corredor aberto real: o VL53L0X nao ve alvo e devolve sinal fraco.
 if(!present&&!moving&&sensor!=LASER_U3&&openNoTarget){l.brutoMm=LASER_SINAL_FRACO;l.statusVL=2;l.parede=PAREDE_INDEFINIDA;}
 if(weakFront&&sensor==LASER_U3){l.brutoMm=LASER_SINAL_FRACO;l.statusVL=2;l.parede=PAREDE_INDEFINIDA;}
 if(timeoutFront&&sensor==LASER_U3){l.brutoMm=LASER_TIMEOUT;l.statusVL=255;l.parede=PAREDE_INDEFINIDA;l.ultimaValidaMs=0;}
 // Sensor esquerdo "mente" que a borda externa esta aberta.
 if(leftLiesOpen&&sensor==LASER_U2&&px==0&&heading==NORTH){l.brutoMm=300;l.parede=PAREDE_AUSENTE;}
 if(moving&&!turning&&sensor==LASER_U3&&targetMm>0){
  const uint32_t dt=nowMs-startDriveMs;
  if(frontCase==2 && dt<240){l.brutoMm=165;l.filtradoMm=165;l.statusVL=0;l.parede=PAREDE_AUSENTE;}
  if(dt>=240){
   if((frontCase==1&&dt<300)||frontCase==2){l.brutoMm=LASER_SINAL_FRACO;l.statusVL=2;l.parede=PAREDE_INDEFINIDA;}
   if(frontCase==3){l.brutoMm=LASER_ERRO_COMUNICACAO;l.statusVL=255;l.parede=PAREDE_INDEFINIDA;}
   if(frontCase==4){l.brutoMm=20;l.statusVL=0;l.parede=PAREDE_PRESENTE;}
   if(frontCase==5){l.ultimoEventoMs=nowMs-NAV_IDADE_SENSOR_MS-1;}
   // Parede no fim do corredor; simulacao do freio termina no centro.
   if(frontCase==7&&px==0&&py==2&&heading==NORTH&&progress>=175){l.brutoMm=35;l.statusVL=0;l.parede=PAREDE_PRESENTE;}
   if(frontCase==6){l.brutoMm=LASER_SINAL_FRACO;l.statusVL=1;l.parede=PAREDE_INDEFINIDA;}
  }
 }
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
 moving=true;turning=false;resultOk=true;startDriveMs=nowMs;targetMm=mm;progress=0;++moves;if(mm<0)++reverses;return true;
}
float navigationDriveProgressMm(){return progress;}
float navigationDriveCenterRefMm(){return 38;}
void navigationDriveSetCenterRef(float mm){assert(mm>0);}
float navigationDriveCenterRefEsqMm(){return 38;}
float navigationDriveCenterRefDirMm(){return 42;}
void navigationDriveSetCenterRefs(float e,float d){assert(e==38&&d==42);}
bool navigationDriveStopAtWall(){
 assert(moving&&!turning&&targetMm>0);
 moving=false;resultOk=false;++resultId;
 if(frontCase==7){ // modelo idealizado: parada final no alvo
  assert(progress>=175);progress=targetMm;
  px+=Grid::dx(heading);py+=Grid::dy(heading);
 }
 return true;
}
bool navigationDriveSettledStraight(float& p,float& sync){if(moving)return false;p=progress;sync=0;return true;}
bool navigationDriveAdjustStraight(float mm){assert(moving&&!turning&&fabsf(mm-targetMm)<=NAV_AJUSTE_MAX_MM);targetMm=mm;++adjustments;return true;}
static float turnTarget=0,driftPerMove=0,alignTotal=0,lastTurnAlvo=0;static int aligns=0,nudges=0;static bool aligningFake=false;
bool navigationDriveStartTurn(bool right,float rpm,float alvo){assert(rpm>0&&fabsf(fabsf(alvo)-90)<=15);moving=true;turning=true;aligningFake=false;resultOk=true;rightTurn=right;turnTarget=alvo;lastTurnAlvo=alvo;startDriveMs=nowMs;++turns;return true;}
bool navigationDriveStartAlign(float g){assert(!moving&&fabsf(g)<=15);moving=true;turning=true;aligningFake=true;resultOk=true;turnTarget=g;alignTotal+=g;startDriveMs=nowMs;++aligns;return true;}
// Como o driver real: fica em andamento e so incrementa o resultado ao terminar (tick).
bool navigationDriveStartNudge(float mm){assert(!moving&&fabsf(mm)>=1&&fabsf(mm)<=60);lastNudgeMm=mm;++nudges;moving=true;turning=true;aligningFake=true;turnTarget=0;resultOk=true;startDriveMs=nowMs;return true;}
uint32_t navigationDriveResultId(){return resultId;}
bool navigationDriveResultOk(){return resultOk;}
const char* navigationDriveResultReason(){return "simulacao_falhou";}
bool navigationDriveIsActive(){return moving;}
void pararTesteDeEmergencia(const char* why){moving=false;navigationNotifyStop(why);}
static void tick(){
 if(moving){
  if(forceMotionFail&&nowMs-startDriveMs>200){moving=false;resultOk=false;++resultId;}
  else if(turning&&nowMs-startDriveMs>350){
   // Giro com correcao pela IMU: termina no alvo pedido pela navegacao.
   if(!aligningFake)heading=rightTurn?Grid::right(heading):Grid::left(heading);
   yaw+=turnTarget;moving=false;++resultId;}
  else if(!turning){
   const float p=0.14f*(nowMs-startDriveMs);progress=targetMm>0?p:-p;
   if(p>=fabsf(targetMm)){const auto d=targetMm>0?heading:Grid::opposite(heading);px+=Grid::dx(d);py+=Grid::dy(d);moving=false;yaw+=driftPerMove;++resultId;}
  }
 }
 navigationTick();if(has("folga_medida_recente"))usedMemory=true;nowMs+=20;
}
static void reset(uint8_t w,uint8_t h,uint8_t gx,uint8_t gy){
 assert(truth.init(w,h,gx,gy));moves=turns=reverses=adjustments=0;moving=turning=false;resultOk=true;weakFront=missingFront=forceMotionFail=trackingCase=timeoutFront=leftLiesOpen=openNoTarget=edgeNoTarget=false;lateralBias=0;frontCase=0;frontStoppedRead=60;lastNudgeMm=0;nudges=0;usedMemory=false;px=py=0;heading=NORTH;yaw=0;nowMs+=10000;
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
 // V4.9.3: a borda e encontrada mesmo quando a abertura le "sem alvo".
 reset(8,4,7,3);trackingCase=true;edgeNoTarget=true;
 for(int y=0;y<3;++y)assert(truth.observe(0,y,NORTH,false));
 for(int x=0;x<7;++x)assert(truth.observe(x,1,EAST,false));
 for(int y=1;y<3;++y)assert(truth.observe(7,y,NORTH,false));
 assert(!navigationStart(8,4,7,3));run();expectGoal();assert(adjustments>=1);puts("PASS: re guiada pela borda com abertura sem retorno optico");
 // V4.9.3: desvio lateral medido parado vira inclinacao de saida para o centro.
 {reset(2,3,0,2);for(int y=0;y<2;++y)assert(truth.observe(0,y,NORTH,false));lateralBias=20;aligns=0;alignTotal=0;
  assert(!navigationStart(2,3,0,2));run();expectGoal();
  const float esperado=-asinf(20.0f/NAV_CELULA_MM)*57.29578f;
  assert(aligns>=1&&fabsf(alignTotal-esperado)<0.2f);assert(has("\"correcoesDeDesvio\":2"));
  puts("PASS: desvio de 20 mm para a direita corrigido saindo inclinado para a esquerda");}
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
 // V4.9.2: deriva de 4 graus em cada reta e corrigida antes da reta seguinte.
 {reset(4,2,3,0);for(int x=0;x<3;++x)assert(truth.observe(x,0,EAST,false));
  aligns=0;alignTotal=0;driftPerMove=4;auto o=navigationDefaultOptions();o.initialDirection='L';heading=EAST;
  assert(!navigationStartWithOptions(4,2,3,0,o));run();expectGoal();
  assert(aligns==2&&fabsf(alignTotal+8)<0.01f);driftPerMove=0;puts("PASS: deriva de rumo nas retas e alinhada pela IMU");}
 // Erro acumulado entra no alvo do giro seguinte (rumo absoluto).
 {reset(2,2,1,1);assert(truth.observe(0,0,NORTH,false));assert(truth.observe(0,1,EAST,false));
  driftPerMove=3;aligns=0;assert(!navigationStart(2,2,1,1));run();expectGoal();
  assert(fabsf(lastTurnAlvo-87)<0.01f);driftPerMove=0;puts("PASS: giro mira o rumo absoluto (87 graus apos 3 de deriva)");}
 reset(2,2,0,1);auto invalid=navigationDefaultOptions();invalid.corridorWidth=0;assert(navigationStartWithOptions(2,2,0,1,invalid));puts("PASS: ajuste exige geometria valida");
 reset(2,2,0,1);missingFront=true;assert(navigationStart(2,2,0,1));puts("PASS: sensor nao inicializado bloqueia inicio");
 reset(2,2,0,1);assert(truth.observe(0,0,NORTH,false));forceMotionFail=true;assert(!navigationStart(2,2,0,1));run();assert(has("FALHA")&&has("\"poseConfiavel\":false"));assert(std::string(mazeEstadoJson().c_str()).find("\"pose\":null")!=std::string::npos);puts("PASS: falha no deslocamento invalida pose");
 reset(2,2,0,1);assert(truth.observe(0,0,NORTH,false));frontCase=1;assert(!navigationStart(2,2,0,1));run();expectGoal();assert(usedMemory);puts("PASS: sinal fraco transitorio com folga medida recente");
 reset(2,2,0,1);assert(truth.observe(0,0,NORTH,false));frontCase=2;assert(!navigationStart(2,2,0,1));run();assert(usedMemory&&has("U3_sem_evidencia_durante_avanco")&&has("\"poseConfiavel\":false"));puts("PASS: sinal fraco persistente sem folga comprovada para");
 reset(2,2,0,1);assert(truth.observe(0,0,NORTH,false));frontCase=3;assert(!navigationStart(2,2,0,1));run();assert(!usedMemory&&has("\"u3CodigoAvanco\":-6"));puts("PASS: I2C para imediatamente mesmo com memoria");
 reset(2,2,0,1);assert(truth.observe(0,0,NORTH,false));frontCase=4;assert(!navigationStart(2,2,0,1));run();assert(!usedMemory&&has("obstaculo_frontal_antes_da_celula_pose_incerta")&&has("\"poseConfiavel\":false")&&reverses==0);puts("PASS: obstaculo prevalece sobre memoria");
 reset(2,2,0,1);assert(truth.observe(0,0,NORTH,false));frontCase=5;assert(!navigationStart(2,2,0,1));run();assert(!usedMemory&&has("evento_antigo_ou_indisponivel"));puts("PASS: evento antigo para imediatamente");
 reset(2,2,0,1);assert(truth.observe(0,0,NORTH,false));frontCase=6;assert(!navigationStart(2,2,0,1));run();assert(!usedMemory&&has("\"u3RangeStatusAvanco\":1"));puts("PASS: sigma invalido nao usa memoria");

 reset(8,4,7,3);frontCase=7;
 for(int y=0;y<3;++y)assert(truth.observe(0,y,NORTH,false));
 for(int xx=0;xx<7;++xx)assert(truth.observe(xx,1,EAST,false));
 for(int yy=1;yy<3;++yy)assert(truth.observe(7,yy,NORTH,false));
 assert(!navigationStart(8,4,7,3));run();expectGoal();
 assert(reverses==2&&has("\"paradasFrontaisNaCelula\":1"));
 puts("PASS: parada frontal confirmada em celula, recuo no beco e rota alternativa");


 // V4.10: parou curto com parede a frente (U3 le 100, alvo 80): anda 20 mm para frente uma unica vez.
 reset(2,2,1,0);assert(truth.observe(0,0,EAST,false));frontStoppedRead=100;assert(!navigationStart(2,2,1,0));run();expectGoal();
 assert(nudges==1&&fabsf(lastNudgeMm-20.0f)<0.01f&&turns==1);assert(has("\"ajustesCentroFrente\":1"));
 puts("PASS: parada curta com parede a frente e corrigida para o centro antes do giro");
 // Ja no centro (U3 le 80): nao mexe.
 reset(2,2,1,0);assert(truth.observe(0,0,EAST,false));frontStoppedRead=80;assert(!navigationStart(2,2,1,0));run();expectGoal();
 assert(nudges==0&&has("\"ajustesCentroFrente\":0"));puts("PASS: ja no centro, sem correcao");
 // Leitura absurda (nao e a parede desta celula): nao mexe.
 reset(2,2,1,0);assert(truth.observe(0,0,EAST,false));frontStoppedRead=200;assert(!navigationStart(2,2,1,0));run();expectGoal();
 assert(nudges==0);puts("PASS: leitura fora da faixa nao vira correcao");
}
