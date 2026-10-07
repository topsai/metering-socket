# 构建与烧录

[返回首页](../README.md)

## 环境

| 项 | 版本/配置 |
|---|---|
| PIO平台 | espressif32 7.0.1 |
| Arduino ESP32 | 2.0.17（包3.20017） |
| 目标 | esp32-c3-devkitm-1参数，ESP32-C3裸芯片，4MB Flash |
| Flash/分区 | QIO / 80MHz / min_spiffs.csv双OTA |
| ArduinoJson / PubSubClient | 7.4.2 / 2.8 |
| JDK / Gradle / AGP | 17 / 8.13 / 8.13.2 |
| Android | min26，compile/target36，Build Tools36.0.0 |

开发板名仅用于构建参数，不要求购买开发板。具体裸芯片的Flash容量、接法和晶振必须匹配，否则修改构建配置重新验证。

## 固件

安装PlatformIO Core或VS Code PlatformIO扩展，在仓库根目录：

```powershell
pio run -d firmware
pio device list
pio run -d firmware -t upload --upload-port COM7
pio device monitor -d firmware --port COM7 --baud 115200
```

COM7替换为实际串口；Linux/macOS使用实际 `/dev/ttyACM0` 等设备路径。首次需要网络下载平台和库，`.pio`为忽略的缓存。

GPIO20/21接3.3V串口，或使用芯片USB Serial/JTAG，按硬件接法进入下载模式，见[PINS](PINS.md)。不使用5V串口逻辑。生成文件在 `firmware/.pio/build/esp32c3/`。

## 镜像和地址

| 地址 | 文件 |
|---:|---|
| 0x0000 | bootloader.bin |
| 0x8000 | partitions.bin |
| 0xe000 | boot_app0.bin |
| 0x10000 | firmware.bin |

`dist/meteringsocket-factory.bin`按上述地址合并，首次烧录起始0。优先PIO；手工使用已安装esptool：

```powershell
python -m esptool --chip esp32c3 --port COM7 write_flash 0x0 dist/meteringsocket-factory.bin
```

`dist/firmware.bin`只用于App/网页OTA，不能向空白芯片地址0烧录。factory镜像不能OTA。改变分区表需串口重烧；只更新应用通常保留NVS，擦除整片Flash会丢失Wi-Fi、令牌及累计电量。

修改源码后需重新构建、复制最新产物并更新哈希，仓库没有自动发布流程。合并地址见[flash-layout](../dist/flash-layout.txt)。

## Android

安装JDK17、SDK Platform36、Build Tools36.0.0、platform-tools。设置JAVA_HOME和ANDROID_HOME，或在忽略的 `android/local.properties` 中设置sdk.dir。

Windows：

```powershell
$env:JAVA_HOME='D:\Tools\jdk-17'
$env:ANDROID_HOME='D:\Android\Sdk'
.\tools\build-android.ps1
```

脚本的空变量默认值指向原开发环境用户目录，其他电脑显式设置环境变量。脚本运行Java测试、lint、assembleDebug并复制APK到dist。

Linux/macOS：

```sh
cd android
chmod +x gradlew
./gradlew :app:testDebugUnitTest :app:lintDebug :app:assembleDebug
```

Windows对应gradlew.bat。首次联网下载依赖；缓存齐全才可加 `--offline`。代理设置只放本机环境，不提交带凭据的代理地址。

应用ID `com.topsai.meteringsocket`，版本1.0.0。输出 `android/app/build/outputs/apk/debug/app-debug.apk`。debug keystore由本机工具生成，不在仓库；别的电脑重建可能无法覆盖安装旧APK。正式release签名尚未配置。

## 校验

```powershell
Get-FileHash dist/meteringsocket-debug.apk -Algorithm SHA256
Get-FileHash dist/firmware.bin -Algorithm SHA256
adb install -r dist/meteringsocket-debug.apk
```

对照[SHA256SUMS](../dist/SHA256SUMS.txt)。Linux执行 `cd dist && sha256sum -c SHA256SUMS.txt`。哈希不是签名，需同时确认下载来源。APK验签用SDK Build Tools的 `apksigner verify --verbose <APK路径>`。

常见构建问题见[排障](TROUBLESHOOTING.md)，开发测试见[TESTING](TESTING.md)。
