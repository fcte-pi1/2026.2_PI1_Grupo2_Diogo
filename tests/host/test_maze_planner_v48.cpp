#include "../../src/firmware/maze_planner.h"
#include <assert.h>
#include <stdio.h>
using namespace mazeplan;
int main(){
 Grid g;assert(g.init(4,4,1,1));
 // Bordas nunca podem ser marcadas como livres.
 assert(!g.observe(0,0,SOUTH,false));assert(!g.open(0,0,WEST));
 assert(g.observe(0,0,NORTH,false));
 assert(g.open(0,0,NORTH));assert(g.cell[1][0].known&Grid::wallBit(SOUTH));
 assert(!g.observe(0,0,NORTH,true)); // discrepancia nao sobrescreve
 g.cell[0][0].visited=1;
 auto n=g.next(0,0,NORTH);assert(n.valid && n.dir==NORTH && n.x==0 && n.y==1);
 assert(g.observe(0,1,EAST,false));g.cell[1][0].visited=1;
 n=g.next(0,1,EAST);assert(n.valid&&n.x==1&&n.y==1);
 assert(g.atGoal(1,1));
 // Nao atravessa parede desconhecida.
 Grid h;h.init(4,4,1,1);assert(!h.next(0,0,NORTH).valid);
 // Parede reciproca
 assert(h.observe(0,0,NORTH,true)); assert(h.cell[1][0].wall&Grid::wallBit(SOUTH));
 puts("PASS maze_planner: fronteiras, reciprocidade, BFS e caminho desconhecido");
}
