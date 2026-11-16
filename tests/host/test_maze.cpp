#include "maze_state.h"
#include <iostream>
#include <string>
int main(){
 mazeReset();std::string start=mazeEstadoJson().c_str();
 if(start.find("\"pose\":null")==std::string::npos)return 1;
 if(start.find("\"largura\":4")==std::string::npos)return 2;
 if(mazeObserve(16,1,1,1))return 3;
 if(mazeObserve(1,1,1,2))return 4; // parede fora de mascaras conhecidas
 if(!mazeObserve(0,0,1,0))return 5; // norte livre
 if(mazeObserve(0,0,1,1))return 6; // proibido sobrescrever contradicao
 if(!mazeSetPose(0,0,'N'))return 7;
 if(!mazeMarkVisited(0,0))return 8;
 char direction='?';uint8_t x=99,y=99;
 if(!mazeNextDirection(direction,x,y) || direction!='N' || x!=0 || y!=1)return 9;
 if(!mazeSetPose(0,1,'N'))return 10;
 if(!mazeObserve(0,1,2,0))return 11; // leste livre
 if(!mazeNextDirection(direction,x,y) || direction!='L' || x!=1 || y!=1)return 12;
 if(!mazeSetPose(1,1,'L') || !mazeIsGoal())return 13;
 std::string end=mazeEstadoJson().c_str();
 if(end.find("\"pose\":{\"x\":1,\"y\":1,\"direcao\":\"L\"}")==std::string::npos)return 14;
 std::cout << "PASS: mapa 4x4, reciproco, planejamento seguro e objetivo\n";
}
