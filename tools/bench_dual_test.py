"""Bare COM3 board only: synthetic meter/input/CF1 values, no relay or mains."""
import datetime,json,time
from bench_client import Bench,PRIVATE
from bench_ota import upload
results=[]
def passed(name):results.append(name);print('PASS:',name,flush=True)
b=Bench()
def cmd(path,channel='onboard',**kw):return b.api(path,dict(kw,channel=channel))
def selected(s,channel):return s if channel=='onboard' else s['external']
try:
 b.usb('reboot');time.sleep(3);d=b.discover();b.provision();time.sleep(.3);d=b.usb();assert not d['relay'] and not d['external']['relay'] and d['gpio_relay']==0 and d['gpio_ina']==0 and d['gpio_inb']==0 and d['latch_known'] and not d['latch_estimated'];passed('startup forces estimated latch OFF and external LOW')
 b.api(token='wrong',expected=401)
 for path,body in [('/api/relay',{'on':True}),('/api/timer',{'seconds':1}),('/api/reset',{}),('/api/config',{})]:b.api(path,dict(body,channel='invalid'),expected=400)
 for body in [{'latch_on_ina':1},{'latch_pulse_ms':49},{'latch_pulse_ms':201}]:b.api('/api/config',body,expected=400)
 passed('auth/channel/pulse configuration validation')
 assert not cmd('/api/relay',on=True)['relay'];assert not cmd('/api/relay','external',on=True)['external']['relay'];passed('both ON blocked without real meter')
 assert b.meter()['accepted'];b.inputs(True,False)
 for channel in ['onboard','external']:
  cmd('/api/config',channel,rule='manual',schedule_on=-1,schedule_off=-1,max_current=10,max_power=2200);cmd('/api/reset',channel)
 cmd('/api/relay',on=True);cmd('/api/relay','external',on=True);time.sleep(.2);d=b.usb();assert d['relay'] and d['external']['relay'] and d['latch_estimated'] and d['gpio_relay']==1 and d['gpio_ina']==0 and d['gpio_inb']==0
 cmd('/api/relay','external',on=False);assert b.api()['relay'];cmd('/api/relay','external',on=True);cmd('/api/relay',on=False);time.sleep(.15);assert b.api()['external']['relay'];passed('independent actual external GPIO and latch estimate')
 cmd('/api/relay',on=True)
 for _ in range(20):
  d=b.usb()
  if d['gpio_ina']==1:break
  time.sleep(.005)
 assert d['gpio_ina']==1 and d['gpio_inb']==0
 b.usb('safety_pause',ms=400);time.sleep(.15);d=b.usb();assert d['gpio_ina']==0 and d['gpio_inb']==0 and d['latch_known'] and d['latch_estimated'];time.sleep(.3);passed('IRAM hardware timer deasserts coil while safety task is paused')
 cmd('/api/relay',on=False);time.sleep(.2)
 trace=b.usb('trace')['trace'];assert all(not(e['ina'] and e['inb']) for e in trace)
 pulses=[];start=None
 for event in trace:
  if event['ina'] or event['inb']:
   if start is None:start=event
  elif start is not None:pulses.append((start,event));start=None
 assert pulses
 assert all(90<=c['ms']-a['ms']<=120 for a,c in pulses if c['ms']-a['ms']<200);passed('drive trace never activates both inputs and normal pulse cutoff timing')
 b.api('/api/config',{'latch_on_ina':False});cmd('/api/reset');cmd('/api/relay',on=True)
 for _ in range(25):
  d=b.usb()
  if d['gpio_inb']==1:break
  time.sleep(.005)
 assert d['gpio_inb']==1 and d['gpio_ina']==0;time.sleep(.15);assert b.api()['latch_estimated'];cmd('/api/relay',on=False);time.sleep(.15);b.api('/api/config',{'latch_on_ina':True});passed('configurable coil direction inversion on actual GPIO')
 rules=['manual','input1','input2','both','either','both_follow','either_follow']
 for channel in ['onboard','external']:
  other='external' if channel=='onboard' else 'onboard';cmd('/api/config',other,rule='manual');cmd('/api/reset',other);cmd('/api/relay',other,on=False)
  for rule in rules:
   cmd('/api/config',channel,rule=rule)
   for a,c in [(False,False),(False,True),(True,False),(True,True)]:
    b.inputs(a,c);cmd('/api/reset',channel)
    allowed=rule=='manual' or (a if rule=='input1' else c if rule=='input2' else a and c if rule.startswith('both') else a or c)
    if rule.endswith('follow'):b.api('/api/relay',{'channel':channel,'on':True},expected=409)
    else:cmd('/api/relay',channel,on=True)
    time.sleep(.04);s=b.api();assert selected(s,channel)['relay']==allowed,(channel,rule,a,c);assert not selected(s,other)['relay']
   passed(channel+' '+rule+' truth table and channel isolation')
 for channel in ['onboard','external']:cmd('/api/config',channel,rule='manual');cmd('/api/reset',channel);cmd('/api/relay',channel,on=True)
 cmd('/api/timer','external',seconds=1);time.sleep(1.2);s=b.api();assert s['relay'] and not s['external']['relay'];passed('external timer does not stop onboard')
 cmd('/api/relay','external',on=True);cmd('/api/timer',seconds=1);time.sleep(1.2);s=b.api();assert not s['relay'] and s['external']['relay'];passed('onboard timer does not stop external')
 energy=b.api()['energy'];before=b.api()['cf1_count'];b.usb('cf1',pulses=20);time.sleep(1.1);s=b.api();assert s['cf1_count']==before+20 and s['energy']==energy;passed('CF1 fixture count without double counting UART energy')
 b.meter(current=12);time.sleep(.2);s=b.api();assert s['fault'] and s['external']['fault'] and not s['relay'] and not s['external']['relay'] and not s['latch_estimated'];passed('measured overload stops both with latch OFF pulse')
 b.meter();b.inputs(True,True)
 for channel in ['onboard','external']:cmd('/api/config',channel,rule='either_follow');cmd('/api/reset',channel)
 time.sleep(.2);assert b.api()['relay'] and b.api()['external']['relay']
 upload(b.device,b'not-firmware',expected=400);time.sleep(.3);s=b.api();assert not s['relay'] and not s['external']['relay'] and s['fault'] and s['external']['fault'] and not s['latch_estimated'];passed('failed OTA locks both follow channels OFF until reset')
 for channel in ['onboard','external']:cmd('/api/config',channel,rule='manual');cmd('/api/reset',channel);cmd('/api/relay',channel,on=True)
 b.usb('key',held=True);time.sleep(3.4);b.usb('key',held=False);d=b.usb();assert d['setup_mode'] and not d['relay'] and not d['external']['relay'];time.sleep(.25);assert not b.usb()['latch_estimated'];b.usb('setup_exit');passed('original KEY long hold enters setup and stops both outputs')
 epoch=int(datetime.datetime(2026,10,8,10,0,tzinfo=datetime.timezone(datetime.timedelta(hours=8))).timestamp())
 for channel in ['onboard','external']:cmd('/api/reset',channel)
 cmd('/api/config',schedule_on=600,schedule_off=601);cmd('/api/config','external',schedule_on=601,schedule_off=602)
 b.usb('clock',epoch=epoch);time.sleep(.2);s=b.api();assert s['relay'] and not s['external']['relay']
 b.usb('clock',epoch=epoch+60);time.sleep(.2);s=b.api();assert not s['relay'] and s['external']['relay']
 b.usb('clock',epoch=epoch+120);time.sleep(.2);assert not b.api()['external']['relay'];passed('separate daily schedules on chip')
 cmd('/api/config',rule='input1',max_current=9,schedule_on=-1,schedule_off=-1);cmd('/api/config','external',rule='either',max_current=8,schedule_on=-1,schedule_off=-1)
 b.usb('reboot');time.sleep(3);d=b.discover();assert d['rule']=='input1' and d['external']['rule']=='either' and d['max_current']==9 and d['external']['max_current']==8 and d['cf1_count']==0 and not d['relay'] and not d['external']['relay'] and not d['latch_estimated'];passed('separate NVS config persistence and reboot OFF reset')
 b.provision();b.meter();b.inputs(True,False)
 for channel in ['onboard','external']:cmd('/api/config',channel,rule='manual',max_current=10,max_power=2200,schedule_on=-1,schedule_off=-1);cmd('/api/reset',channel);cmd('/api/relay',channel,on=False)
 passed('safe settings restored for HA/App tests')
finally:
 b.close();(PRIVATE/'dual-results.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
