$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
if(!$env:JAVA_HOME){$env:JAVA_HOME=Join-Path $env:USERPROFILE 'Documents\Codex\AndroidTools\jdk-17.0.20.1+1'}
if(!$env:ANDROID_HOME){$env:ANDROID_HOME=Join-Path $env:LOCALAPPDATA 'Android\Sdk'}
$env:Path="$env:JAVA_HOME\bin;$env:Path"
Push-Location (Join-Path $projectRoot 'android')
try {& .\gradlew.bat :app:testDebugUnitTest :app:lintDebug :app:assembleDebug;if($LASTEXITCODE -ne 0){throw 'Android build failed'};Copy-Item -LiteralPath 'app\build\outputs\apk\debug\app-debug.apk' -Destination (Join-Path $projectRoot 'dist\meteringsocket-debug.apk')} finally {Pop-Location}
