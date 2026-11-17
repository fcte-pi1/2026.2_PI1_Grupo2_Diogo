#include "nav_wall_arrival.h"
#include <cassert>
#include <cstdio>
int main(){
 assert(navWallArrivalAtTarget(179.7f,180,0.4f,1,0.7f,2.5f,12));
 assert(!navWallArrivalAtTarget(147,180,0,0,0.7f,2.5f,12));
 assert(!navWallArrivalAtTarget(180,180,3,0,0.7f,2.5f,12));
 assert(!navWallArrivalAtTarget(180,180,0,13,0.7f,2.5f,12));
 assert(!navWallArrivalAtTarget(NAN,180,0,0,0.7f,2.5f,12));
 assert(!navWallArrivalAtTarget(180,180,NAN,0,0.7f,2.5f,12));
 puts("PASS: parada frontal no alvo, interrupcao precoce, dessincronia, yaw e odometria invalida");
}
