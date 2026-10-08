#include "hardware.h"
#include <cassert>
#include <cstdio>
using namespace socketcore;
int main(){
 KeyGesture k;assert(k.update(true,0)==KeyAction::None);assert(k.update(false,10)==KeyAction::None);assert(k.update(true,20)==KeyAction::None);assert(k.update(true,60)==KeyAction::None);
 assert(k.update(false,100)==KeyAction::None);assert(k.update(false,140)==KeyAction::Toggle);assert(k.update(false,200)==KeyAction::None);
 k.update(true,300);k.update(true,340);assert(k.update(true,3340)==KeyAction::Setup);assert(k.update(true,5000)==KeyAction::None);k.update(false,5100);assert(k.update(false,5140)==KeyAction::None);
 KeyGesture wrap;wrap.update(true,0xfffffff0);wrap.update(true,25);wrap.update(false,100);assert(wrap.update(false,140)==KeyAction::Toggle);
 Controller c;c.tick(0,true,true,true,0,0);toggleLocal(c,1,true);c.tick(2,true,true,true,0,0);assert(c.output);toggleLocal(c,3,true);c.tick(4,true,true,true,0,0);assert(!c.output);
 c.fault=true;toggleLocal(c,5,true);assert(c.fault&&!c.requested);c.fault=false;toggleLocal(c,6,false);assert(!c.requested);
 c.rule=Rule::BothFollow;c.tick(7,true,true,true,0,0);toggleLocal(c,8,true);c.tick(9,true,true,true,0,0);assert(c.paused&&!c.output);toggleLocal(c,10,true);c.tick(11,true,true,true,0,0);assert(!c.paused&&c.output);
 c.rule=Rule::Both;c.command(false,12);toggleLocal(c,13,true);c.tick(14,true,false,true,0,0);assert(!c.output&&!c.requested);
 puts("PASS: key debounce/single toggle, long hold consumes release, clock wrap, local toggle/fault/meter/interlock/follow pause");
}
