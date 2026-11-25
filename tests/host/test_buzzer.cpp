#include "buzzer_logic.h"
#include <cassert>
#include <cstdio>
using buzzer::Effect;
using buzzer::Player;
int main(){
 Player p; p.startup();
 auto tick=[&](uint32_t t,bool l=false,bool c=false,bool m=false,bool d=false,bool r=false,bool allow=true){return p.tick(t,l,c,m,d,r,allow);};
 assert(tick(0)==523); assert(tick(99)==523); assert(tick(100)==0); assert(tick(130)==659);
 assert(tick(131,true)==880); assert(tick(132,false,true)==1800);
 assert(!p.request(Effect::Boot)); assert(tick(252,false,true)==0); assert(tick(352,false,true)==1800);
 assert(tick(472,false,true)==0); assert(tick(572,false,true)==1800); assert(tick(692,false,true)==0);
 assert(tick(5131,false,true)==0); assert(tick(5132,false,true)==1800);
 assert(tick(5133)==0); assert(tick(5200,false,false,true)==659);
 for(uint32_t t=5201;t<6200;++t) tick(t,false,false,true);
 assert(tick(6200,false,false,true,false,true)==1500); assert(tick(6350,false,false,true,false,true)==0);
 assert(tick(6700,false,false,true,false,true)==1500); assert(tick(6701,false,false,true,false,false)==0);
 assert(tick(6800,false,false,false,true)==392); assert(p.active()==Effect::Victory);
 for(uint32_t t=6801;t<13000;++t) tick(t,false,false,false,true);
 assert(p.active()==Effect::None); assert(tick(13001,false,false,false,true)==0);
 assert(p.request(Effect::Reverse)); assert(tick(13002,false,false,false,true)==1500); assert(p.isTest());
 p.cancelTest(); assert(tick(13003,false,false,false,true)==0);
 assert(p.request(Effect::Low)); assert(tick(13004,false,false,false,true)==880);
 assert(tick(13005,false,false,false,true,false,false)==0);
 // Abort does not play a mission-failed sound.
 assert(tick(13006,false,false,true)==659); assert(tick(13007)==0);
 Player wrap; wrap.startup(); assert(wrap.tick(UINT32_MAX-40,false,false,false,false,false,true)==523);
 assert(wrap.tick(60,false,false,false,false,false,true)==0);
 puts("PASS buzzer: boot, battery priorities/repeat, mission, reverse, manual cancel, wraparound");
}
