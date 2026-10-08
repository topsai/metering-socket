#pragma once
#include <stdint.h>
namespace socketcore {
// Commands are estimates; the board has no relay contact feedback.
struct LatchingRelay {
 enum Phase { Idle, Gap, Pulse };
 Phase phase=Idle;bool ina=false,inb=false,known=false,estimated=false,pulseTarget=false;uint32_t since=0;
 bool busy()const{return phase!=Idle;}
 void invalidate(){ina=inb=false;known=false;phase=Idle;}
 void tick(uint32_t now,bool target,bool onIna,uint32_t width){
  if(phase==Pulse){
   if(target!=pulseTarget){ina=inb=false;known=false;pulseTarget=target;phase=Gap;since=now;return;}
   if(uint32_t(now-since)>=width){ina=inb=false;known=true;estimated=pulseTarget;phase=Idle;}
   return;
  }
  if(phase==Idle){if(known&&estimated==target)return;ina=inb=false;known=false;pulseTarget=target;since=now;phase=Gap;return;}
  pulseTarget=target;
  if(uint32_t(now-since)>=5){ina=target?onIna:!onIna;inb=!ina;since=now;phase=Pulse;}
 }
};
}
