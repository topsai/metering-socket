"""No hardware: restoration must continue after each failed cleanup operation."""
import pathlib, sys, unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from bench_restore import restore

class RestoreTests(unittest.TestCase):
 def test_wifi_failure_cannot_skip_flash_restore(self):
  calls=[]
  def wifi():calls.append('wifi');raise TimeoutError('private details must not appear in result')
  actions=[('wifi',wifi),('clear',lambda:calls.append('clear')),('close',lambda:calls.append('close'))]
  errors=restore(actions,lambda:calls.append('ota'),lambda:calls.append('verify'),lambda:calls.append('serial'))
  self.assertEqual(calls,['wifi','clear','close','ota','verify'])
  self.assertEqual(errors,['wifi: TimeoutError'])
 def test_ota_failure_attempts_serial_then_verifies(self):
  calls=[]
  def ota():calls.append('ota');raise ConnectionError('offline')
  errors=restore([('close',lambda:calls.append('close'))],ota,lambda:calls.append('verify'),lambda:calls.append('serial'))
  self.assertEqual(calls,['close','ota','serial','verify'])
  self.assertEqual(errors,['production_ota: ConnectionError'])

if __name__=='__main__':unittest.main()
