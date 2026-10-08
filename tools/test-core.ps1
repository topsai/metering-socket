$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$nativeCompiler=Join-Path $env:USERPROFILE '.platformio\packages\toolchain-gccmingw32\bin\g++.exe'
Push-Location $projectRoot
try {
 foreach($test in @('core_test','hardware_test','key_test','core_bridge','config_test')){
  & $nativeCompiler -std=c++11 -static -I firmware/include -I firmware/.pio/libdeps/esp32c3/ArduinoJson/src "tests/$test.cpp" -o "tests/$test.exe"
  if($LASTEXITCODE -ne 0){throw "Compile failed: $test"}
  if($test -ne 'core_bridge'){& ".\tests\$test.exe";if($LASTEXITCODE -ne 0){throw "Test failed: $test"}}
 }
} finally {Pop-Location}
