"""Bare-board LED/OTA-abort checks; always restore the production image."""
import json,socket,time,urllib.parse,urllib.request
from bench_client import Bench,ROOT,PRIVATE
from bench_ota import upload
device=json.loads((PRIVATE/'device.json').read_text());results=[];b=None
def passed(s):results.append(s);print('PASS:',s,flush=True)
def api(path='/api/state',body=None):
 return json.load(urllib.request.build_opener(urllib.request.ProxyHandler({})).open(urllib.request.Request(device['host']+path,data=None if body is None else json.dumps(body).encode(),headers={'Authorization':'Bearer '+device['token'],'Content-Type':'application/json'}),timeout=10))
try:
 upload(device,(ROOT/'firmware/.pio/build/esp32c3_bench/firmware.bin').read_bytes());time.sleep(5);b=Bench();b.provision();b.meter();b.inputs(True,True)
 d=b.usb();assert d['led_on'] and d['gpio_led']==0;passed('connected status LED active LOW GPIO readback')
 b.usb('setup');values=[]
 for _ in range(8):values.append(b.usb()['gpio_led']);time.sleep(.09)
 assert set(values)=={0,1};b.usb('setup_exit');passed('setup status LED blinks on original GPIO2')
 b.usb('wifi_off');values=[]
 for _ in range(18):values.append(b.usb()['gpio_led']);time.sleep(.12)
 assert set(values)=={0,1};b.usb('wifi_on');b.provision();passed('disconnected status LED blinks and reconnects')
 for channel in ['onboard','external']:
  b.api('/api/config',{'channel':channel,'rule':'either_follow','schedule_on':-1,'schedule_off':-1});b.api('/api/reset',{'channel':channel})
 time.sleep(.2);s=b.api();assert s['relay'] and s['external']['relay']
 upload(device,b'invalid',401,token='invalid');assert b.api()['relay'] and b.api()['external']['relay'];passed('unauthenticated OTA does not alter either active channel')
 u=urllib.parse.urlparse(device['host']);boundary='AbortBench';prefix=(f'--{boundary}\r\nContent-Disposition: form-data; name="firmware"; filename="firmware.bin"\r\nContent-Type: application/octet-stream\r\n\r\n').encode()
 image=(ROOT/'firmware/.pio/build/esp32c3_bench/firmware.bin').read_bytes()
 with socket.create_connection((u.hostname,u.port or 80),timeout=10) as sock:
  headers=f'POST /api/ota HTTP/1.1\r\nHost: {u.hostname}\r\nAuthorization: Bearer {device["token"]}\r\nContent-Type: multipart/form-data; boundary={boundary}\r\nContent-Length: {len(image)+len(prefix)+200}\r\nConnection: close\r\n\r\n'
  sock.sendall(headers.encode()+prefix+image[:16384]);time.sleep(.5)
 time.sleep(1);b.meter();time.sleep(.3);s=b.api();assert s['fault'] and s['external']['fault'] and not s['relay'] and not s['external']['relay'] and not s['latch_estimated'];passed('interrupted real OTA keeps both follow channels locked OFF even after valid meter resumes')
 # Explicit empty broker is supported; both HTTP controls remain usable.
 b.api('/api/config',{'broker':''});b.meter()
 for channel in ['onboard','external']:
  b.api('/api/config',{'channel':channel,'rule':'manual'});b.api('/api/reset',{'channel':channel});b.api('/api/relay',{'channel':channel,'on':True})
 time.sleep(.2);s=b.api();assert not s['mqtt_connected'] and s['relay'] and s['external']['relay'];passed('direct LAN HTTP operates both outputs with MQTT disabled')
 b.provision();b.usb('stream_off');time.sleep(5.4);s=b.api();assert not s['meter_valid'] and not s['relay'] and not s['external']['relay'] and s['voltage'] is None and not s['latch_estimated'];passed('five-second stale-meter cutoff shuts both outputs and latch drive')
finally:
 if b:
  try:
   b.usb('clear_energy');b.usb('stream_off')
   for channel in ['onboard','external']:b.api('/api/config',{'channel':channel,'rule':'manual','max_current':10,'max_power':2200,'schedule_on':-1,'schedule_off':-1});b.api('/api/relay',{'channel':channel,'on':False})
  finally:b.close()
 upload(device,(ROOT/'firmware/.pio/build/esp32c3/firmware.bin').read_bytes());time.sleep(5);s=api();assert 'bench_mode' not in s and not s['relay'] and not s['external']['relay'] and not s['latch_estimated'] and s['energy']==0;passed('latest production firmware restored after extra checks')
 (PRIVATE/'dual-extra-results.json').write_text(json.dumps(results,ensure_ascii=False,indent=2))
