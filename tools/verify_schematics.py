"""Verify release schematic archive, shared interfaces and firmware GPIO constants."""
import collections,hashlib,json,pathlib,re,zipfile
ROOT=pathlib.Path(__file__).resolve().parents[1];DIR=ROOT/'hardware/schematics'
def components(name):return json.loads((DIR/(name+'.net.json')).read_text(encoding='utf-8'))['components']
def ref(cs,name):return next(c for c in cs.values() if c['props']['Designator']==name)
ctrl=components('controller-c3');main=components('socket-main')
signals={'4':'INB','5':'INA','6':'LIGHT','8':'RELAY_OUT','9':'SW_IN1','10':'SW_IN2','12':'METER_RX','13':'METER_TX','14':'KEY','16':'CF1','25':'USB_DM','26':'USB_DP','27':'UART0_RX','28':'UART0_TX','33':'GND'}
for pin,net in signals.items():assert ref(ctrl,'U60')['pinInfoMap'][pin]['net']==net
interface={'1':'METER_TX','2':'+5V','3':'SW_IN1','4':'CF1','5':'SW_IN2','6':'+3V3','7':'GND','8':'LIGHT','9':'KEY','10':'INA','11':'INB','12':'METER_RX'}
for pin,net in interface.items():assert ref(ctrl,'U2')['pinInfoMap'][pin]['net']==ref(main,'P2')['pinInfoMap'][pin]['net']==net
assert ref(main,'U3')['pinInfoMap']['3']['net']=='INA';assert ref(main,'U3')['pinInfoMap']['6']['net']=='INB'
assert ref(main,'U4')['pinInfoMap']['5']['net']=='HOT_GND'
assert all(p['net']!='HOT_GND' for c in ctrl.values() for p in c['pinInfoMap'].values())
pins=(ROOT/'firmware/include/pins.h').read_text()
for key,value in {'LATCH_INA_PIN':1,'LATCH_INB_PIN':0,'LED_PIN':2,'RELAY_PIN':3,'INPUT1_PIN':4,'INPUT2_PIN':5,'METER_RX_PIN':6,'METER_TX_PIN':7,'SETUP_PIN':8,'CF1_PIN':10}.items():assert re.search(r'\b'+key+r'\s*=\s*'+str(value)+r'\s*;',pins)
with zipfile.ZipFile(DIR/'metering-socket-schematics.epro2') as z:
 types=collections.Counter()
 for name in z.namelist():
  if not name.endswith('.epru'):continue
  for line in z.read(name).decode().splitlines():
   if line.startswith('{"type":"DOCHEAD"'):types[json.loads(line.split('||',1)[1].rstrip('|'))['docType']]+=1
 assert types['PCB']==types['PANEL']==0 and types['SCH_PAGE']==3
for line in (DIR/'SHA256SUMS.txt').read_text().splitlines():
 expected,name=line.split(maxsplit=1);assert hashlib.sha256((DIR/name).read_bytes()).hexdigest()==expected
print('PASS: native archive 3 schematic pages/zero PCB or PANEL; 15 chip nets, 12 board links, isolated grounds and firmware GPIO map; release hashes')
