"""Independent restoration steps; error summaries never include private values."""
def restore(actions,upload,verify,serial_fallback):
 errors=[]
 for name,action in actions:
  try:action()
  except Exception as error:errors.append(name+': '+type(error).__name__)
 try:upload()
 except Exception as error:
  errors.append('production_ota: '+type(error).__name__)
  try:serial_fallback()
  except Exception as fallback_error:errors.append('production_serial: '+type(fallback_error).__name__)
 try:verify()
 except Exception as error:errors.append('production_verification: '+type(error).__name__)
 return errors
