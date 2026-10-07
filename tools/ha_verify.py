"""Runs inside existing HA container: configure MQTT and verify simulated device.
No physical relay is controlled; simulated discovery entities are removed afterwards.
"""
import json, sys, time, threading, urllib.request, urllib.parse
import paho.mqtt.client as mqtt
creds=json.load(sys.stdin)
records=json.load(open('/config/.storage/auth'))['data']['refresh_tokens']
refresh=next(x for x in records if x.get('token_type')=='normal')
body=urllib.parse.urlencode({'grant_type':'refresh_token','refresh_token':refresh['token'],'client_id':refresh['client_id']}).encode()
access=json.load(urllib.request.urlopen(urllib.request.Request('http://localhost:8123/auth/token',data=body)))['access_token']
def api(path,body=None):
    req=urllib.request.Request('http://localhost:8123/api/'+path,data=None if body is None else json.dumps(body).encode(),headers={'Authorization':'Bearer '+access,'Content-Type':'application/json'})
    return json.load(urllib.request.urlopen(req,timeout=10))
entries=api('config/config_entries/entry')
if not any(e['domain']=='mqtt' for e in entries):
    flow=api('config/config_entries/flow',{'handler':'mqtt','show_advanced_options':False})
    print('MQTT configuration step:',flow.get('step_id'),[(x.get('name'),x.get('required')) for x in flow.get('data_schema',[])])
    step=api('config/config_entries/flow/'+flow['flow_id'],{'broker':'meteringsocket-mqtt','port':1883,'protocol':'3.1.1','other_settings':{'set_client_cert':False,'set_ca_cert':'off','transport':'tcp'},'username':creds['username'],'password':creds['password']})
    assert step['type']=='create_entry',step.get('errors')
    print('PASS: existing Home Assistant MQTT integration configured')
client=mqtt.Client(mqtt.CallbackAPIVersion.VERSION2,client_id='meteringsocket-verification')
client.username_pw_set(creds['username'],creds['password'])
ready=threading.Event();received=threading.Event()
state={'relay':False,'input1':True,'input2':False,'voltage':230.0,'current':0.25,'power':57.5,'energy':0.125,'frequency':50.0,'fault':False}
base='meteringsocket/ms_test'
def connect(c,u,flags,code,properties):
    assert code==0
    c.subscribe(base+'/relay/set');ready.set()
def message(c,u,msg):
    if msg.payload==b'ON':state['relay']=True;c.publish(base+'/state',json.dumps(state),retain=True);received.set()
client.on_connect=connect;client.on_message=message;client.connect('meteringsocket-mqtt',1883);client.loop_start();assert ready.wait(10)
topics=[]
try:
    for key in state:
        component='switch' if key=='relay' else 'binary_sensor' if key in ('input1','input2','fault') else 'sensor'
        d={'name':key,'unique_id':'ms_test_'+key,'state_topic':base+'/state','availability_topic':base+'/availability','value_template':'{{ value_json.'+key+' }}','device':{'identifiers':['ms_test'],'name':'计量插座 软件验收'}}
        if component=='switch':d.update(command_topic=base+'/relay/set',payload_on='ON',payload_off='OFF',state_on='True',state_off='False')
        elif component=='binary_sensor':d.update(payload_on='True',payload_off='False')
        else:
            unit,dc={'voltage':('V','voltage'),'current':('A','current'),'power':('W','power'),'energy':('kWh','energy'),'frequency':('Hz','frequency')}[key]
            d.update(unit_of_measurement=unit,device_class=dc,state_class='total_increasing' if key=='energy' else 'measurement',expire_after=15)
        topic=f'homeassistant/{component}/ms_test/{key}/config';topics.append(topic);client.publish(topic,json.dumps(d),retain=True).wait_for_publish()
    client.publish(base+'/availability','online',retain=True).wait_for_publish()
    client.publish(base+'/state',json.dumps(state),retain=True).wait_for_publish()
    found=[]
    for _ in range(30):
        all_states=api('states');found=[e for e in all_states if e.get('attributes',{}).get('friendly_name','').startswith('计量插座 软件验收')]
        if len(found)==9 and all(e['state'] not in ('unknown','unavailable') for e in found):break
        time.sleep(1)
    assert len(found)==9,[(e['entity_id'],e['state']) for e in found]
    assert all(e['state'] not in ('unknown','unavailable') for e in found)
    relay=next(e['entity_id'] for e in found if e['entity_id'].startswith('switch.'))
    api('services/switch/turn_on',{'entity_id':relay});assert received.wait(10)
    for _ in range(10):
        if api('states/'+relay)['state']=='on':break
        time.sleep(1)
    assert api('states/'+relay)['state']=='on'
    print('PASS: 9 discovery entities, measurement + boolean states, HA relay command and state acknowledgement')
    client.publish(base+'/availability','offline',retain=True).wait_for_publish();time.sleep(2)
    assert api('states/'+relay)['state']=='unavailable'
    print('PASS: offline availability')
finally:
    for topic in topics:client.publish(topic,'',retain=True).wait_for_publish()
    for suffix in ('state','availability'):client.publish(base+'/'+suffix,'',retain=True).wait_for_publish()
    client.loop_stop();client.disconnect()
