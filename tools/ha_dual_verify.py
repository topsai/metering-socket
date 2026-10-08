"""Run inside HA container; injected meter data on a bare ESP32-C3 only."""
import json,sys,time,urllib.request,urllib.parse
device=json.load(sys.stdin)
records=json.load(open('/config/.storage/auth'))['data']['refresh_tokens']
refresh=next(x for x in records if x.get('token_type')=='normal')
body=urllib.parse.urlencode({'grant_type':'refresh_token','refresh_token':refresh['token'],'client_id':refresh['client_id']}).encode()
access=json.load(urllib.request.urlopen(urllib.request.Request('http://localhost:8123/auth/token',data=body)))['access_token']
opener=urllib.request.build_opener(urllib.request.ProxyHandler({}))
def request(url,body=None,token=access):
 return json.load(opener.open(urllib.request.Request(url,data=None if body is None else json.dumps(body).encode(),headers={'Authorization':'Bearer '+token,'Content-Type':'application/json'}),timeout=10))
def api(path,body=None):return request('http://localhost:8123/api/'+path,body)
def state():return request(device['host']+'/api/state',token=device['token'])
prefix='计量插座 '+device['id']
production=device.get('mode')=='production'
for _ in range(35):
 found=[s for s in api('states') if s.get('attributes',{}).get('friendly_name','').startswith(prefix)]
 if len(found)==13 and all(s['state']!='unavailable' and (production or s['state']!='unknown') for s in found):break
 time.sleep(1)
assert len(found)==13, len(found)
assert all(s['state']!='unavailable' and (production or s['state']!='unknown') for s in found)
switches=[s for s in found if s['entity_id'].startswith('switch.')];assert len(switches)==2
print('PASS: real ESP32 MQTT discovers thirteen HA entities including two switches and CF1 sensors',flush=True)
if production:
 s=state();assert 'bench_mode' not in s and not s['meter_valid'] and not s['relay'] and not s['external']['relay'] and not s['latch_estimated']
 readings=[x for x in found if x['entity_id'].startswith('sensor.') and 'CF1' not in x['attributes'].get('friendly_name','') and x['attributes'].get('unit_of_measurement') in ('V','A','W','Hz')]
 assert len(readings)==4 and all(x['state']=='unknown' for x in readings)
 for entity in switches:
  api('services/switch/turn_on',{'entity_id':entity['entity_id']});time.sleep(2)
  s=state();assert not s['relay'] and not s['external']['relay']
  assert api('states/'+entity['entity_id'])['state']=='off'
 print('PASS: final production HA online, both MQTT ON blocked without BL0942, four physical readings unknown',flush=True)
 sys.exit(0)
for external in [False,True]:
 entity=next(s['entity_id'] for s in switches if ('外接' in s['attributes']['friendly_name'])==external)
 for action,on in [('turn_on',True),('turn_off',False)]:
  api('services/switch/'+action,{'entity_id':entity})
  for _ in range(15):
   s=state();selected=s['external'] if external else s
   if selected['relay']==on and api('states/'+entity)['state']==('on' if on else 'off'):break
   time.sleep(.5)
  assert selected['relay']==on
  assert (s if external else s['external'])['relay'] is False
  assert api('states/'+entity)['state']==('on' if on else 'off')
 print('PASS: HA '+('external' if external else 'onboard')+' ON/OFF MQTT command and independent state acknowledgement',flush=True)
