#pragma once
#include <stdint.h>
#include "core.h"
namespace socketcore {
enum class KeyAction { None, Toggle, Setup };
struct KeyGesture {
 Debounce debounce;bool down=false,consumed=false;uint32_t pressedAt=0;
 KeyAction update(bool pressed,uint32_t now){
  bool stable=debounce.update(pressed,now);
  if(stable&&!down){down=true;consumed=false;pressedAt=now;}
  if(down&&!consumed&&uint32_t(now-pressedAt)>=3000){consumed=true;if(!stable)down=false;return KeyAction::Setup;}
  if(!stable&&down){down=false;return consumed?KeyAction::None:KeyAction::Toggle;}
  return KeyAction::None;
 }
};
inline void toggleLocal(Controller& c,uint32_t now,bool meterHealthy){
 bool follow=c.rule==Rule::BothFollow||c.rule==Rule::EitherFollow;
 if(c.output||c.requested){if(follow)c.stop();else c.command(false,now);return;}
 if(c.fault||!meterHealthy)return;
 c.resumePause();if(!follow)c.command(true,now);
}
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
