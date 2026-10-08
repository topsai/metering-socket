#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Update.h>
#include <time.h>
#include "soc/gpio_struct.h"
#include "esp_intr_alloc.h"
#include "core.h"
#include "pins.h"
#include "hardware.h"
#include "config_validation.h"
using namespace socketcore;
WebServer server(80); WiFiClient net; PubSubClient mqtt(net); Preferences prefs;
KeyGesture physicalKey;bool setupRequested=false;Controller control,externalControl; LatchingRelay latch; bool latchOnIna=true,ledOn=false;uint32_t latchPulseMs=100;hw_timer_t* coilTimer=nullptr;volatile uint32_t cf1Count=0;float cf1Frequency=0; Debounce inputs[2]; Energy energy; MeterRaw raw;
portMUX_TYPE safetyMux=portMUX_INITIALIZER_UNLOCKED;
struct Guard { Guard(){portENTER_CRITICAL(&safetyMux);} ~Guard(){portEXIT_CRITICAL(&safetyMux);} };
String id,token,ssid,password,broker,muser,mpass;uint16_t port=1883;
float vref=15883.34116f,iref=251065.6814f,pref=623.0270705f;
bool pressed[2]={false,false},seen=false,setupMode=false,otaOk=false,otaActive=false;uint32_t lastMeter=0,lastPoll=0,lastPublish=0,lastSave=0,lastConnect=0,setupStart=0;
int scheduleOn=-1,scheduleOff=-1,externalScheduleOn=-1,externalScheduleOff=-1,lastMinute=-1;uint8_t packet[23];size_t packetLen=0;uint32_t packetStart=0;
const char* rules[]={"manual","input1","input2","both","either","both_follow","either_follow"};
#ifdef METERING_BENCH
bool benchInputOverride=false,benchInput1=false,benchInput2=false,benchKeyHeld=false;
struct DriveEvent{uint32_t ms;bool ina,inb,external;};DriveEvent driveTrace[64];uint32_t driveIndex=0,benchSafetyPauseUntil=0;
#endif
bool acceptMeter(const uint8_t* bytes,size_t length){MeterRaw m;if(!parseMeter(bytes,length,m))return false;{Guard lock;raw=m;seen=true;lastMeter=millis();}energy.add(m.count,pref*3600000.0/419430.4);return true;}
String base(){return "meteringsocket/"+id;}
void reply(int status,const String& value);
Controller& channelControl(bool external){return external?externalControl:control;}
bool healthy(bool external=false){Guard lock;const Controller& c=channelControl(external);return seen && millis()-lastMeter<5000 && raw.current/iref<=c.maxCurrent && fabs(raw.power/pref)<=c.maxPower;}
bool selectChannel(const JsonDocument& d,bool& external){if(!d.is<JsonObjectConst>()){reply(400,"{\"error\":\"object_required\"}");return false;}if(d["channel"].isNull()){external=false;return true;}if(!d["channel"].is<const char*>()){reply(400,"{\"error\":\"unknown_channel\"}");return false;}String name=d["channel"].as<String>();if(name!="onboard"&&name!="external"){reply(400,"{\"error\":\"unknown_channel\"}");return false;}external=name=="external";return true;}
void command(bool on,bool external=false){Guard lock;channelControl(external).command(on,millis());}
void reply(int status,const String& value){server.send(status,"application/json; charset=utf-8",value);}
bool auth(){if(server.header("Authorization")=="Bearer "+token)return true;reply(401,"{\"error\":\"unauthorized\"}");return false;}
void fillState(JsonDocument& d){
#ifdef METERING_BENCH
 d["bench_mode"]=true;
#endif
 Controller snapshot,externalSnapshot;LatchingRelay latchSnapshot;uint32_t cfCount;float cfHz;bool ledSnapshot,onInaSnapshot;uint32_t pulseSnapshot; MeterRaw rawSnapshot;bool inputSnapshot[2],seenSnapshot;uint32_t lastSnapshot;float vSnapshot,iSnapshot,pSnapshot;
 {Guard lock;snapshot=control;externalSnapshot=externalControl;latchSnapshot=latch;cfCount=cf1Count;cfHz=cf1Frequency;ledSnapshot=ledOn;onInaSnapshot=latchOnIna;pulseSnapshot=latchPulseMs;rawSnapshot=raw;inputSnapshot[0]=pressed[0];inputSnapshot[1]=pressed[1];seenSnapshot=seen;lastSnapshot=lastMeter;vSnapshot=vref;iSnapshot=iref;pSnapshot=pref;}
 const Controller& control=snapshot;const MeterRaw& raw=rawSnapshot;const bool* pressed=inputSnapshot;bool seen=seenSnapshot;uint32_t lastMeter=lastSnapshot;float vref=vSnapshot,iref=iSnapshot,pref=pSnapshot;
 JsonObject ext=d["external"].to<JsonObject>();ext["relay"]=externalSnapshot.output;ext["requested"]=externalSnapshot.requested;ext["rule"]=rules[int(externalSnapshot.rule)];ext["fault"]=externalSnapshot.fault||externalSnapshot.paused;ext["paused"]=externalSnapshot.paused;ext["reason"]=externalSnapshot.reason;ext["countdown"]=externalSnapshot.timer?uint32_t(externalSnapshot.deadline-millis())/1000:0;ext["max_current"]=externalSnapshot.maxCurrent;ext["max_power"]=externalSnapshot.maxPower;ext["schedule_on"]=externalScheduleOn;ext["schedule_off"]=externalScheduleOff;
 d["cf1_count"]=cfCount;d["cf1_frequency"]=cfHz;d["led_on"]=ledSnapshot;d["latch_busy"]=latchSnapshot.busy();d["latch_known"]=latchSnapshot.known;if(latchSnapshot.known)d["latch_estimated"]=latchSnapshot.estimated;else d["latch_estimated"]=nullptr;d["latch_on_ina"]=onInaSnapshot;d["latch_pulse_ms"]=pulseSnapshot;
 d["id"]=id;d["name"]="计量插座";d["relay"]=control.output;d["requested"]=control.requested;d["input1"]=pressed[0];d["input2"]=pressed[1];d["fault"]=control.fault||control.paused;d["paused"]=control.paused;d["reason"]=control.reason;d["meter_valid"]=seen&&millis()-lastMeter<5000;
 if(d["meter_valid"].as<bool>()){d["voltage"]=raw.voltage/vref;d["current"]=raw.current/iref;d["power"]=raw.power/pref;d["frequency"]=raw.period?1000000.0/raw.period:0;}else{d["voltage"]=nullptr;d["current"]=nullptr;d["power"]=nullptr;d["frequency"]=nullptr;}
 d["energy"]=energy.kwh;d["rule"]=rules[int(control.rule)];d["max_current"]=control.maxCurrent;d["max_power"]=control.maxPower;d["countdown"]=control.timer?uint32_t(control.deadline-millis())/1000:0;d["ip"]=WiFi.localIP().toString();d["mqtt_connected"]=mqtt.connected();d["schedule_on"]=scheduleOn;d["schedule_off"]=scheduleOff;d["vref"]=vref;d["iref"]=iref;d["pref"]=pref;d["broker"]=broker;d["mqtt_port"]=port;d["mqtt_user"]=muser;
}
String state(){JsonDocument d;fillState(d);String s;serializeJson(d,s);return s;}
void save(){JsonDocument d;d["external_rule"]=int(externalControl.rule);d["external_max_current"]=externalControl.maxCurrent;d["external_max_power"]=externalControl.maxPower;d["external_schedule_on"]=externalScheduleOn;d["external_schedule_off"]=externalScheduleOff;d["latch_on_ina"]=latchOnIna;d["latch_pulse_ms"]=latchPulseMs;d["ssid"]=ssid;d["password"]=password;d["broker"]=broker;d["mqtt_user"]=muser;d["mqtt_password"]=mpass;d["mqtt_port"]=port;d["rule"]=int(control.rule);d["max_current"]=control.maxCurrent;d["max_power"]=control.maxPower;d["schedule_on"]=scheduleOn;d["schedule_off"]=scheduleOff;d["vref"]=vref;d["iref"]=iref;d["pref"]=pref;String s;serializeJson(d,s);prefs.putString("config",s);}
void load(){token=prefs.getString("token");if(token.length()!=32){char s[33];snprintf(s,sizeof(s),"%08lx%08lx%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random());token=s;prefs.putString("token",token);}JsonDocument d;deserializeJson(d,prefs.getString("config","{}"));ssid=d["ssid"]|"";password=d["password"]|"";broker=d["broker"]|"";muser=d["mqtt_user"]|"";mpass=d["mqtt_password"]|"";port=d["mqtt_port"]|1883;control.rule=Rule(d["rule"]|0);control.maxCurrent=d["max_current"]|10.0f;control.maxPower=d["max_power"]|2200.0f;scheduleOn=d["schedule_on"]|-1;scheduleOff=d["schedule_off"]|-1;vref=d["vref"]|15883.34116f;iref=d["iref"]|251065.6814f;pref=d["pref"]|623.0270705f;externalControl.rule=Rule(d["external_rule"]|0);externalControl.maxCurrent=d["external_max_current"]|10.0f;externalControl.maxPower=d["external_max_power"]|2200.0f;externalScheduleOn=d["external_schedule_on"]|-1;externalScheduleOff=d["external_schedule_off"]|-1;latchOnIna=d["latch_on_ina"]|true;latchPulseMs=d["latch_pulse_ms"]|100;if(latchPulseMs<50||latchPulseMs>200)latchPulseMs=100;energy.kwh=prefs.getDouble("energy",0);}
void config(){if(!auth())return;JsonDocument d;if(server.arg("plain").length()>2048||deserializeJson(d,server.arg("plain"))){reply(400,"{\"error\":\"invalid_json\"}");return;}
 // Validate the complete request before changing any state or saving to flash.
 if(const char* error=validateConfig(d)){JsonDocument response;response["error"]=error;String text;serializeJson(response,text);reply(400,text);return;}
 bool external=false;if(!selectChannel(d,external))return;Controller& selected=channelControl(external);int rule=int(selected.rule);if(!d["rule"].isNull()){rule=-1;for(int i=0;i<7;i++)if(d["rule"].as<String>()==rules[i])rule=i;if(rule<0){reply(400,"{\"error\":\"unknown_rule\"}");return;}}
 bool globalChange=false;for(const char* key:{"ssid","password","broker","mqtt_user","mqtt_password","mqtt_port","vref","iref","pref","latch_on_ina","latch_pulse_ms"})if(!d[key].isNull())globalChange=true;
 {Guard lock;if(globalChange){control.command(false,millis());externalControl.command(false,millis());}else selected.command(false,millis());selected.rule=Rule(rule);
 if(!d["max_current"].isNull())selected.maxCurrent=d["max_current"].as<float>();if(!d["max_power"].isNull())selected.maxPower=d["max_power"].as<float>();
 if(!d["latch_on_ina"].isNull()){latchOnIna=d["latch_on_ina"];latch.invalidate();digitalWrite(LATCH_INA_PIN,LOW);digitalWrite(LATCH_INB_PIN,LOW);}if(!d["latch_pulse_ms"].isNull())latchPulseMs=d["latch_pulse_ms"];if(!d["vref"].isNull())vref=d["vref"];if(!d["iref"].isNull())iref=d["iref"];if(!d["pref"].isNull())pref=d["pref"];}
 if(!d["schedule_on"].isNull())(external?externalScheduleOn:scheduleOn)=d["schedule_on"].as<int>();if(!d["schedule_off"].isNull())(external?externalScheduleOff:scheduleOff)=d["schedule_off"].as<int>();
 if(!d["broker"].isNull())broker=d["broker"].as<String>();if(!d["mqtt_user"].isNull())muser=d["mqtt_user"].as<String>();if(!d["mqtt_password"].isNull())mpass=d["mqtt_password"].as<String>();if(!d["mqtt_port"].isNull())port=d["mqtt_port"];
 bool wifiChange=!d["ssid"].isNull();if(wifiChange){ssid=d["ssid"].as<String>();password=d["password"].as<String>();}save();mqtt.disconnect();mqtt.setServer(broker.c_str(),port);reply(200,"{\"ok\":true}");if(wifiChange)WiFi.begin(ssid.c_str(),password.c_str());
}
void IRAM_ATTR coilTimeout(){portENTER_CRITICAL_ISR(&safetyMux);GPIO.out_w1tc.val=(1UL<<LATCH_INA_PIN)|(1UL<<LATCH_INB_PIN);if(latch.phase==LatchingRelay::Pulse){latch.ina=latch.inb=false;latch.known=true;latch.estimated=latch.pulseTarget;latch.phase=LatchingRelay::Idle;}portEXIT_CRITICAL_ISR(&safetyMux);}
void applyOutput(){Guard lock;bool wasPulse=latch.phase==LatchingRelay::Pulse;uint32_t now=millis();bool valid=seen&&now-lastMeter<5000;control.tick(now,pressed[0],pressed[1],valid,raw.current/iref,raw.power/pref);externalControl.tick(now,pressed[0],pressed[1],valid,raw.current/iref,raw.power/pref);latch.tick(now,control.output&&!otaActive,latchOnIna,latchPulseMs);if(wasPulse&&latch.phase!=LatchingRelay::Pulse)timerAlarmDisable(coilTimer);if(!wasPulse&&latch.phase==LatchingRelay::Pulse){timerAlarmDisable(coilTimer);timerWrite(coilTimer,0);timerAlarmWrite(coilTimer,latchPulseMs*1000,false);timerAlarmEnable(coilTimer);}digitalWrite(LATCH_INA_PIN,latch.ina?HIGH:LOW);digitalWrite(LATCH_INB_PIN,latch.inb?HIGH:LOW);digitalWrite(RELAY_PIN,externalControl.output&&!otaActive?HIGH:LOW);
#ifdef METERING_BENCH
 static bool lastA=false,lastB=false,lastE=false;bool e=externalControl.output&&!otaActive;if(latch.ina!=lastA||latch.inb!=lastB||e!=lastE){driveTrace[driveIndex%64]={now,latch.ina,latch.inb,e};++driveIndex;lastA=latch.ina;lastB=latch.inb;lastE=e;}
#endif
}
void IRAM_ATTR cf1Edge(){portENTER_CRITICAL_ISR(&safetyMux);++cf1Count;portEXIT_CRITICAL_ISR(&safetyMux);}
bool waitForOff(){uint32_t start=millis();for(;;){bool done;{Guard lock;done=latch.known&&!latch.estimated&&!latch.busy();}if(done)return true;if(millis()-start>1000)return false;delay(5);}}
void safetyTask(void*){for(;;){uint32_t now=millis();{Guard lock;bool a=digitalRead(INPUT1_PIN)==LOW,b=digitalRead(INPUT2_PIN)==LOW;
#ifdef METERING_BENCH
 if(benchInputOverride){a=benchInput1;b=benchInput2;}
#endif
 bool keyHeld=digitalRead(SETUP_PIN)==LOW;
#ifdef METERING_BENCH
 keyHeld=keyHeld||benchKeyHeld;
#endif
 KeyAction action=physicalKey.update(keyHeld,now);
 if(action==KeyAction::Toggle&&!setupMode&&!setupRequested&&!otaActive){bool valid=seen&&now-lastMeter<5000&&raw.current/iref<=control.maxCurrent&&fabs(raw.power/pref)<=control.maxPower;toggleLocal(control,now,valid);}
 if(action==KeyAction::Setup&&!setupMode&&!otaActive){control.stop();externalControl.stop();setupRequested=true;}
 pressed[0]=inputs[0].update(a,now);pressed[1]=inputs[1].update(b,now);static uint32_t sampleTime=0,sampleCount=0;if(now-sampleTime>=1000){cf1Frequency=float(uint32_t(cf1Count-sampleCount))*1000.0f/uint32_t(now-sampleTime);sampleCount=cf1Count;sampleTime=now;}}
#ifdef METERING_BENCH
 if(int32_t(now-benchSafetyPauseUntil)>=0)
#endif
 applyOutput();vTaskDelay(pdMS_TO_TICKS(5));}}
void publishDiscovery(){
 const char* keys[]={"relay","input1","input2","voltage","current","power","energy","frequency","fault","external_relay","external_fault","cf1_count","cf1_frequency"};
 const char* names[]={"插座继电器","微动开关1","微动开关2","电压","电流","功率","累计电量","频率","保护锁定","外接继电器","外接保护锁定","CF1脉冲计数","CF1脉冲频率"};
 const char* units[]={"","","","V","A","W","kWh","Hz","","","","","Hz"};
 const char* classes[]={"","","","voltage","current","power","energy","frequency","problem","","problem","","frequency"};
 for(int i=0;i<13;i++){String component=(i==0||i==9)?"switch":(i==1||i==2||i==8||i==10)?"binary_sensor":"sensor";JsonDocument d;d["name"]=names[i];d["unique_id"]=id+"_"+keys[i];d["state_topic"]=base()+"/state";d["availability_topic"]=base()+"/availability";d["value_template"]=String("{{ value_json.")+(i==9?"external.relay":i==10?"external.fault":keys[i])+" }}";d["device"]["identifiers"][0]=id;d["device"]["name"]="计量插座 "+id;d["device"]["manufacturer"]="DIY";d["device"]["model"]="ESP32-C3 BL0942";
 if(i==0||i==9){d["command_topic"]=base()+(i==9?"/external/relay/set":"/relay/set");d["payload_on"]="ON";d["payload_off"]="OFF";d["state_on"]="True";d["state_off"]="False";}
 else if(i==1||i==2||i==8||i==10){d["payload_on"]="True";d["payload_off"]="False";}else{d["unit_of_measurement"]=units[i];if(strlen(classes[i]))d["device_class"]=classes[i];d["state_class"]=(i==6||i==11)?"total_increasing":"measurement";d["expire_after"]=15;}
 if(i==8||i==10)d["device_class"]="problem";
 String s;serializeJson(d,s);mqtt.publish(("homeassistant/"+component+"/"+id+"/"+keys[i]+"/config").c_str(),s.c_str(),true);
 }
}
void stop(bool all=true,bool external=false){Guard lock;if(all){control.stop();externalControl.stop();}else channelControl(external).stop();if(all||external)digitalWrite(RELAY_PIN,LOW);}
void startSetup(){setupMode=true;setupStart=millis();stop();WiFi.mode(WIFI_AP_STA);WiFi.softAP(("MeteringSocket-"+id.substring(6)).c_str(),"socketsetup");}
const char DASH[] PROGMEM=R"HTML(<!doctype html><html lang="zh"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>计量插座</title><style>body{font:17px system-ui;background:#101c27;color:#edf6fa;max-width:540px;margin:40px auto;padding:20px}button,input,select{font:inherit;padding:12px;margin:5px;border-radius:12px}button{background:#42dcc0}pre{white-space:pre-wrap}section{background:#213342;padding:16px;border-radius:20px}</style><h1>计量插座</h1><section><input id="token" placeholder="设备令牌"><button onclick="pair()">首次配对</button><pre id="state">尚未连接</pre><h3>板载磁保持继电器（状态为估计值）</h3><button onclick="cmd(true)">开启</button><button onclick="cmd(false)">关闭</button><h3>外接继电器</h3><button onclick="cmd(true,'external')">开启外接</button><button onclick="cmd(false,'external')">关闭外接</button><button onclick="api('/api/reset',{channel:'external'})">解除外接保护</button><button onclick="api('/api/reset',{})">解除保护</button></section><h3>Wi-Fi 配网</h3><input id="ssid" placeholder="2.4GHz Wi-Fi 名称"><input id="pass" type="password" placeholder="Wi-Fi 密码"><button onclick="api('/api/config',{ssid:ssid.value,password:pass.value})">保存</button><h3>联动规则</h3><select id="channel"><option value="onboard">板载</option><option value="external">外接</option></select><select id="rule"><option value="manual">手动</option><option value="input1">开关1允许</option><option value="input2">开关2允许</option><option value="both">两个都按下允许</option><option value="either">任一个按下允许</option><option value="both_follow">跟随两路同时按下</option><option value="either_follow">跟随任一路按下</option></select><button onclick="api('/api/config',{channel:channel.value,rule:rule.value})">保存规则</button><h3>保护上限</h3><input id="maxCurrent" type="number" min="0.1" max="16" step="0.1" placeholder="电流上限 A"><input id="maxPower" type="number" min="1" max="3680" placeholder="功率上限 W"><button onclick="api('/api/config',{channel:channel.value,max_current:Number(maxCurrent.value),max_power:Number(maxPower.value)})">保存保护上限</button><h3>倒计时与定时</h3><input id="seconds" type="number" min="0" max="86400" placeholder="倒计时秒"><button onclick="api('/api/timer',{channel:channel.value,seconds:Number(seconds.value)})">设置倒计时</button><input id="scheduleOn" type="number" min="-1" max="1439" placeholder="开启分钟，-1禁用"><input id="scheduleOff" type="number" min="-1" max="1439" placeholder="关闭分钟，-1禁用"><button onclick="api('/api/config',{channel:channel.value,schedule_on:Number(scheduleOn.value),schedule_off:Number(scheduleOff.value)})">保存定时</button><h3>磁保持方向</h3><select id="direction"><option value="true">INA脉冲开启</option><option value="false">INB脉冲开启</option></select><button onclick="api('/api/config',{latch_on_ina:direction.value==='true'})">保存方向</button><input id="pulseWidth" type="number" value="100" min="50" max="200" placeholder="脉冲毫秒"><button onclick="api('/api/config',{latch_pulse_ms:Number(pulseWidth.value)})">保存脉冲宽度</button><h3>计量校准</h3><input id="voltageRef" type="number" step="any" placeholder="电压 vref"><input id="currentRef" type="number" step="any" placeholder="电流 iref"><input id="powerRef" type="number" step="any" placeholder="功率 pref"><button onclick="api('/api/config',{vref:Number(voltageRef.value),iref:Number(currentRef.value),pref:Number(powerRef.value)})">保存校准系数</button><p>先用低压测试确认方向；无触点反馈。CF1原始计数及频率显示在状态中。</p><h3>升级固件</h3><input id="bin" type="file" accept=".bin"><button onclick="upgrade()">上传</button><script>token.value=localStorage.token||'';async function api(path,data){try{let r=await fetch(path,{method:data?'POST':'GET',headers:{'Authorization':'Bearer '+token.value,'Content-Type':'application/json'},body:data?JSON.stringify(data):undefined});let j=await r.json();if(!r.ok)throw Error(j.error);localStorage.token=token.value;return j}catch(e){state.textContent=e.message;throw e}}async function pair(){let r=await fetch('/api/pair');let j=await r.json();if(j.token){token.value=j.token;localStorage.token=j.token}else alert('请长按配网按钮进入配对模式')}function cmd(on,channel='onboard'){api('/api/relay',{on,channel})}async function upgrade(){let r=await fetch('/api/ota',{method:'POST',headers:{Authorization:'Bearer '+token.value},body:(()=>{let f=new FormData();f.append("firmware",bin.files[0],"firmware.bin");return f})()});alert(await r.text())}setInterval(async()=>{if(token.value)try{state.textContent=JSON.stringify(await api('/api/state'),null,2)}catch(e){}},2000)</script></html>)HTML";
#include "bench.h"
void setup(){
 digitalWrite(LATCH_INA_PIN,LOW);digitalWrite(LATCH_INB_PIN,LOW);pinMode(LATCH_INA_PIN,OUTPUT);pinMode(LATCH_INB_PIN,OUTPUT);digitalWrite(LED_PIN,HIGH);pinMode(LED_PIN,OUTPUT);digitalWrite(RELAY_PIN,LOW);pinMode(RELAY_PIN,OUTPUT);pinMode(INPUT1_PIN,INPUT_PULLUP);pinMode(INPUT2_PIN,INPUT_PULLUP);pinMode(SETUP_PIN,INPUT_PULLUP);pinMode(CF1_PIN,INPUT);attachInterrupt(digitalPinToInterrupt(CF1_PIN),cf1Edge,RISING);coilTimer=timerBegin(0,80,true);if(!coilTimer){Serial.begin(115200);Serial.println("coil_timer_init_failed");for(;;)delay(1000);}timerAttachInterruptFlag(coilTimer,coilTimeout,false,ESP_INTR_FLAG_IRAM);
 Serial.begin(115200);Serial1.begin(4800,SERIAL_8N1,METER_RX_PIN,METER_TX_PIN);prefs.begin("socket",false);id="ms"+String(uint32_t(ESP.getEfuseMac()),HEX);load();WiFi.mode(WIFI_STA);if(ssid.isEmpty())startSetup();else WiFi.begin(ssid.c_str(),password.c_str());configTime(8*3600,0,"pool.ntp.org","ntp.aliyun.com");MDNS.begin(id.c_str());MDNS.addService("http","tcp",80);
 const char* headers[]={"Authorization"};server.collectHeaders(headers,1);server.on("/",HTTP_GET,[]{server.send_P(200,"text/html; charset=utf-8",DASH);});
 server.on("/api/pair",HTTP_GET,[]{if(!setupMode||server.client().localIP()!=WiFi.softAPIP()){reply(403,"{\"error\":\"physical_setup_required\"}");return;}JsonDocument d;d["token"]=token;d["id"]=id;String s;serializeJson(d,s);reply(200,s);});
 server.on("/api/state",HTTP_GET,[]{if(auth())reply(200,state());});server.on("/api/config",HTTP_POST,config);
 server.on("/api/relay",HTTP_POST,[]{if(!auth())return;JsonDocument d;if(deserializeJson(d,server.arg("plain"))||!d["on"].is<bool>()){reply(400,"{\"error\":\"boolean_required\"}");return;}bool external=false;if(!selectChannel(d,external))return;bool follow;{Guard lock;Controller& c=channelControl(external);follow=c.rule==Rule::BothFollow||c.rule==Rule::EitherFollow;}if(follow&&d["on"].as<bool>()){reply(409,"{\"error\":\"follow_mode\"}");return;}if(follow)stop(false,external);else command(d["on"],external);applyOutput();reply(200,state());});
 server.on("/api/reset",HTTP_POST,[]{if(!auth())return;JsonDocument d;if(deserializeJson(d,server.arg("plain"))){reply(400,"{\"error\":\"invalid_json\"}");return;}bool external=false;if(!selectChannel(d,external))return;bool ok=healthy(external);{Guard lock;channelControl(external).resetFault(ok);}applyOutput();reply(ok?200:409,state());});
 server.on("/api/timer",HTTP_POST,[]{if(!auth())return;JsonDocument d;if(deserializeJson(d,server.arg("plain"))||!d["seconds"].is<uint32_t>()||d["seconds"].as<uint32_t>()>86400){reply(400,"{\"error\":\"timer_range\"}");return;}bool external=false;if(!selectChannel(d,external))return;{Guard lock;channelControl(external).startTimer(d["seconds"],millis());}reply(200,state());});
 server.on("/api/ota",HTTP_POST,[]{if(!auth())return;reply(otaOk?200:400,otaOk?"{\"ok\":true,\"reboot\":true}":"{\"error\":\"update_failed\"}");if(otaOk){waitForOff();delay(200);ESP.restart();}},[]{HTTPUpload& u=server.upload();if(u.status==UPLOAD_FILE_START){otaOk=false;if(server.header("Authorization")!="Bearer "+token)return;{Guard lock;otaActive=true;control.stop();externalControl.stop();control.fault=true;externalControl.fault=true;digitalWrite(RELAY_PIN,LOW);}applyOutput();if(!waitForOff()){otaOk=false;return;}otaOk=Update.begin(UPDATE_SIZE_UNKNOWN);}else if(u.status==UPLOAD_FILE_WRITE){if(otaActive&&otaOk)otaOk=Update.write(u.buf,u.currentSize)==u.currentSize;}else if(u.status==UPLOAD_FILE_END){if(otaActive){otaOk=otaOk&&Update.end(true);{Guard lock;otaActive=false;control.fault=true;externalControl.fault=true;}}}else if(u.status==UPLOAD_FILE_ABORTED){Update.abort();{Guard lock;otaActive=false;control.fault=true;externalControl.fault=true;}}});
 if(xTaskCreate(safetyTask,"relay_safety",3072,nullptr,4,nullptr)!=pdPASS){Serial.println("safety_task_init_failed");for(;;){digitalWrite(RELAY_PIN,LOW);delay(1000);}}server.begin();mqtt.setBufferSize(2048);mqtt.setSocketTimeout(1);mqtt.setServer(broker.c_str(),port);mqtt.setCallback([](char* topic,uint8_t* payload,unsigned int n){String s;for(unsigned i=0;i<n;i++)s+=char(payload[i]);if(String(topic)=="homeassistant/status"&&s=="online"){publishDiscovery();return;}bool external=String(topic)==base()+"/external/relay/set";if((!external&&String(topic)!=base()+"/relay/set")||(s!="ON"&&s!="OFF"))return;bool follow;{Guard lock;Controller& c=channelControl(external);follow=c.rule==Rule::BothFollow||c.rule==Rule::EitherFollow;}if(follow&&s=="ON")return;if(follow)stop(false,external);else command(s=="ON",external);applyOutput();mqtt.publish((base()+"/state").c_str(),state().c_str(),true);});
}
void loop(){
#ifdef METERING_BENCH
 benchLoop();
#endif
 uint32_t now=millis();
 {Guard lock;ledOn=setupMode?((now/200)%2==0):WiFi.status()!=WL_CONNECTED?((now/1000)%2==0):true;digitalWrite(LED_PIN,ledOn?LOW:HIGH);}
 bool enterSetup;{Guard lock;enterSetup=setupRequested;setupRequested=false;}if(enterSetup)startSetup();
 if(setupMode&&now-setupStart>600000){WiFi.softAPdisconnect(true);setupMode=false;}
 if(now-lastPoll>=1000){lastPoll=now;Serial1.write(0x58);Serial1.write(0xAA);}
 if(packetLen&&now-packetStart>200)packetLen=0;
 while(Serial1.available()){uint8_t b=Serial1.read();if(!packetLen){if(b!=0x55)continue;packetStart=now;}packet[packetLen++]=b;if(packetLen==23){acceptMeter(packet,23);packetLen=0;}}
 server.handleClient();
 if(!broker.isEmpty()&&WiFi.status()==WL_CONNECTED&&!mqtt.connected()&&now-lastConnect>10000){lastConnect=now;if(mqtt.connect(id.c_str(),muser.c_str(),mpass.c_str(),(base()+"/availability").c_str(),1,true,"offline")){mqtt.publish((base()+"/availability").c_str(),"online",true);mqtt.subscribe((base()+"/relay/set").c_str());mqtt.subscribe((base()+"/external/relay/set").c_str());mqtt.subscribe("homeassistant/status");publishDiscovery();}}
 mqtt.loop();if(mqtt.connected()&&now-lastPublish>2000){lastPublish=now;mqtt.publish((base()+"/state").c_str(),state().c_str(),true);}
 time_t t=time(nullptr);if(t>1700000000){tm local;localtime_r(&t,&local);int minute=local.tm_hour*60+local.tm_min;if(minute!=lastMinute){lastMinute=minute;if(minute==scheduleOn){bool ok=healthy();Guard lock;if(ok)control.resumePause();control.command(true,now);}if(minute==scheduleOff)stop(false,false);if(minute==externalScheduleOn){bool ok=healthy(true);Guard lock;if(ok)externalControl.resumePause();externalControl.command(true,now);}if(minute==externalScheduleOff)stop(false,true);}}
 if(now-lastSave>300000){lastSave=now;prefs.putDouble("energy",energy.kwh);}delay(1);
}
