#include "core.h"
#include <iostream>
#include <sstream>
#include <string>
using namespace socketcore;
int main(){Controller c;bool a=false,b=false,valid=true;float current=0.25,power=57.5;uint32_t now=0;std::string line;
 while(std::getline(std::cin,line)){std::istringstream in(line);std::string op;in>>op;
  if(op=="tick")in>>now>>a>>b>>valid>>current>>power;
  else if(op=="on"){bool value;in>>value;c.command(value,now);}
  else if(op=="rule"){int rule;in>>rule;c.rule=Rule(rule);c.command(false,now);}
  else if(op=="reset")c.resetFault(valid&&current<=c.maxCurrent&&power<=c.maxPower);
  else if(op=="timer"){uint32_t seconds;in>>seconds;c.startTimer(seconds,now);}
  else if(op=="stop")c.stop();
  c.tick(now,a,b,valid,current,power);
  std::cout<<"{\"relay\":"<<(c.output?"true":"false")<<",\"requested\":"<<(c.requested?"true":"false")<<",\"fault\":"<<(c.fault?"true":"false")<<",\"reason\":\""<<c.reason<<"\"}"<<std::endl;
 }
}
