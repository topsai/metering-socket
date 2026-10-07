#include "config_validation.h"
#include <cassert>
#include <cstdio>
int main(){
 const char* bad[]={"[]","{\"max_current\":\"99999\"}","{\"max_current\":17}","{\"max_power\":false}","{\"max_power\":0}","{\"vref\":-1}","{\"rule\":\"bad\"}","{\"schedule_on\":1440}","{\"mqtt_port\":0}","{\"ssid\":123}"};
 for(auto text:bad){JsonDocument d;assert(!deserializeJson(d,text));assert(validateConfig(d)!=nullptr);}
 JsonDocument d;deserializeJson(d,"{\"max_current\":10,\"max_power\":2200,\"rule\":\"both\",\"schedule_on\":-1}");assert(validateConfig(d)==nullptr);puts("PASS: config numeric types, ranges, rule, time, network fields");
}
