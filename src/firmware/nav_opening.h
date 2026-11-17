#pragma once
#include <stdint.h>
#include <math.h>
// p: percurso positivo no sentido da marcha, inclusive quando de re.
// Centro = p_borda + largura livre/2 + projecao do laser no sentido da marcha.
// Duas amostras de parede e duas de abertura; estima a borda entre a ultima
// parede valida e a primeira amostra aberta. Nao usa eventos inconclusivos como abertura.
class NavOpening {
public:
 void reset(uint32_t last=0){*this=NavOpening{};lastId=last;}
 bool update(uint32_t id, bool valid, bool wall, bool clear, float p){
   if(id==lastId || !isfinite(p))return false;
   lastId=id;
   if(!valid){wallCount=0;clearCount=0;armed=false;ambiguous=0;return false;}
   if(!wall&&!clear){wallCount=0;clearCount=0;if(++ambiguous>3)armed=false;return false;}
   ambiguous=0;
   if(wall){lastWall=p;if(wallCount<2)++wallCount;if(wallCount>=2)armed=true;clearCount=0;return false;}
   wallCount=0;
   if(!armed || detected)return false;
   if(!clearCount){if(p-lastWall>24||p<lastWall){armed=false;return false;}edge=(lastWall+p)*0.5f;}
   if(++clearCount<2)return false;
   detected=true;return true;
 }
 bool found() const{return detected;}
 float edgeMm() const{return detected?edge:NAN;}
 float centerMm(float width,float sensorAlongTravel) const{
   if(!detected || !isfinite(width) || !isfinite(sensorAlongTravel) || width<=0)return NAN;
   return edge+0.5f*width+sensorAlongTravel;
 }
private:
 uint32_t lastId=0;uint8_t wallCount=0,clearCount=0,ambiguous=0;bool armed=false,detected=false;float edge=0,lastWall=0;
};
inline bool navOpeningAdjustmentOk(float center,float nominal,float progress,float maxTrim,float minRemaining){
 return isfinite(center)&&isfinite(nominal)&&isfinite(progress)&&fabsf(center-nominal)<=maxTrim&&center-progress>=minRemaining&&nominal-progress>=minRemaining;
}
