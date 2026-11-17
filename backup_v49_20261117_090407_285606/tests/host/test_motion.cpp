#include "motion_control.h"
#include "motors.h"
#include "encoders.h"
#include "imu.h"
#include <iostream>
#include <cmath>
#include <cstdlib>
static uint32_t t=1000;
uint32_t millis(){return t;}
static DadosEncoder encoder{0,0,0,0};
static int pwmLeft=0,pwmRight=0;static bool fLeft=true,fRight=true;
DadosEncoder lerEncoders(){return encoder;}
DadosIMU lerIMU(){DadosIMU d;return d;}
bool zerarYawIMU(){return true;}
void setMotorEsq(int pwm,bool frente){pwmLeft=pwm;fLeft=frente;}
void setMotorDir(int pwm,bool frente){pwmRight=pwm;fRight=frente;}
void frearMotorEsq(){pwmLeft=0;}
void frearMotorDir(){pwmRight=0;}
static void updateWheel(){
 // idealized immediate speed as a coarse bench simulation, not physical validation
 float rpmE=(float)pwmLeft*0.55f*(fLeft?1:-1),rpmD=(float)pwmRight*0.55f*(fRight?1:-1);
 encoder.cpsEsq=rpmE*815.55f/60.0f;encoder.cpsDir=rpmD*815.48f/60.0f;
 encoder.esq+=(int64_t)std::round(encoder.cpsEsq*0.02f);
 encoder.dir+=(int64_t)std::round(encoder.cpsDir*0.02f);
 t+=20;
}
int main(){
 if(!iniciarControleReto(100,30,815.55f,815.48f,true))return 1;
 const char* result=nullptr;
 for(int i=0;i<1200;i++){
  updateWheel(); result=atualizarControleMovimento();
  if(result)break;
 }
 std::cout<<"100mm -> final counts: E="<<encoder.esq<<" D="<<encoder.dir<<" result="<<(result?result:"timeout")<<"\n";
 if(!result) return 2;
 if(std::string(result)!="reto_meta_atingida")return 3;
 desativarControleMovimento();
 if(!iniciarControleReto(-100,30,815.55f,815.48f,true))return 4;
 result=nullptr;
 for(int i=0;i<1200;i++){
  updateWheel(); result=atualizarControleMovimento();
  if(result)break;
 }
 std::cout<<"-100mm -> final counts: E="<<encoder.esq<<" D="<<encoder.dir<<" result="<<(result?result:"timeout")<<"\n";
 if(!result)return 5;
 if(std::string(result)!="reto_meta_atingida")return 6;
 std::cout<<"PASS: motor control v4.5 idealized forward+reverse simulation\n";
}
