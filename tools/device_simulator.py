"""Local-only test fixture. Relay/interlock behavior executes the firmware C++ core.
No real electrical load is connected. Do not deploy this test server as firmware.
"""
from http.server import HTTPServer, BaseHTTPRequestHandler
import json, math, pathlib, subprocess, time
ROOT=pathlib.Path(__file__).resolve().parents[1]
TOKEN='0123456789abcdef0123456789abcdef'
process=subprocess.Popen([str(ROOT/'tests/core_bridge.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
externalProcess=subprocess.Popen([str(ROOT/'tests/core_bridge.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,text=True)
rules=['manual','input1','input2','both','either','both_follow','either_follow']
config={'rule':'manual','max_current':10,'max_power':2200,'schedule_on':-1,'schedule_off':-1,'vref':15883.34116,'iref':251065.6814,'pref':623.0270705,'broker':'','mqtt_port':1883,'mqtt_user':''}
externalConfig=dict(config)
started=time.monotonic();a=True;b=False;valid=True;ota_count=0
timerDeadlines={False:0.0,True:0.0}
def countdown(external,state):
    if state["fault"] or state["reason"]=="timer":timerDeadlines[external]=0.0
    return max(0,math.ceil(timerDeadlines[external]-time.monotonic()))
def core(cmd,external=False):
    proc=externalProcess if external else process;proc.stdin.write(cmd+'\n');proc.stdin.flush();return json.loads(proc.stdout.readline())
def state():
    d=core(f'tick {int((time.monotonic()-started)*1000)} {int(a)} {int(b)} {int(valid)} 0.25 57.5')
    e=core(f'tick {int((time.monotonic()-started)*1000)} {int(a)} {int(b)} {int(valid)} 0.25 57.5',True)
    e.update(externalConfig,countdown=countdown(True,e))
    return dict(d,external=e,cf1_count=42,cf1_frequency=2.5,led_on=True,latch_known=True,latch_busy=False,latch_estimated=d['relay'],latch_on_ina=True,latch_pulse_ms=100,id='ms_sim',name='计量插座 模拟设备',input1=a,input2=b,meter_valid=valid,voltage=230,current=0.25,power=57.5,frequency=50,energy=0.125,countdown=countdown(False,d),ota_count=ota_count,ip='10.0.2.2:8765',mqtt_connected=False,**config)
class Handler(BaseHTTPRequestHandler):
    def log_message(self,*args):pass
    def response(self,code,value):
        raw=json.dumps(value,ensure_ascii=False).encode();self.send_response(code);self.send_header('Content-Type','application/json');self.send_header('Content-Length',str(len(raw)));self.end_headers();self.wfile.write(raw)
    def do_GET(self):
        if self.path=='/api/pair':return self.response(200,{'id':'ms_sim','token':TOKEN})
        if self.headers.get('Authorization')!='Bearer '+TOKEN:return self.response(401,{'error':'unauthorized'})
        if self.path=='/api/state':return self.response(200,state())
        self.response(404,{'error':'not_found'})
    def do_POST(self):
        global a,b,valid,ota_count
        if self.headers.get('Authorization')!='Bearer '+TOKEN:return self.response(401,{'error':'unauthorized'})
        n=int(self.headers.get('Content-Length','0'))
        if n>2000000:return self.response(413,{'error':'too_large'})
        raw=self.rfile.read(n)
        if self.path=='/api/ota':
            ok=raw.startswith(b'--SocketFirmware\r\n') and raw.endswith(b'--SocketFirmware--\r\n')
            if ok:ota_count+=1
            return self.response(200 if ok else 400,{'ok':ok})
        try:d=json.loads(raw)
        except Exception:return self.response(400,{'error':'invalid_json'})
        channel=d.get('channel','onboard')
        if channel not in ('onboard','external'):return self.response(400,{'error':'unknown_channel'})
        external=channel=='external';selected=externalConfig if external else config
        if self.path=='/api/relay':
            if selected['rule'].endswith('follow'):return self.response(409,{'error':'follow_mode'})
            if not isinstance(d.get('on'),bool):return self.response(400,{'error':'boolean_required'})
            core('on '+str(int(d['on'])),external)
            if not d['on']:timerDeadlines[external]=0.0
        elif self.path=='/api/reset':core('reset',external)
        elif self.path=='/api/timer':
            seconds=d.get('seconds',0)
            core('timer '+str(seconds),external)
            timerDeadlines[external]=time.monotonic()+seconds if seconds>0 else 0.0
        elif self.path=='/api/config':
            if 'rule' in d:
                if d['rule'] not in rules:return self.response(400,{'error':'unknown_rule'})
                core('rule '+str(rules.index(d['rule'])),external)
                timerDeadlines[external]=0.0
            selected.update({k:v for k,v in d.items() if k in selected})
        elif self.path=='/test/inputs':a=d.get('input1',a);b=d.get('input2',b);valid=d.get('meter_valid',valid)
        else:return self.response(404,{'error':'not_found'})
        self.response(200,state())
if __name__=='__main__':
    print('C++ core simulator listening on 127.0.0.1:8765',flush=True)
    try:HTTPServer(('127.0.0.1',8765),Handler).serve_forever()
    finally:process.terminate();externalProcess.terminate()
