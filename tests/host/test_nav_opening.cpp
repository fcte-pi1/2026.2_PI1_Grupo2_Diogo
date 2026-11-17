#include "nav_opening.h"
#include <assert.h>
#include <stdio.h>
int main(){
 NavOpening e;assert(!e.update(1,true,true,false,20));assert(!e.update(1,true,true,false,21));assert(!e.update(2,true,true,false,30));
 assert(!e.update(3,false,false,true,80));assert(!e.found());
 assert(!e.update(4,true,true,false,82));assert(!e.update(5,true,true,false,85));
 assert(!e.update(6,true,false,true,90));assert(e.update(7,true,false,true,93));assert(e.edgeMm()==87.5f);
 assert(e.centerMm(168,5)==176.5f);assert(e.centerMm(168,-5)==166.5f); // reverso: projecao oposta
 assert(navOpeningAdjustmentOk(179,180,93,12,25));assert(!navOpeningAdjustmentOk(160,180,93,12,25));assert(!navOpeningAdjustmentOk(179,180,170,12,25));
 e.reset();assert(!e.update(1,false,false,true,90));assert(!e.found());
 puts("PASS: borda, amostras distintas, projecao assinada, limite de ajuste e rejeicao de leitura invalida");
}
