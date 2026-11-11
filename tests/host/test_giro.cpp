// Giro no proprio eixo com rodas ideais (sem atrito nem inercia) e uma IMU
// simulada a partir dos encoders. Confere a logica do controlador, nao a fisica.
#include "motion_control.h"
#include "motors.h"
#include "encoders.h"
#include "imu.h"
#include "config.h"
#include <iostream>
#include <cmath>
#include <string>
static uint32_t t=1000;
uint32_t millis(){return t;}
static DadosEncoder encoder{0,0,0,0};
static int pwmLeft=0,pwmRight=0;static bool fLeft=true,fRight=true;
static const float CPR_E=815.55f, CPR_D=815.48f;
static int64_t refE=0, refD=0;
static bool imuOk=true;
static float yawSim(){
 const float circ=3.14159265f*DIAMETRO_RODA_MM;
 const float dE=(encoder.esq-refE)*circ/CPR_E, dD=(encoder.dir-refD)*circ/CPR_D;
 return (dE-dD)/ENTRE_RODAS_MM*57.2957795f; // direita positivo
}
DadosEncoder lerEncoders(){return encoder;}
DadosIMU lerIMU(){DadosIMU d;d.presente=d.calibrada=true;d.confiavel=imuOk;d.yawDireitaGraus=yawSim();d.ultimaLeituraMs=t;return d;}
bool zerarYawIMU(){if(!imuOk)return false;refE=encoder.esq;refD=encoder.dir;return true;}
void setMotorEsq(int pwm,bool frente){pwmLeft=pwm;fLeft=frente;}
void setMotorDir(int pwm,bool frente){pwmRight=pwm;fRight=frente;}
void frearMotorEsq(){pwmLeft=0;}
void frearMotorDir(){pwmRight=0;}
static void passo(){
 float rpmE=(float)pwmLeft*0.55f*(fLeft?1:-1),rpmD=(float)pwmRight*0.55f*(fRight?1:-1);
 encoder.cpsEsq=rpmE*CPR_E/60.0f;encoder.cpsDir=rpmD*CPR_D/60.0f;
 encoder.esq+=(int64_t)std::round(encoder.cpsEsq*0.02f);
 encoder.dir+=(int64_t)std::round(encoder.cpsDir*0.02f);
 t+=20;
}
static std::string rodar(){
 const char* r=nullptr;
 for(int i=0;i<1500&&!r;i++){passo();r=atualizarControleMovimento();}
 pwmLeft=pwmRight=0;encoder.cpsEsq=encoder.cpsDir=0;desativarControleMovimento();
 return r?r:"timeout";
}
static int falhas=0;
static void confere(bool ok,const char* nome){std::cout<<(ok?"ok   ":"FALHA ")<<nome<<"\n";if(!ok)++falhas;}
int main(){
 // Giro de 90 graus: encoder e IMU, nos dois sentidos.
 for(int direita=1;direita>=0;direita--)for(int imu=0;imu<2;imu++){
  zerarYawIMU();
  const bool iniciou=iniciarControleGiro(direita==1,imu==1,40,CPR_E,CPR_D,true);
  const std::string r=iniciou?rodar():"nao_iniciou";
  const float alvo=direita?90.0f:-90.0f;
  std::cout<<"90 "<<(direita?"direita ":"esquerda ")<<(imu?"imu ":"encoder ")<<r<<" yaw="<<yawSim()<<"\n";
  confere(r==(imu?"giro_imu_concluido":"giro_encoder_concluido"),"giro de 90 termina normalmente");
  confere(std::fabs(yawSim()-alvo)<=2.0f,"giro de 90 fica a ate 2 graus do alvo");
 }
 // Correcao curta pela IMU, sem zerar a referencia de yaw.
 const float correcoes[]={2.0f,-1.2f,5.0f,-8.0f};
 for(float angulo:correcoes){
  zerarYawIMU();
  const float antes=yawSim();
  const bool iniciou=iniciarControleGiroAngulo(angulo,true,20,CPR_E,CPR_D,true,false,0.4f);
  const std::string r=iniciou?rodar():"nao_iniciou";
  const float girou=yawSim()-antes;
  std::cout<<"correcao "<<angulo<<" -> "<<r<<" girou="<<girou<<"\n";
  confere(r=="giro_imu_concluido","correcao termina normalmente");
  confere(girou*angulo>0,"correcao gira para o lado pedido");
  confere(std::fabs(girou-angulo)<=1.0f,"correcao fica a ate 1 grau do pedido");
 }
 // zerarYaw=false nao mexe na referencia: o yaw acumulado continua valendo.
 zerarYawIMU();
 iniciarControleGiroAngulo(5.0f,true,20,CPR_E,CPR_D,true,false,0.4f);rodar();
 const float acumulado=yawSim();
 iniciarControleGiroAngulo(-5.0f,true,20,CPR_E,CPR_D,true,false,0.4f);rodar();
 std::cout<<"ida "<<acumulado<<" volta "<<yawSim()<<"\n";
 confere(std::fabs(yawSim())<=1.5f,"ida e volta sem zerar o yaw retorna perto de zero");
 // Recusas.
 confere(!iniciarControleGiroAngulo(0.2f,true,20,CPR_E,CPR_D,true,false,0.4f),"recusa angulo menor que o minimo");
 confere(!iniciarControleGiroAngulo(200.0f,true,20,CPR_E,CPR_D,true,false,0.4f),"recusa angulo maior que 180");
 confere(!iniciarControleGiroAngulo(NAN,true,20,CPR_E,CPR_D,true,false,0.4f),"recusa angulo invalido");
 confere(!iniciarControleGiroAngulo(2.0f,true,10,CPR_E,CPR_D,true,false,0.4f),"recusa RPM abaixo do minimo");
 imuOk=false;
 confere(!iniciarControleGiroAngulo(2.0f,true,20,CPR_E,CPR_D,true,false,0.4f),"recusa giro por IMU sem IMU confiavel");
 confere(!iniciarControleGiro(true,true,40,CPR_E,CPR_D,true),"recusa giro de 90 por IMU sem IMU confiavel");
 confere(iniciarControleGiroAngulo(2.0f,false,20,CPR_E,CPR_D,true,false,0.4f),"giro por encoder nao depende da IMU");
 desativarControleMovimento();
 if(falhas){std::cout<<falhas<<" verificacoes falharam\n";return 1;}
 std::cout<<"PASS: giro de 90 graus e correcao de angulo em simulacao ideal\n";
}
