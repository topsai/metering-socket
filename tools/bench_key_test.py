"""Bare USB board key fixture only; restore production firmware in finally."""
import json,time
from bench_client import Bench,ROOT,PRIVATE
from bench_ota import upload
device=json.loads((PRIVATE/'device.json').read_text());b=None;results=[]
def passed(s):results.append(s);print('PASS:',s,flush=True)
def press(duration=.12):b.usb('key',held=True);time.sleep(duration);b.usb('key',held=False);time.sleep(.2)
try:
 upload(device,(ROOT/'firmware/.pio/build/esp32c3_bench/firmware.bin').read_bytes());time.sleep(4);b=Bench();b.provision();b.meter();b.inputs(True,True)
 for c in ['onboard','external']:b.api('/api/config',{'channel':c,'rule':'manual','schedule_on':-1,'schedule_off':-1});b.api('/api/reset',{'channel':c})
 b.api('/api/relay',{'channel':'external','on':True})
 press();s=b.api();assert s['relay'] and s['latch_estimated'] and s['external']['relay'];press();s=b.api();assert not s['relay'] and not s['latch_estimated'] and s['external']['relay'];passed('short key toggles board ON/OFF once and preserves external relay')
 b.api('/api/config',{'rule':'both'});b.inputs(True,False);b.api('/api/reset',{});press();assert not b.api()['relay'];b.inputs(True,True);assert not b.api()['relay'];passed('short key cannot bypass input interlock or leave restart request')
 b.meter(current=12);time.sleep(.1);press();assert b.api()['fault'] and not b.api()['relay'];b.meter();press();assert b.api()['fault'] and not b.api()['relay'];passed('key cannot clear overload fault')
 b.api('/api/config',{'rule':'both_follow'});b.api('/api/reset',{});time.sleep(.2);assert b.api()['relay'];press();s=b.api();assert s['paused'] and not s['relay'];press();s=b.api();assert not s['paused'] and s['relay'];passed('follow key OFF pauses and next healthy press resumes without bypass')
 b.usb('key',held=True);time.sleep(3.3);s=b.usb();assert s['setup_mode'] and not s['relay'] and not s['external']['relay'];b.usb('key',held=False);time.sleep(.25);s=b.usb();assert not s['relay'] and not s['external']['relay'] and not s['latch_estimated'];b.usb('setup_exit');passed('long hold enters setup and release never toggles back ON')
 b.api('/api/config',{'rule':'manual'});b.usb('stream_off');time.sleep(5.3);press();s=b.api();assert not s['meter_valid'] and not s['relay'];passed('short key cannot enable board without meter')
finally:
 if b:
  try:
   b.usb('key',held=False);b.usb('clear_energy');b.usb('stream_off')
   for c in ['onboard','external']:b.api('/api/config',{'channel':c,'rule':'manual','schedule_on':-1,'schedule_off':-1});b.api('/api/relay',{'channel':c,'on':False})
  finally:b.close()
 upload(device,(ROOT/'firmware/.pio/build/esp32c3/firmware.bin').read_bytes());time.sleep(4)
 (PRIVATE/'key-results.json').write_text(json.dumps(results,ensure_ascii=False,indent=2))
 print('PASS: production image restored after key tests',flush=True)
