#include "maze_state.h"
#include <string.h>
namespace {
struct Celula { uint8_t conhecidas=0, paredes=0, visitada=0; };
static Celula mapa[16][16]; // 768 bytes para os estados por celula.
static bool poseConhecida=false;
static uint8_t posX=0,posY=0;
static char orientacao='N';
static uint32_t revisao=0;
static bool dentro(uint8_t x,uint8_t y) { return x<16 && y<16; }
}
void mazeReset(){ memset(mapa,0,sizeof(mapa));poseConhecida=false;posX=posY=0;orientacao='N';++revisao; }
bool mazeSetPose(uint8_t x,uint8_t y,char d){ if(!dentro(x,y)||(d!='N'&&d!='S'&&d!='L'&&d!='O'))return false;posX=x;posY=y;orientacao=d;poseConhecida=true;++revisao;return true; }
bool mazeObserve(uint8_t x,uint8_t y,uint8_t conhecidas,uint8_t presentes){
  if(!dentro(x,y)||((presentes & (uint8_t)~conhecidas)!=0) || (conhecidas&0xF0))return false;
  Celula &c=mapa[y][x]; // so atualiza bits conhecidos; nunca trata desconhecido como livre
  c.paredes=(uint8_t)((c.paredes & (uint8_t)~conhecidas)|(presentes & conhecidas));
  c.conhecidas|=conhecidas;++revisao;return true;
}
bool mazeMarkVisited(uint8_t x,uint8_t y){if(!dentro(x,y))return false;mapa[y][x].visitada=1;++revisao;return true;}
String mazeEstadoJson(){
  String s; s.reserve(4300);
  s="{\"largura\":16,\"altura\":16,\"revisao\":"+String(revisao)+",\"pose\":";
  if(poseConhecida){s+="{\"x\":"+String(posX)+",\"y\":"+String(posY)+",\"direcao\":\""+String(orientacao)+"\"}";}
  else s+="null";
  s+=",\"celulas\":[";
  // campos = [mascara_conhecidas,mascara_paredes,visitada], em linha y,x
  for(uint8_t y=0;y<16;++y){if(y)s+=',';s+='[';for(uint8_t x=0;x<16;++x){
     if(x) s+=',';
     const auto &c=mapa[y][x];
     s+='[';s+=String(c.conhecidas);s+=',';s+=String(c.paredes);s+=',';s+=String(c.visitada);s+=']';
  }s+=']';}s+="]}";return s;
}
