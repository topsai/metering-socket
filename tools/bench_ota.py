"""Explicit bench OTA test; restores production and clears synthetic energy.

Requires private/bench/device.json and both already-built PIO environments.
Never prints credentials. Operates a bare board only; no mains/load connected.
"""
import json, pathlib, time, urllib.request, urllib.error
from bench_client import Bench, ROOT, PRIVATE

def upload(device, image, expected=200, token=None):
    boundary='MeteringSocketBenchBoundary'
    body=(f'--{boundary}\r\nContent-Disposition: form-data; name="firmware"; filename="firmware.bin"\r\nContent-Type: application/octet-stream\r\n\r\n'.encode()+image+f'\r\n--{boundary}--\r\n'.encode())
    request=urllib.request.Request(device['host']+'/api/ota',data=body,headers={'Authorization':'Bearer '+(device['token'] if token is None else token),'Content-Type':'multipart/form-data; boundary='+boundary})
    try:
        response=urllib.request.build_opener(urllib.request.ProxyHandler({})).open(request,timeout=60)
    except urllib.error.HTTPError as error:
        response=error
    assert response.code==expected,f'OTA HTTP {response.code}, expected {expected}'
    result=json.load(response)
    if expected==200:assert result.get('ok') and result.get('reboot')

def main():
    device=json.loads((PRIVATE/'device.json').read_text())
    upload(device,b'not-a-firmware',401,token='invalid')
    print('PASS: OTA wrong token rejected',flush=True)
    upload(device,b'not-a-firmware',400)
    print('PASS: OTA invalid image rejected',flush=True)
    upload(device,(ROOT/'firmware/.pio/build/esp32c3_bench/firmware.bin').read_bytes())
    time.sleep(5)
    b=Bench()
    try:
        d=b.discover();assert d['token']==device['token']
        b.usb('stream_off');b.usb('clear_energy')
        b.api('/api/config',{'rule':'manual','schedule_on':-1,'schedule_off':-1,'max_current':10,'max_power':2200})
        b.api('/api/relay',{'on':False})
        assert b.api()['energy']==0
    finally:b.close()
    print('PASS: bench OTA writes Flash, reboots and retains credentials; synthetic energy cleared',flush=True)
    upload(device,(ROOT/'firmware/.pio/build/esp32c3/firmware.bin').read_bytes())
    time.sleep(5)
    request=urllib.request.Request(device['host']+'/api/state',headers={'Authorization':'Bearer '+device['token']})
    state=json.load(urllib.request.build_opener(urllib.request.ProxyHandler({})).open(request,timeout=10))
    assert 'bench_mode' not in state and not state['relay'] and not state['meter_valid'] and state['energy']==0
    print('PASS: production OTA writes Flash, reboots, reconnects Wi-Fi; no bench_mode, no synthetic data, relay OFF',flush=True)
    (PRIVATE/'ota-results.json').write_text(json.dumps({'production_restored':True,'state':{k:state[k] for k in ['relay','meter_valid','energy','reason','mqtt_connected']}}))

if __name__=='__main__':main()
