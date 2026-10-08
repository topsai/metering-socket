"""USB bench client. Credentials remain in private/bench, never stdout."""
import json, pathlib, time, urllib.request, urllib.error
import serial
ROOT=pathlib.Path(__file__).resolve().parents[1]
PRIVATE=ROOT/'private'/'bench'
class Bench:
 def __init__(self, port='COM3'):
  self.serial=serial.Serial(port,115200,timeout=0.2);self.serial.dtr=True;self.serial.rts=False;time.sleep(0.5);self.serial.reset_input_buffer();self.device=None
 def close(self):self.serial.close()
 def usb(self,op='status',**values):
  self.serial.write((json.dumps(dict(op=op,**values))+'\n').encode());deadline=time.monotonic()+5
  while time.monotonic()<deadline:
   line=self.serial.readline()
   try:result=json.loads(line)
   except (ValueError,UnicodeDecodeError):continue
   if isinstance(result,dict):return result
  raise TimeoutError('No bench JSON response on USB')
 def discover(self):
  d=self.usb();assert d.get('bench_mode') is True,'Not bench firmware';self.device={'host':'http://'+d['ip'],'token':d['token'],'id':d['id']};return d
 def api(self,path='/api/state',body=None,expected=200,token=None):
  req=urllib.request.Request(self.device['host']+path,data=None if body is None else json.dumps(body).encode(),headers={'Authorization':'Bearer '+(self.device['token'] if token is None else token),'Content-Type':'application/json'})
  try:
   with urllib.request.build_opener(urllib.request.ProxyHandler({})).open(req,timeout=10) as response:status=response.status;value=response.read()
  except urllib.error.HTTPError as error:status=error.code;value=error.read()
  assert status==expected,f'{path}: HTTP {status}, expected {expected}'
  return json.loads(value)
 def meter(self,current=.25,voltage=230,power=57.5,count=0,stream=True,corrupt=False):
  frame=bytearray(23);frame[0]=0x55
  for start,number in ((1,round(current*251065.6814)),(4,round(voltage*15883.34116)),(10,round(power*623.0270705)),(13,count)):
   frame[start:start+3]=(number&0xffffff).to_bytes(3,'little')
  frame[16:18]=(20000).to_bytes(2,'little');frame[22]=((0x58+sum(frame[:22]))&255)^255
  if corrupt:frame[22]^=1
  return self.usb('meter',frame=frame.hex(),stream=stream)
 def inputs(self,a,b):self.usb('inputs',input1=a,input2=b);time.sleep(.08)
 def provision(self):
  network=json.loads((PRIVATE/'network.json').read_text(encoding='utf-8-sig'));mqtt=json.loads((ROOT/'private'/'mqtt.json').read_text(encoding='utf-8-sig'));network.update(mqtt_user=mqtt['username'],mqtt_password=mqtt['password']);self.usb('provision',**network)
  for _ in range(40):
   d=self.discover()
   if d.get('wifi_connected') and d['ip']!='0.0.0.0':
    (PRIVATE/'device.json').write_text(json.dumps(self.device),encoding='utf-8');return d
   time.sleep(1)
  raise TimeoutError('Station did not connect')
if __name__=='__main__':
 b=Bench()
 try:
  d=b.discover();print('PASS: USB bench, startup relay',d['gpio_relay'],'meter_valid',d['meter_valid'])
  d=b.provision();print('PASS: Wi-Fi station',d['ip'],'device',d['id']);s=b.api();print('PASS: live HTTP, meter_valid',s['meter_valid'],'relay',s['relay'])
 finally:b.close()
