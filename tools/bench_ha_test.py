"""Bare board HA/MQTT tests with USB inputs and independent restoration steps."""
import json, subprocess, sys, time
from bench_client import Bench, ROOT, PRIVATE
from bench_ota import upload
from bench_restore import restore

def verify(device,mode='control',**values):
 payload=dict(device,mode=mode,**values)
 r=subprocess.run(['docker','exec','-i','homeassistant','python','/tmp/meteringsocket_hardware_verify.py'],input=json.dumps(payload),text=True,capture_output=True)
 print(r.stdout,flush=True)
 if r.returncode:
  print(r.stderr,flush=True)
  raise RuntimeError('HA hardware test failed: '+mode)

def main():
 device=json.loads((PRIVATE/'device.json').read_text())
 subprocess.run(['docker','cp',str(ROOT/'tools/ha_hardware_verify.py'),'homeassistant:/tmp/meteringsocket_hardware_verify.py'],check=True)
 b=Bench();failure=None
 try:
  b.discover();b.meter();b.inputs(True,False);b.api('/api/config',{'rule':'manual','schedule_on':-1,'schedule_off':-1});b.api('/api/reset',{})
  verify(device)
  b.inputs(False,True);b.api('/api/config',{'rule':'both'});b.api('/api/reset',{});verify(device,'blocked',reason='interlock')
  b.inputs(True,True);assert not b.api()['relay'];verify(device)
  b.api('/api/config',{'rule':'either_follow'});b.api('/api/reset',{});verify(device,'follow')
  b.meter(current=12);time.sleep(.1);verify(device,'blocked',reason='overload')
  b.meter();b.api('/api/config',{'rule':'manual'});b.api('/api/reset',{});b.api('/api/relay',{'on':False})
  # Abrupt radio loss without MQTT DISCONNECT must trigger the actual will.
  b.usb('wifi_off');verify(device,'offline')
  b.usb('wifi_on');b.provision();b.meter();b.api('/api/reset',{});verify(device)
  print('PASS: real ESP32 Wi-Fi/MQTT reconnect and discovery recovery',flush=True)
 except Exception as error:failure=error
 finally:
  actions=[('wifi_on',lambda:b.usb('wifi_on')),('wifi_restore',b.provision),('stream_off',lambda:b.usb('stream_off')),('clear_energy',lambda:b.usb('clear_energy')),('config_restore',lambda:b.api('/api/config',{'rule':'manual','schedule_on':-1,'schedule_off':-1,'max_current':10,'max_power':2200})),('relay_off',lambda:b.api('/api/relay',{'on':False})),('usb_close',b.close)]
  def production_ota():
   upload(device,(ROOT/'firmware/.pio/build/esp32c3/firmware.bin').read_bytes());time.sleep(6)
  def serial_restore():
   subprocess.run([sys.executable,'-m','platformio','run','-d',str(ROOT/'firmware'),'-e','esp32c3','-t','upload','--upload-port','COM3'],check=True)
  errors=restore(actions,production_ota,lambda:verify(device,'production'),serial_restore)
  subprocess.run(['docker','exec','homeassistant','rm','-f','/tmp/meteringsocket_hardware_verify.py'],check=True)
  for message in errors:print('RESTORATION ERROR: '+message,flush=True)
 if failure is not None:raise failure
 if errors:raise RuntimeError('Restoration steps reported failures; inspect board before use')

if __name__=='__main__':main()
