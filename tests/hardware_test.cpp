#include "hardware.h"
#include "core.h"
#include <cassert>
#include <cstdio>
using namespace socketcore;
int main(){
 LatchingRelay r;
 r.tick(0,false,true,100);assert(!r.ina&&!r.inb&&!r.known);
 r.tick(5,false,true,100);assert(!r.ina&&r.inb);
 r.tick(104,false,true,100);assert(r.inb&&!r.known);
 r.tick(105,false,true,100);assert(!r.ina&&!r.inb&&r.known&&!r.estimated);
 r.tick(200,false,true,100);assert(!r.busy());
 r.tick(201,true,true,100);r.tick(206,true,true,100);assert(r.ina&&!r.inb);
 r.tick(207,false,true,100);assert(!r.ina&&!r.inb&&!r.known);
 r.tick(211,false,true,100);assert(!r.ina&&!r.inb);
 r.tick(212,false,true,100);assert(!r.ina&&r.inb);
 r.tick(312,false,true,100);assert(r.known&&!r.estimated);
 r.tick(400,true,false,100);r.tick(405,true,false,100);assert(!r.ina&&r.inb);
 r.tick(505,true,false,100);assert(r.known&&r.estimated&&!r.busy());
 LatchingRelay wrap;wrap.tick(0xfffffff0,false,true,50);wrap.tick(0xfffffff5,false,true,50);wrap.tick(39,false,true,50);assert(wrap.known&&!wrap.busy());
 Controller onboard,external;onboard.rule=Rule::Both;external.rule=Rule::Manual;
 onboard.command(true,0);external.command(true,0);onboard.tick(1,true,false,true,0,0);external.tick(1,true,false,true,0,0);assert(!onboard.output&&external.output);
 external.tick(2,true,false,false,0,0);assert(!external.output&&external.fault);
 puts("PASS: startup OFF pulse, width cutoff, idle no retrigger, emergency reversal deadtime, polarity inversion, clock wrap, independent rules/shared meter safety");
}
