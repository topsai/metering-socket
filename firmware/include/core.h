#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
namespace socketcore {
enum class Rule { Manual, Input1, Input2, Both, Either, BothFollow, EitherFollow };
struct Debounce { bool stable=false, candidate=false; uint32_t since=0; bool update(bool value,uint32_t now){if(value!=candidate){candidate=value;since=now;}if(now-since>=35)stable=candidate;return stable;} };
struct Controller {
 Rule rule=Rule::Manual; bool requested=false,output=false,fault=false,paused=false,timer=false;uint32_t deadline=0;
 float maxCurrent=10,maxPower=2200; const char* reason="off";
 void command(bool on,uint32_t){requested=on;if(!on){timer=false;output=false;}}
 void stop(){command(false,0);paused=true;if(!fault)reason="paused";}
 void resumePause(){paused=false;}
 void startTimer(uint32_t seconds,uint32_t now){timer=seconds>0;deadline=now+seconds*1000;}
 void resetFault(bool healthy){if(healthy){fault=false;paused=false;requested=false;output=false;}}
 void tick(uint32_t now,bool a,bool b,bool meter,float current,float power){
  bool allowed=rule==Rule::Manual || (rule==Rule::Input1?a:rule==Rule::Input2?b:rule==Rule::Both||rule==Rule::BothFollow?a&&b:a||b);
  bool follow=rule==Rule::BothFollow||rule==Rule::EitherFollow;
  if(timer && int32_t(now-deadline)>=0){timer=false;requested=false;if(follow)paused=true;if(!fault)reason="timer";}
  if(!meter || current>maxCurrent || fabs(power)>maxPower){if(!fault)reason=!meter?"meter_stale":"overload";fault=true;requested=false;timer=false;}
  if(!allowed){requested=false;if(!fault&&!paused)reason="interlock";}
  output=!fault && !paused && allowed && (follow||requested);
  if(output)reason="on";else if(!fault&&paused)reason="paused";else if(!fault&&allowed)reason="off";
 }
};
struct MeterRaw {uint32_t current=0,voltage=0,count=0;int32_t power=0;uint16_t period=0;};
inline uint32_t le24(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16);}
inline bool parseMeter(const uint8_t* p,size_t n,MeterRaw& m){
 if(n!=23||p[0]!=0x55)return false;uint8_t sum=0x58;for(size_t i=0;i<22;i++)sum+=p[i];if(uint8_t(sum^255)!=p[22])return false;
 m.current=le24(p+1);m.voltage=le24(p+4);uint32_t w=le24(p+10);m.power=(w&0x800000)?int32_t(w|0xff000000):int32_t(w);m.count=le24(p+13);m.period=p[16]|(uint16_t(p[17])<<8);return true;
}
struct Energy {double kwh=0;bool initialized=false;uint32_t previous=0;void add(uint32_t count,double reference){if(initialized){uint32_t delta=(count-previous)&0xffffff;if(delta<100000 && reference>0)kwh+=delta/reference;}previous=count;initialized=true;}};
}
