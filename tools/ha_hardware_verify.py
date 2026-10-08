"""Run inside HA container. Real ESP32-C3, USB fixture values, no load attached."""
import json,sys,time,urllib.request,urllib.parse
device=json.load(sys.stdin)
records=json.load(open('/config/.storage/auth'))['data']['refresh_tokens']
refresh=next(x for x in records if x.get('token_type')=='normal')
body=urllib.parse.urlencode({'grant_type':'refresh_token','refresh_token':refresh['token'],'client_id':refresh['client_id']}).encode()
access=json.load(urllib.request.urlopen(urllib.request.Request('http://localhost:8123/auth/token',data=body)))['access_token']
def request(url,body=None,token=access):
 req=urllib.request.Request(url,data=None if body is None else json.dumps(body).encode(),headers={'Authorization':'Bearer '+token,'Content-Type':'application/json'})
 return json.load(urllib.request.build_opener(urllib.request.ProxyHandler({})).open(req,timeout=10))
def api(path,body=None):return request('http://localhost:8123/api/'+path,body)
def state():return request(device['host']+'/api/state',token=device['token'])
prefix='计量插座 '+device['id']
mode=device.get('mode','control')
found=[]
for _ in range(30):
 found=[x for x in api('states') if x.get('attributes',{}).get('friendly_name','').startswith(prefix)]
 if len(found)==13:
  if mode=='offline' and all(x['state']=='unavailable' for x in found):break
  if mode!='offline' and state()['mqtt_connected']:
   if mode=='production' and all(x['state']!='unavailable' for x in found):break
   if mode!='production' and all(x['state'] not in ('unknown','unavailable') for x in found):break
 time.sleep(1)
assert len(found)==13,f'Found {len(found)} entities'
relay=next(x['entity_id'] for x in found if x['entity_id'].startswith('switch.') and '外接' not in x['attributes'].get('friendly_name',''))
if mode=='offline':
 assert all(x['state']=='unavailable' for x in found)
 print('PASS: real ESP32 Wi-Fi disconnect -> MQTT last will -> all 13 HA entities unavailable',flush=True)
 sys.exit(0)
if mode=='production':
 assert all(x['state']!='unavailable' for x in found)
 s=state();assert not s['relay'] and not s['meter_valid'] and s['mqtt_connected'] and 'bench_mode' not in s and s['energy']==0 and s['rule']=='manual'
 api('services/switch/turn_on',{'entity_id':relay});time.sleep(3)
 assert not state()['relay'] and api('states/'+relay)['state']=='off'
 found=[x for x in api('states') if x.get('attributes',{}).get('friendly_name','').startswith(prefix)]
 sensors=[x for x in found if x['entity_id'].startswith('sensor.') and x['attributes'].get('unit_of_measurement') in ('V','A','W','Hz') and 'CF1' not in x['attributes'].get('friendly_name','')]
 assert len(sensors)==4 and all(x['state']=='unknown' for x in sensors)
 print('PASS: production MQTT online, real missing meter blocks HA ON, four readings unknown',flush=True)
 sys.exit(0)
assert all(x['state'] not in ('unknown','unavailable') for x in found)
assert state()['mqtt_connected'] and state()['meter_valid']
if mode=='blocked':
 api('services/switch/turn_on',{'entity_id':relay});time.sleep(3)
 s=state();assert not s['relay'] and s['reason']==device['reason'] and api('states/'+relay)['state']=='off'
 print('PASS: HA ON constrained by real firmware '+s['reason'],flush=True)
 sys.exit(0)
if mode=='follow':
 api('services/switch/turn_off',{'entity_id':relay});time.sleep(3)
 assert state()['paused'] and not state()['relay'] and api('states/'+relay)['state']=='off'
 api('services/switch/turn_on',{'entity_id':relay});time.sleep(3)
 assert state()['paused'] and not state()['relay'] and not state()['requested'] and api('states/'+relay)['state']=='off'
 print('PASS: HA OFF pauses follow, HA ON cannot bypass follow pause',flush=True)
 sys.exit(0)
for command,expected in [('turn_on',True),('turn_off',False)]:
 api('services/switch/'+command,{'entity_id':relay})
 for _ in range(12):
  if state()['relay']==expected and api('states/'+relay)['state']==('on' if expected else 'off'):break
  time.sleep(1)
 s=state()
 assert s['relay']==expected,{k:s[k] for k in ('relay','reason','paused','meter_valid','mqtt_connected','input1','input2')}
 assert api('states/'+relay)['state']==('on' if expected else 'off')
 print('PASS: Home Assistant '+command+' -> real ESP32-C3 -> actual state acknowledgement',flush=True)
print('PASS: real ESP32 MQTT firmware discovers all 13 entities with fixture meter/input data',flush=True)
