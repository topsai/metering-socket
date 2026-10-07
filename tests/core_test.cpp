#include "core.h"
#include <cassert>
#include <cstdio>
using namespace socketcore;
int main(){
 Controller c; c.tick(0,false,false,true,0,0); assert(!c.output);
 c.rule=Rule::Both; c.command(true,0); c.tick(1,true,false,true,0,0); assert(!c.output && !c.requested);
 c.command(true,2); c.tick(2,true,true,true,0,0); assert(c.output);
 c.tick(3,false,true,true,0,0); assert(!c.output); c.tick(4,true,true,true,0,0); assert(!c.output);
 c.rule=Rule::EitherFollow; c.tick(5,false,true,true,0,0); assert(c.output);
 c.stop();c.tick(5,false,false,true,0,0);assert(!c.output&&c.paused&&!c.fault);c.resumePause();c.tick(5,false,true,true,0,0);assert(c.output);
 c.tick(6,true,true,true,20,100); assert(!c.output && c.fault); c.tick(7,true,true,true,0,0); assert(!c.output);
 c.stop();c.resumePause();c.tick(7,true,true,true,0,0);assert(c.fault&&!c.output);
 c.rule=Rule::Manual;c.startTimer(1,0);c.tick(2000,false,false,true,0,0);assert(c.fault&&!c.output);
 c.resetFault(false); assert(c.fault); c.resetFault(true); assert(!c.fault);
 c.rule=Rule::Manual; c.command(true,0xfffffff0); c.startTimer(1,0xfffffff0); c.tick(990,false,false,true,0,0); assert(!c.output);
 Debounce d; assert(!d.update(true,0)); assert(!d.update(false,10)); assert(!d.update(true,20)); assert(d.update(true,60));
 uint8_t b[23]={0x55}; b[10]=0xff;b[11]=0xff;b[12]=0xff; uint8_t sum=0x58;for(int i=0;i<22;i++)sum+=b[i];b[22]=sum^255;
 MeterRaw m;assert(parseMeter(b,23,m));assert(m.power==-1);b[3]^=1;assert(!parseMeter(b,23,m));assert(!parseMeter(b,22,m));
 Energy e; e.add(0xfffffe,100);e.add(1,100);assert(e.kwh==0.03);e.add(0,100);assert(e.kwh==0.03);
 puts("PASS: startup, interlock, no restart, follow, fault, reset, timer wrap, debounce, checksum, signed power, counter wrap/reset");
}
