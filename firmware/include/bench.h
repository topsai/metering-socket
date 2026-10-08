#pragma once
#ifdef METERING_BENCH
#include <sys/time.h>
// This header is compiled only into esp32c3_bench; no network injection endpoints.
bool benchStream=false;uint8_t benchFrame[23]={0};uint32_t benchFeed=0;String benchLine;
void benchLoop(){
 if(benchStream&&millis()-benchFeed>=500){benchFeed=millis();acceptMeter(benchFrame,23);}
 while(Serial.available()){
  char ch=Serial.read();if(ch=='\r')continue;if(ch!='\n'){if(benchLine.length()<2048)benchLine+=ch;else benchLine="";continue;}
  JsonDocument input,response;auto error=deserializeJson(input,benchLine);benchLine="";if(error){response["error"]="invalid_json";}else{
   String op=input["op"]|"status";
   if(op=="safety_pause"){benchSafetyPauseUntil=millis()+min(input["ms"].as<uint32_t>(),uint32_t(1000));response["ok"]=true;
   }else if(op=="cf1"){Guard lock;cf1Count+=input["pulses"].as<uint32_t>();response["ok"]=true;
   }else if(op=="key"){benchKeyHeld=input["held"]|false;response["ok"]=true;
   }else if(op=="trace"){DriveEvent events[64];uint32_t count;{Guard lock;count=driveIndex;memcpy(events,driveTrace,sizeof(events));}JsonArray trace=response["trace"].to<JsonArray>();for(uint32_t i=count>64?count-64:0;i<count;++i){const DriveEvent& e=events[i%64];JsonObject row=trace.add<JsonObject>();row["ms"]=e.ms;row["ina"]=e.ina;row["inb"]=e.inb;row["external"]=e.external;}
   }else if(op=="provision"){
    ssid=input["ssid"].as<String>();password=input["password"].as<String>();broker=input["broker"]|"";port=input["mqtt_port"]|1883;muser=input["mqtt_user"]|"";mpass=input["mqtt_password"]|"";save();mqtt.disconnect();mqtt.setServer(broker.c_str(),port);WiFi.begin(ssid.c_str(),password.c_str());response["ok"]=true;
   }else if(op=="inputs"){
    Guard lock;benchInputOverride=input["override"]|true;benchInput1=input["input1"]|false;benchInput2=input["input2"]|false;response["ok"]=true;
   }else if(op=="meter"){
    String hex=input["frame"]|"";bool valid=hex.length()==46;uint8_t candidate[23];
    for(int i=0;i<23&&valid;i++){char pair[3]={hex[2*i],hex[2*i+1],0};char* end=nullptr;candidate[i]=strtoul(pair,&end,16);valid=end&&*end==0;}
    if(valid)valid=acceptMeter(candidate,23);if(valid){memcpy(benchFrame,candidate,23);benchStream=input["stream"]|false;benchFeed=millis();}response["accepted"]=valid;
   }else if(op=="stream_off"){benchStream=false;response["ok"]=true;
   }else if(op=="clock"){timeval tv;tv.tv_sec=input["epoch"].as<long>();tv.tv_usec=0;settimeofday(&tv,nullptr);response["ok"]=true;
   }else if(op=="flush_energy"){prefs.putDouble("energy",energy.kwh);response["ok"]=true;
   }else if(op=="clear_energy"){energy=Energy{};prefs.putDouble("energy",0);response["ok"]=true;
   }else if(op=="wifi_off"){WiFi.mode(WIFI_OFF);response["ok"]=true;
   }else if(op=="wifi_on"){WiFi.mode(WIFI_STA);WiFi.begin(ssid.c_str(),password.c_str());response["ok"]=true;
   }else if(op=="setup_exit"){setupMode=false;WiFi.softAPdisconnect(true);response["ok"]=true;
   }else if(op=="setup"){startSetup();response["ok"]=true;
   }else if(op=="reboot"){response["ok"]=true;serializeJson(response,Serial);Serial.println();Serial.flush();delay(100);ESP.restart();return;
   }else if(op!="status"){response["error"]="unknown_op";}
   if(op=="status"){fillState(response);response["token"]=token;response["gpio_relay"]=digitalRead(RELAY_PIN);response["gpio_ina"]=digitalRead(LATCH_INA_PIN);response["gpio_inb"]=digitalRead(LATCH_INB_PIN);response["gpio_led"]=digitalRead(LED_PIN);response["heap"]=ESP.getFreeHeap();response["uptime_ms"]=millis();response["epoch"]=time(nullptr);response["setup_mode"]=setupMode;response["wifi_connected"]=WiFi.status()==WL_CONNECTED;}
  }
  serializeJson(response,Serial);Serial.println();
 }
}
#endif
