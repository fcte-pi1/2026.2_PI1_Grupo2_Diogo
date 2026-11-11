#include "maze_state.h"
#include <iostream>
#include <string>
int main(){
 mazeReset(); std::string start=mazeEstadoJson().c_str();
 if(start.find("\"pose\":null")==std::string::npos) return 1;
 if(start.find("\"celulas\":[[[0,0,0]")==std::string::npos) return 2;
 if(mazeObserve(16,1,1,1))return 3;
 if(mazeObserve(2,3,1,2))return 4; // cannot mark unknown wall present
 if(!mazeObserve(2,3,3,1))return 5;
 if(!mazeObserve(2,3,2,0))return 6; // preserve known north wall
 if(!mazeSetPose(2,3,'L'))return 7;
 if(!mazeMarkVisited(2,3))return 8;
 std::string after=mazeEstadoJson().c_str();
 if(after.find("\"pose\":{\"x\":2,\"y\":3,\"direcao\":\"L\"}")==std::string::npos)return 9;
 if(after.find("[3,1,1]")==std::string::npos)return 10;
 std::cout << "PASS: maze 16x16 unknown pose and wall/visit logic\n";
}
