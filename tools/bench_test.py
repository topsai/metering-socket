"""Live ESP32-C3 API tests with USB synthetic metering/input fixture, no peripherals."""
import json,time,datetime
from bench_client import Bench,PRIVATE
results=[]
def passed(name):results.append(name);print('PASS:',name,flush=True)
b=Bench()
try:
 d=b.discover();assert not d['meter_valid'] and not d['relay'] and d['gpio_relay']==0;passed('startup fail-closed without BL0942')
 b.api(token='wrong',expected=401);b.api('/api/pair',expected=403);passed('HTTP authentication and station pairing forbidden')
 assert not b.api('/api/relay',{'on':True})['relay'];b.api('/api/reset',{},expected=409);passed('missing meter blocks ON and reset')
 for data in ({'max_current':'99999'},{'max_current':17},{'max_power':False},{'rule':'invalid'},{'schedule_on':1440},{'mqtt_port':0}):b.api('/api/config',data,expected=400)
 passed('invalid config rejected atomically')
 assert not b.meter(corrupt=True)['accepted'];assert b.meter()['accepted'];time.sleep(.1);s=b.api();assert s['meter_valid'] and abs(s['voltage']-230)<.01 and abs(s['current']-.25)<.001 and abs(s['power']-57.5)<.01;passed('real chip BL0942 frame parser and unit conversion with USB fixture')
 rules=['manual','input1','input2','both','either','both_follow','either_follow']
 for rule in rules:
  b.api('/api/config',{'rule':rule});b.api('/api/reset',{})
  for a,c in ((False,False),(False,True),(True,False),(True,True)):
   b.inputs(a,c);b.api('/api/reset',{})
   allowed=rule=='manual' or (a if rule=='input1' else c if rule=='input2' else a and c if rule.startswith('both') else a or c)
   if rule.endswith('follow'):b.api('/api/relay',{'on':True},expected=409)
   else:b.api('/api/relay',{'on':True})
   time.sleep(.04);s=b.api();assert s['relay']==allowed,(rule,a,c)
   assert b.usb()['gpio_relay']==int(allowed)
  passed('rule '+rule+' truth table + GPIO latch readback')
 b.api('/api/config',{'rule':'both'});b.inputs(True,True);b.api('/api/reset',{});assert b.api('/api/relay',{'on':True})['relay'];b.inputs(False,True);assert not b.api()['relay'];b.inputs(True,True);assert not b.api()['relay'];passed('input invalidation cancels request without automatic restart')
 b.api('/api/config',{'rule':'either_follow'});b.api('/api/reset',{});assert b.api()['relay'];s=b.api('/api/relay',{'on':False});assert s['paused'] and not s['relay'];b.inputs(False,False);b.inputs(True,True);assert not b.api()['relay'];b.api('/api/reset',{});assert b.api()['relay'];passed('follow OFF pauses until reset')
 b.meter(current=12);time.sleep(.06);assert b.api()['fault'] and not b.api()['relay'];b.api('/api/reset',{},expected=409);b.api('/api/relay',{'on':False});b.meter();time.sleep(.06);assert b.api()['fault'];b.api('/api/reset',{});passed('overcurrent latch, recovery requires explicit reset')
 b.meter(power=2500);time.sleep(.06);assert b.api()['fault'] and not b.api()['relay'];b.meter(power=-57.5);time.sleep(.06);assert b.api()['power']<0 and b.api()['fault'];b.api('/api/reset',{});passed('overpower protection and signed power')
 b.api('/api/config',{'rule':'manual'});b.api('/api/reset',{});b.api('/api/relay',{'on':True});b.api('/api/timer',{'seconds':1});time.sleep(1.3);assert not b.api()['relay'];b.api('/api/timer',{'seconds':86401},expected=400);passed('real-time countdown closes output')
 b.api('/api/relay',{'on':True});b.usb('stream_off');time.sleep(5.4);s=b.api();assert not s['meter_valid'] and s['fault'] and not s['relay'] and s['voltage'] is None;passed('five-second missing-frame cutoff with Wi-Fi active')
 b.meter();time.sleep(.1);b.api('/api/reset',{});b.api('/api/config',{'schedule_on':600,'schedule_off':601});
 epoch=int(datetime.datetime(2026,10,8,10,0,tzinfo=datetime.timezone(datetime.timedelta(hours=8))).timestamp());b.usb('clock',epoch=epoch);time.sleep(.2);assert b.api()['relay'];b.usb('clock',epoch=epoch+60);time.sleep(.2);assert not b.api()['relay'] and b.api()['paused'];b.usb('clock',epoch=epoch+86400);time.sleep(.2);assert b.api()['relay'];passed('daily ON/OFF and next-day pause resume on chip')
 b.api('/api/config',{'schedule_on':600,'schedule_off':600});b.usb('clock',epoch=epoch+120);time.sleep(.1);b.usb('clock',epoch=epoch);time.sleep(.2);assert not b.api()['relay'];passed('same-minute daily OFF priority')
 b.api('/api/config',{'schedule_on':-1,'schedule_off':-1,'rule':'manual'});b.meter(count=100);e=b.api()['energy'];b.meter(count=105);assert b.api()['energy']>e;b.usb('flush_energy');e=b.api()['energy'];b.api('/api/config',{'rule':'both','max_current':9});b.usb('reboot');time.sleep(3);d=b.discover();assert d['rule']=='both' and d['max_current']==9 and abs(d['energy']-e)<1e-8 and not d['relay'] and not d['meter_valid'];passed('NVS config/energy/token persistence and reboot fail-closed')
 b.meter();b.inputs(True,True);time.sleep(.1);b.api('/api/config',{'rule':'manual','max_current':10});b.api('/api/reset',{});passed('restored manual safe test settings')
finally:
 b.close();(PRIVATE/'logic-results.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
