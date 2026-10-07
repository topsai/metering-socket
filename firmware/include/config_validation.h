#pragma once
#include <ArduinoJson.h>
#include <math.h>
#include <string.h>
inline const char* validateConfig(const JsonDocument& d){
 if(!d.is<JsonObjectConst>())return "object_required";
 for(const char* key:{"max_current","max_power","vref","iref","pref"}){
  if(d[key].isNull())continue;
  float max=strcmp(key,"max_current")==0?16:strcmp(key,"max_power")==0?3680:1e9f;
  if(!d[key].is<float>()||!isfinite(d[key].as<float>())||d[key].as<float>()<=0||d[key].as<float>()>max)return "numeric_range";
 }
 for(const char* key:{"schedule_on","schedule_off"})if(!d[key].isNull()&&(!d[key].is<int>()||d[key].as<int>()< -1||d[key].as<int>()>1439))return "schedule_range";
 if(!d["mqtt_port"].isNull()&&(!d["mqtt_port"].is<int>()||d["mqtt_port"].as<int>()<1||d["mqtt_port"].as<int>()>65535))return "port_range";
 for(const char* key:{"ssid","password","broker","mqtt_user","mqtt_password"})if(!d[key].isNull()&&(!d[key].is<const char*>()||strlen(d[key].as<const char*>())>(strcmp(key,"ssid")==0?32:128)))return "string_range";
 if(!d["rule"].isNull()){
  if(!d["rule"].is<const char*>())return "unknown_rule";
  bool valid=false;for(const char* rule:{"manual","input1","input2","both","either","both_follow","either_follow"})if(strcmp(d["rule"],rule)==0)valid=true;
  if(!valid)return "unknown_rule";
 }
 return nullptr;
}
