"""Create an authenticated local MQTT broker; never print its password."""
import json, pathlib, secrets, subprocess
ROOT = pathlib.Path(__file__).resolve().parents[1]
PRIVATE = ROOT / 'private'
PRIVATE.mkdir(exist_ok=True)
credsfile = PRIVATE / 'mqtt.json'
if not credsfile.exists():
    credsfile.write_text(json.dumps({'username':'meteringsocket','password':secrets.token_urlsafe(24)}), encoding='utf-8')
creds = json.loads(credsfile.read_text(encoding='utf-8'))
config = PRIVATE / 'mosquitto'
config.mkdir(exist_ok=True)
pw = config / 'passwords'
if not pw.exists():
    pw.write_text(creds['username']+':'+creds['password']+'\n',encoding='utf-8')
    subprocess.run(['docker','run','--rm','-v',str(config)+':/work','eclipse-mosquitto:2','mosquitto_passwd','-U','/work/passwords'],check=True)
(config / 'mosquitto.conf').write_text('listener 1883\nallow_anonymous false\npassword_file /mosquitto/config/passwords\npersistence true\npersistence_location /mosquitto/data/\n',encoding='utf-8')
def exists(kind,name):return subprocess.run(['docker',kind,'inspect',name],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode==0
if not exists('network','meteringsocket'):
    subprocess.run(['docker','network','create','meteringsocket'],check=True,stdout=subprocess.DEVNULL)
if not exists('container','meteringsocket-mqtt'):
    subprocess.run(['docker','run','-d','--name','meteringsocket-mqtt','--restart','unless-stopped','--network','meteringsocket','-p','1883:1883','-v',str(config)+':/mosquitto/config:ro','-v','meteringsocket_mqtt_data:/mosquitto/data','eclipse-mosquitto:2'],check=True,stdout=subprocess.DEVNULL)
else:subprocess.run(['docker','start','meteringsocket-mqtt'],check=True,stdout=subprocess.DEVNULL)
networks=json.loads(subprocess.check_output(['docker','inspect','homeassistant']))[0]['NetworkSettings']['Networks']
if 'meteringsocket' not in networks:subprocess.run(['docker','network','connect','meteringsocket','homeassistant'],check=True)
subprocess.run(['docker','cp',str(ROOT/'tools'/'ha_verify.py'),'homeassistant:/tmp/meteringsocket_ha_verify.py'],check=True)
result=subprocess.run(['docker','exec','-i','homeassistant','python','/tmp/meteringsocket_ha_verify.py'],input=json.dumps(creds),text=True,capture_output=True)
print(result.stdout)
if result.returncode:print(result.stderr);raise SystemExit(result.returncode)
print('Broker ready on port 1883. Credentials saved privately in private/mqtt.json.')
