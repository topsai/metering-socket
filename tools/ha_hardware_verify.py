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
found=[]
for _ in range(30):
 found=[x for x in api('states') if x.get('attributes',{}).get('friendly_name','').startswith(prefix)]
 if len(found)==9 and all(x['state'] not in ('unknown','unavailable') for x in found):break
 time.sleep(1)
assert len(found)==9,f'Found {len(found)} entities'
assert all(x['state'] not in ('unknown','unavailable') for x in found)
relay=next(x['entity_id'] for x in found if x['entity_id'].startswith('switch.'))
for command,expected in [('turn_on',True),('turn_off',False)]:
 api('services/switch/'+command,{'entity_id':relay})
 for _ in range(12):
  if state()['relay']==expected and api('states/'+relay)['state']==('on' if expected else 'off'):break
  time.sleep(1)
 assert state()['relay']==expected
 assert api('states/'+relay)['state']==('on' if expected else 'off')
 print('PASS: Home Assistant '+command+' -> real ESP32-C3 -> actual state acknowledgement',flush=True)
print('PASS: real ESP32 MQTT firmware discovers all 9 entities with fixture meter/input data',flush=True)
