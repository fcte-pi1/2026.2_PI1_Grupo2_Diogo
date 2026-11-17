#include "maze_state.h"
namespace {
mazeplan::Grid mapa;
bool poseConhecida=false;
uint8_t posX=0,posY=0;
mazeplan::Direction orientacao=mazeplan::NORTH;
uint32_t revisao=0;
mazeplan::Direction parse(char d){return d=='L'?mazeplan::EAST:d=='S'?mazeplan::SOUTH:d=='O'?mazeplan::WEST:mazeplan::NORTH;}
char render(mazeplan::Direction d){return d==mazeplan::NORTH?'N':d==mazeplan::EAST?'L':d==mazeplan::SOUTH?'S':'O';}
}
void mazeReset(){mapa.init(4,4,1,1);poseConhecida=false;posX=posY=0;orientacao=mazeplan::NORTH;++revisao;}
bool mazeConfigure(uint8_t w,uint8_t h,uint8_t gx,uint8_t gy){
 if(!mapa.init(w,h,gx,gy))return false;
 poseConhecida=false;posX=posY=0;orientacao=mazeplan::NORTH;++revisao;return true;
}
bool mazeSetPose(uint8_t x,uint8_t y,char d){
 if(!mapa.inside(x,y)||(d!='N'&&d!='S'&&d!='L'&&d!='O'))return false;
 posX=x;posY=y;orientacao=parse(d);poseConhecida=true;++revisao;return true;
}
bool mazeGetPose(uint8_t& x,uint8_t& y,char& d){if(!poseConhecida)return false;x=posX;y=posY;d=render(orientacao);return true;}
bool mazeObserve(uint8_t x,uint8_t y,uint8_t conhecidas,uint8_t presentes){
 if(!mapa.inside(x,y)||(conhecidas&0xF0)|| (presentes&~conhecidas))return false;
 // Verifica em mapa TEMPORARIO para nao persistir leitura parcialmente inconsistente.
 mazeplan::Grid candidate=mapa;
 for(uint8_t i=0;i<4;i++)if(conhecidas&(1<<i)){
   if(!candidate.observe(x,y,(mazeplan::Direction)i, (presentes&(1<<i))!=0))return false;
 }
 mapa=candidate;++revisao;return true;
}
bool mazeMarkVisited(uint8_t x,uint8_t y){if(!mapa.inside(x,y))return false;mapa.cell[y][x].visited=1;++revisao;return true;}
bool mazeIsGoal(){return poseConhecida&&mapa.atGoal(posX,posY);}
bool mazeNextDirection(char& d,uint8_t& nx,uint8_t& ny){
 if(!poseConhecida)return false;
 auto r=mapa.next(posX,posY,orientacao);
 if(!r.valid)return false;
 d=render(r.dir);nx=r.x;ny=r.y;return true;
}
bool mazeOpenFromPose(char d){return poseConhecida&&mapa.open(posX,posY,parse(d));}
String mazeEstadoJson(){
 String s;s.reserve(4400);
 s="{\"largura\":"+String(mapa.width)+",\"altura\":"+String(mapa.height)+
   ",\"objetivo\":{\"x\":"+String(mapa.goalX)+",\"y\":"+String(mapa.goalY)+"},\"revisao\":"+String(revisao)+",\"pose\":";
 if(poseConhecida) s+="{\"x\":"+String(posX)+",\"y\":"+String(posY)+",\"direcao\":\""+String(render(orientacao))+"\"}";
 else s+="null";
 s+=",\"celulas\":[";
 for(uint8_t y=0;y<mapa.height;y++){
   if(y)s+=',';
   s+='[';
   for(uint8_t x=0;x<mapa.width;x++){
     if(x)s+=',';
     const auto& c=mapa.cell[y][x];
     s+='[';s+=String(c.known);s+=',';s+=String(c.wall);s+=',';s+=String(c.visited);s+=']';
   }s+=']';
 }s+="]}";return s;
}
