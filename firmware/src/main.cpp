#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Update.h>
#include <time.h>
#include "core.h"
#include "pins.h"
#include "config_validation.h"
using namespace socketcore;
WebServer server(80); WiFiClient net; PubSubClient mqtt(net); Preferences prefs;
Controller control; Debounce inputs[2]; Energy energy; MeterRaw raw;
portMUX_TYPE safetyMux=portMUX_INITIALIZER_UNLOCKED;
struct Guard { Guard(){portENTER_CRITICAL(&safetyMux);} ~Guard(){portEXIT_CRITICAL(&safetyMux);} };
String id,token,ssid,password,broker,muser,mpass;uint16_t port=1883;
float vref=15883.34116f,iref=251065.6814f,pref=623.0270705f;
bool pressed[2]={false,false},seen=false,setupMode=false,otaOk=false,otaActive=false;uint32_t lastMeter=0,lastPoll=0,lastPublish=0,lastSave=0,lastConnect=0,setupStart=0;
int scheduleOn=-1,scheduleOff=-1,lastMinute=-1;uint8_t packet[23];size_t packetLen=0;uint32_t packetStart=0;
const char* rules[]={"manual","input1","input2","both","either","both_follow","either_follow"};
String base(){return "meteringsocket/"+id;}
bool healthy(){Guard lock;return seen && millis()-lastMeter<5000 && raw.current/iref<=control.maxCurrent && fabs(raw.power/pref)<=control.maxPower;}
void command(bool on){Guard lock;control.command(on,millis());}
void reply(int status,const String& value){server.send(status,"application/json; charset=utf-8",value);}
bool auth(){if(server.header("Authorization")=="Bearer "+token)return true;reply(401,"{\"error\":\"unauthorized\"}");return false;}
void fillState(JsonDocument& d){
 Controller snapshot; MeterRaw rawSnapshot;bool inputSnapshot[2],seenSnapshot;uint32_t lastSnapshot;float vSnapshot,iSnapshot,pSnapshot;
 {Guard lock;snapshot=control;rawSnapshot=raw;inputSnapshot[0]=pressed[0];inputSnapshot[1]=pressed[1];seenSnapshot=seen;lastSnapshot=lastMeter;vSnapshot=vref;iSnapshot=iref;pSnapshot=pref;}
 const Controller& control=snapshot;const MeterRaw& raw=rawSnapshot;const bool* pressed=inputSnapshot;bool seen=seenSnapshot;uint32_t lastMeter=lastSnapshot;float vref=vSnapshot,iref=iSnapshot,pref=pSnapshot;
 d["id"]=id;d["name"]="计量插座";d["relay"]=control.output;d["requested"]=control.requested;d["input1"]=pressed[0];d["input2"]=pressed[1];d["fault"]=control.fault||control.paused;d["paused"]=control.paused;d["reason"]=control.reason;d["meter_valid"]=seen&&millis()-lastMeter<5000;
 if(d["meter_valid"].as<bool>()){d["voltage"]=raw.voltage/vref;d["current"]=raw.current/iref;d["power"]=raw.power/pref;d["frequency"]=raw.period?1000000.0/raw.period:0;}else{d["voltage"]=nullptr;d["current"]=nullptr;d["power"]=nullptr;d["frequency"]=nullptr;}
 d["energy"]=energy.kwh;d["rule"]=rules[int(control.rule)];d["max_current"]=control.maxCurrent;d["max_power"]=control.maxPower;d["countdown"]=control.timer?uint32_t(control.deadline-millis())/1000:0;d["ip"]=WiFi.localIP().toString();d["mqtt_connected"]=mqtt.connected();d["schedule_on"]=scheduleOn;d["schedule_off"]=scheduleOff;d["vref"]=vref;d["iref"]=iref;d["pref"]=pref;d["broker"]=broker;d["mqtt_port"]=port;d["mqtt_user"]=muser;
}
String state(){JsonDocument d;fillState(d);String s;serializeJson(d,s);return s;}
void save(){JsonDocument d;d["ssid"]=ssid;d["password"]=password;d["broker"]=broker;d["mqtt_user"]=muser;d["mqtt_password"]=mpass;d["mqtt_port"]=port;d["rule"]=int(control.rule);d["max_current"]=control.maxCurrent;d["max_power"]=control.maxPower;d["schedule_on"]=scheduleOn;d["schedule_off"]=scheduleOff;d["vref"]=vref;d["iref"]=iref;d["pref"]=pref;String s;serializeJson(d,s);prefs.putString("config",s);}
void load(){token=prefs.getString("token");if(token.length()!=32){char s[33];snprintf(s,sizeof(s),"%08lx%08lx%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random());token=s;prefs.putString("token",token);}JsonDocument d;deserializeJson(d,prefs.getString("config","{}"));ssid=d["ssid"]|"";password=d["password"]|"";broker=d["broker"]|"";muser=d["mqtt_user"]|"";mpass=d["mqtt_password"]|"";port=d["mqtt_port"]|1883;control.rule=Rule(d["rule"]|0);control.maxCurrent=d["max_current"]|10.0f;control.maxPower=d["max_power"]|2200.0f;scheduleOn=d["schedule_on"]|-1;scheduleOff=d["schedule_off"]|-1;vref=d["vref"]|15883.34116f;iref=d["iref"]|251065.6814f;pref=d["pref"]|623.0270705f;energy.kwh=prefs.getDouble("energy",0);}
void config(){if(!auth())return;JsonDocument d;if(server.arg("plain").length()>2048||deserializeJson(d,server.arg("plain"))){reply(400,"{\"error\":\"invalid_json\"}");return;}
 // Validate the complete request before changing any state or saving to flash.
 if(const char* error=validateConfig(d)){JsonDocument response;response["error"]=error;String text;serializeJson(response,text);reply(400,text);return;}
 int rule=int(control.rule);if(!d["rule"].isNull()){rule=-1;for(int i=0;i<7;i++)if(d["rule"].as<String>()==rules[i])rule=i;if(rule<0){reply(400,"{\"error\":\"unknown_rule\"}");return;}}
 {Guard lock;control.command(false,millis());control.rule=Rule(rule);
 if(!d["max_current"].isNull())control.maxCurrent=d["max_current"].as<float>();if(!d["max_power"].isNull())control.maxPower=d["max_power"].as<float>();
 if(!d["vref"].isNull())vref=d["vref"];if(!d["iref"].isNull())iref=d["iref"];if(!d["pref"].isNull())pref=d["pref"];}
 if(!d["schedule_on"].isNull())scheduleOn=d["schedule_on"];if(!d["schedule_off"].isNull())scheduleOff=d["schedule_off"];
 if(!d["broker"].isNull())broker=d["broker"].as<String>();if(!d["mqtt_user"].isNull())muser=d["mqtt_user"].as<String>();if(!d["mqtt_password"].isNull())mpass=d["mqtt_password"].as<String>();if(!d["mqtt_port"].isNull())port=d["mqtt_port"];
 bool wifiChange=!d["ssid"].isNull();if(wifiChange){ssid=d["ssid"].as<String>();password=d["password"].as<String>();}save();mqtt.disconnect();mqtt.setServer(broker.c_str(),port);reply(200,"{\"ok\":true}");if(wifiChange)WiFi.begin(ssid.c_str(),password.c_str());
}
void applyOutput(){Guard lock;control.tick(millis(),pressed[0],pressed[1],seen&&millis()-lastMeter<5000,raw.current/iref,raw.power/pref);digitalWrite(RELAY_PIN,control.output&&!otaActive?HIGH:LOW);}
void safetyTask(void*){for(;;){uint32_t now=millis();{Guard lock;pressed[0]=inputs[0].update(digitalRead(INPUT1_PIN)==LOW,now);pressed[1]=inputs[1].update(digitalRead(INPUT2_PIN)==LOW,now);}applyOutput();vTaskDelay(pdMS_TO_TICKS(5));}}
void publishDiscovery(){
 const char* keys[]={"relay","input1","input2","voltage","current","power","energy","frequency","fault"};
 const char* names[]={"插座继电器","微动开关1","微动开关2","电压","电流","功率","累计电量","频率","保护锁定"};
 const char* units[]={"","","","V","A","W","kWh","Hz",""};
 const char* classes[]={"","","","voltage","current","power","energy","frequency","problem"};
 for(int i=0;i<9;i++){String component=i==0?"switch":(i==1||i==2||i==8)?"binary_sensor":"sensor";JsonDocument d;d["name"]=names[i];d["unique_id"]=id+"_"+keys[i];d["state_topic"]=base()+"/state";d["availability_topic"]=base()+"/availability";d["value_template"]=String("{{ value_json.")+keys[i]+" }}";d["device"]["identifiers"][0]=id;d["device"]["name"]="计量插座 "+id;d["device"]["manufacturer"]="DIY";d["device"]["model"]="ESP32-C3 BL0942";
 if(i==0){d["command_topic"]=base()+"/relay/set";d["payload_on"]="ON";d["payload_off"]="OFF";d["state_on"]="True";d["state_off"]="False";}
 else if(i==1||i==2||i==8){d["payload_on"]="True";d["payload_off"]="False";}else{d["unit_of_measurement"]=units[i];d["device_class"]=classes[i];d["state_class"]=i==6?"total_increasing":"measurement";d["expire_after"]=15;}
 if(i==8)d["device_class"]="problem";
 String s;serializeJson(d,s);mqtt.publish(("homeassistant/"+component+"/"+id+"/"+keys[i]+"/config").c_str(),s.c_str(),true);
 }
}
void stop(){Guard lock;control.stop();digitalWrite(RELAY_PIN,LOW);}
void startSetup(){setupMode=true;setupStart=millis();stop();WiFi.mode(WIFI_AP_STA);WiFi.softAP(("MeteringSocket-"+id.substring(6)).c_str(),"socketsetup");}
const char DASH[] PROGMEM=R"HTML(<!doctype html><html lang="zh"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>计量插座</title><style>body{font:17px system-ui;background:#101c27;color:#edf6fa;max-width:540px;margin:40px auto;padding:20px}button,input,select{font:inherit;padding:12px;margin:5px;border-radius:12px}button{background:#42dcc0}pre{white-space:pre-wrap}section{background:#213342;padding:16px;border-radius:20px}</style><h1>计量插座</h1><section><input id="token" placeholder="设备令牌"><button onclick="pair()">首次配对</button><pre id="state">尚未连接</pre><button onclick="cmd(true)">开启</button><button onclick="cmd(false)">关闭</button><button onclick="api('/api/reset',{})">解除保护</button></section><h3>Wi-Fi 配网</h3><input id="ssid" placeholder="2.4GHz Wi-Fi 名称"><input id="pass" type="password" placeholder="Wi-Fi 密码"><button onclick="api('/api/config',{ssid:ssid.value,password:pass.value})">保存</button><h3>联动规则</h3><select id="rule"><option value="manual">手动</option><option value="input1">开关1允许</option><option value="input2">开关2允许</option><option value="both">两个都按下允许</option><option value="either">任一个按下允许</option><option value="both_follow">跟随两路同时按下</option><option value="either_follow">跟随任一路按下</option></select><button onclick="api('/api/config',{rule:rule.value})">保存规则</button><h3>升级固件</h3><input id="bin" type="file" accept=".bin"><button onclick="upgrade()">上传</button><script>token.value=localStorage.token||'';async function api(path,data){try{let r=await fetch(path,{method:data?'POST':'GET',headers:{'Authorization':'Bearer '+token.value,'Content-Type':'application/json'},body:data?JSON.stringify(data):undefined});let j=await r.json();if(!r.ok)throw Error(j.error);localStorage.token=token.value;return j}catch(e){state.textContent=e.message;throw e}}async function pair(){let r=await fetch('/api/pair');let j=await r.json();if(j.token){token.value=j.token;localStorage.token=j.token}else alert('请长按配网按钮进入配对模式')}function cmd(on){api('/api/relay',{on})}async function upgrade(){let r=await fetch('/api/ota',{method:'POST',headers:{Authorization:'Bearer '+token.value},body:(()=>{let f=new FormData();f.append("firmware",bin.files[0],"firmware.bin");return f})()});alert(await r.text())}setInterval(async()=>{if(token.value)try{state.textContent=JSON.stringify(await api('/api/state'),null,2)}catch(e){}},2000)</script></html>)HTML";
void setup(){
 digitalWrite(RELAY_PIN,LOW);pinMode(RELAY_PIN,OUTPUT);pinMode(INPUT1_PIN,INPUT_PULLUP);pinMode(INPUT2_PIN,INPUT_PULLUP);pinMode(SETUP_PIN,INPUT_PULLUP);pinMode(CF1_PIN,INPUT);
 Serial.begin(115200);Serial1.begin(4800,SERIAL_8N1,METER_RX_PIN,METER_TX_PIN);prefs.begin("socket",false);id="ms"+String(uint32_t(ESP.getEfuseMac()),HEX);load();WiFi.mode(WIFI_STA);if(ssid.isEmpty())startSetup();else WiFi.begin(ssid.c_str(),password.c_str());configTime(8*3600,0,"pool.ntp.org","ntp.aliyun.com");MDNS.begin(id.c_str());MDNS.addService("http","tcp",80);
 const char* headers[]={"Authorization"};server.collectHeaders(headers,1);server.on("/",HTTP_GET,[]{server.send_P(200,"text/html; charset=utf-8",DASH);});
 server.on("/api/pair",HTTP_GET,[]{if(!setupMode||server.client().localIP()!=WiFi.softAPIP()){reply(403,"{\"error\":\"physical_setup_required\"}");return;}JsonDocument d;d["token"]=token;d["id"]=id;String s;serializeJson(d,s);reply(200,s);});
 server.on("/api/state",HTTP_GET,[]{if(auth())reply(200,state());});server.on("/api/config",HTTP_POST,config);
 server.on("/api/relay",HTTP_POST,[]{if(!auth())return;JsonDocument d;if(deserializeJson(d,server.arg("plain"))||!d["on"].is<bool>()){reply(400,"{\"error\":\"boolean_required\"}");return;}bool follow=control.rule==Rule::BothFollow||control.rule==Rule::EitherFollow;if(follow&&d["on"].as<bool>()){reply(409,"{\"error\":\"follow_mode\"}");return;}if(follow)stop();else command(d["on"]);applyOutput();reply(200,state());});
 server.on("/api/reset",HTTP_POST,[]{if(auth()){bool ok=healthy();{Guard lock;control.resetFault(ok);}applyOutput();reply(ok?200:409,state());}});
 server.on("/api/timer",HTTP_POST,[]{if(!auth())return;JsonDocument d;if(deserializeJson(d,server.arg("plain"))||!d["seconds"].is<uint32_t>()||d["seconds"].as<uint32_t>()>86400){reply(400,"{\"error\":\"timer_range\"}");return;}{Guard lock;control.startTimer(d["seconds"],millis());}reply(200,state());});
 server.on("/api/ota",HTTP_POST,[]{if(!auth())return;reply(otaOk?200:400,otaOk?"{\"ok\":true,\"reboot\":true}":"{\"error\":\"update_failed\"}");if(otaOk){delay(200);ESP.restart();}},[]{HTTPUpload& u=server.upload();if(u.status==UPLOAD_FILE_START){otaOk=false;if(server.header("Authorization")!="Bearer "+token)return;{Guard lock;otaActive=true;control.command(false,millis());digitalWrite(RELAY_PIN,LOW);}otaOk=Update.begin(UPDATE_SIZE_UNKNOWN);}else if(u.status==UPLOAD_FILE_WRITE){if(otaActive&&otaOk)otaOk=Update.write(u.buf,u.currentSize)==u.currentSize;}else if(u.status==UPLOAD_FILE_END){if(otaActive){otaOk=otaOk&&Update.end(true);{Guard lock;otaActive=false;control.fault=true;}}}else if(u.status==UPLOAD_FILE_ABORTED){Update.abort();{Guard lock;otaActive=false;control.fault=true;}}});
 if(xTaskCreate(safetyTask,"relay_safety",3072,nullptr,4,nullptr)!=pdPASS){Serial.println("safety_task_init_failed");for(;;){digitalWrite(RELAY_PIN,LOW);delay(1000);}}server.begin();mqtt.setBufferSize(2048);mqtt.setSocketTimeout(1);mqtt.setServer(broker.c_str(),port);mqtt.setCallback([](char* topic,uint8_t* payload,unsigned int n){String s;for(unsigned i=0;i<n;i++)s+=char(payload[i]);if(String(topic)=="homeassistant/status"&&s=="online"){publishDiscovery();return;}if(String(topic)!=base()+"/relay/set"||(s!="ON"&&s!="OFF"))return;bool follow=control.rule==Rule::BothFollow||control.rule==Rule::EitherFollow;if(follow&&s=="ON")return;if(follow)stop();else command(s=="ON");applyOutput();mqtt.publish((base()+"/state").c_str(),state().c_str(),true);});
}
void loop(){
 uint32_t now=millis();
 static uint32_t held=0;if(digitalRead(SETUP_PIN)==LOW){if(!held)held=now;if(now-held>3000&&!setupMode)startSetup();}else held=0;
 if(setupMode&&now-setupStart>600000){WiFi.softAPdisconnect(true);setupMode=false;}
 if(now-lastPoll>=1000){lastPoll=now;Serial1.write(0x58);Serial1.write(0xAA);}
 if(packetLen&&now-packetStart>200)packetLen=0;
 while(Serial1.available()){uint8_t b=Serial1.read();if(!packetLen){if(b!=0x55)continue;packetStart=now;}packet[packetLen++]=b;if(packetLen==23){MeterRaw m;if(parseMeter(packet,23,m)){{Guard lock;raw=m;seen=true;lastMeter=now;}energy.add(m.count,pref*3600000.0/419430.4);}packetLen=0;}}
 server.handleClient();
 if(!broker.isEmpty()&&WiFi.status()==WL_CONNECTED&&!mqtt.connected()&&now-lastConnect>10000){lastConnect=now;if(mqtt.connect(id.c_str(),muser.c_str(),mpass.c_str(),(base()+"/availability").c_str(),1,true,"offline")){mqtt.publish((base()+"/availability").c_str(),"online",true);mqtt.subscribe((base()+"/relay/set").c_str());mqtt.subscribe("homeassistant/status");publishDiscovery();}}
 mqtt.loop();if(mqtt.connected()&&now-lastPublish>2000){lastPublish=now;mqtt.publish((base()+"/state").c_str(),state().c_str(),true);}
 time_t t=time(nullptr);if(t>1700000000){tm local;localtime_r(&t,&local);int minute=local.tm_hour*60+local.tm_min;if(minute!=lastMinute){lastMinute=minute;if(minute==scheduleOn){bool ok=healthy();Guard lock;if(ok)control.resumePause();control.command(true,now);}if(minute==scheduleOff)stop();}}
 if(now-lastSave>300000){lastSave=now;prefs.putDouble("energy",energy.kwh);}delay(1);
}
