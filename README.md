# 计量插座 · MeteringSocket

基于 **ESP32-C3 + BL0942** 的计量插座软件：PlatformIO Arduino 固件、原生中文 Android App，以及可选的 Home Assistant MQTT 接入。

手机与设备在同一局域网、能够互相访问时，可以通过 HTTP 直接控制设备，**在没有 MQTT 服务的情况下也可以使用**。配置 MQTT 后，还可以接入 Home Assistant（HA），两种方式可以同时启用。

使用 HA 接入时，HA 和 MQTT broker（消息服务器）需要保持运行，可部署在电脑、NAS 或其他主机上。离家使用需要预先配置 HA 的安全远程入口或家庭 VPN。

> 当前版本 **1.1.0**。固件、安卓 App、网页和 Home Assistant 已同步板载磁保持继电器、外接继电器、原灯／按键及 CF1。裸 ESP32-C3 已验证双路接口与 GPIO 驱动、注入联动／保护、定时、NVS、真实 Flash OTA；安卓模拟器直连实板和 HA 十三实体双路控制通过。未接真实 BL0942、继电器或市电负载，详见[本次验证](docs/DUAL_CHANNEL_VERIFICATION_2026-10-08.md)。原理图工程包见[硬件文件](hardware/schematics/README.md)，不含 PCB；主体既有 DRC 错误仍待定位。

## 功能

- 板载磁保持K1与GPIO3外接普通继电器独立控制，共享两路按下为 `true` 的微动开关及同一计量；没有触点反馈，外接继电器不单独计量。
- 电压、电流、有功功率、频率、累计电量；失联显示无效读数。
- 手动、单路允许、AND/OR允许、AND/OR自动跟随，共七种规则。
- 关闭倒计时、每日定时、过流/功率保护，故障与暂停分别管理。
- 原生中文Android 8.0（API 26）及以上 App：多设备、配对、计量、控制、设置、OTA。
- 热点配网、令牌认证HTTP API、简易浏览器控制页。
- MQTT Home Assistant十三实体自动发现、在线状态和离线遗嘱。
- 配置和电量周期保存、双OTA分区、独立安全控制任务。

## 下载与开始

- [安卓 APK（调试签名）](dist/meteringsocket-debug.apk)
- [首次烧录整片镜像](dist/meteringsocket-factory.bin)
- [OTA 应用固件](dist/firmware.bin)
- [SHA-256 校验值](dist/SHA256SUMS.txt)
- [原理图工程包](hardware/schematics/metering-socket-schematics.epro2)（不含 PCB）

GitHub文件页选择下载原始文件；不要将首次烧录镜像上传到OTA。

1. 按[接线定义](docs/PINS.md)准备ESP32-C3，按[构建与烧录](docs/BUILD.md)烧录。
2. 手机接入 `MeteringSocket-…`，密码 `socketsetup`；重新配网长按GPIO8原有KEY按钮三秒。
3. 安装APK，添加 `192.168.4.1`，点“从热点首次配对”，保存设备。
4. App填写家庭2.4GHz Wi-Fi；手机切回家庭Wi-Fi，将当前设备地址更新为路由器分配的IP。
5. 计量有效后解除锁定；手动模式再开启，跟随模式按输入自动输出。

详见[用户手册](docs/USER_GUIDE.md)。热点开放十分钟；令牌只允许在配网模式通过设备热点读取。

[首版界面截图](dist/android-preview.png)仅为历史模拟数据界面；当前双路界面以 1.1.0 APK 为准。

## 引脚速查

| 信号 | GPIO | 默认行为 |
|---|---:|---|
| 板载K1 INB / INA | 0 / 1 | CN8023B限时脉冲驱动磁保持继电器 |
| 外接普通继电器 | 3 | HIGH开启，LOW关闭 |
| LIGHT | 2 | LOW点亮原有LED |
| 微动开关1 / 2 | 4 / 5 | 上拉，闭合到CTRL_GND为true |
| BL0942 UART RX / TX | 6 / 7 | 4800bps、8N1，经隔离通信 |
| BL0942 CF1 | 10 | 隔离后上升沿计数及脉冲频率 |
| 原有KEY | 8 | LOW有效，长按三秒配网；ROM下载时松开 |

GPIO号不是封装脚号。USB18/19、下载串口20/21、启动和Flash脚保留。裸芯片要求、继电器输入下拉及隔离边界见[PINS.md](docs/PINS.md)。

## 文档

| 文档 | 内容 |
|---|---|
| [用户手册](docs/USER_GUIDE.md) | 安装、配对、联动、定时、保护、校准、升级 |
| [GPIO与接线](docs/PINS.md) | 引脚、隔离信号、裸芯片、复位电平 |
| [构建与烧录](docs/BUILD.md) | 环境、PIO、Android、整片/OTA镜像、校验 |
| [Home Assistant](docs/HOME_ASSISTANT.md) | MQTT、Docker、自动发现、远程及联测 |
| [HTTP / MQTT API](docs/API.md) | 接口、字段、错误、示例、主题 |
| [软件架构](docs/ARCHITECTURE.md) | 控制状态、任务、计量、持久化 |
| [故障排查](docs/TROUBLESHOOTING.md) | 配网、联动、计量、MQTT、OTA、构建 |
| [测试与验收](docs/TESTING.md) | 原生测试、安卓模拟器、HA、实板步骤 |
| [验证记录](docs/VERIFICATION.md) | 已运行检查与未实测项 |
| [裸开发板联测](docs/HARDWARE_BENCH_2026-10-08.md) | 实际烧录、测试注入、OTA及验收边界 |
| [文档审核记录](docs/DOCUMENTATION_REVIEW.md) | 连接方式、接口约定、术语与验证范围的修订 |
| [安全说明](SECURITY.md) | 凭据、网络、OTA、公开提交检查 |
| [参与开发](CONTRIBUTING.md) | 修改与验证要求 |
| [版本记录](CHANGELOG.md) | 内容与限制 |
| [第三方说明](THIRD_PARTY_NOTICES.md) | 依赖、许可、协议参考 |

## 开发

```powershell
git clone https://github.com/topsai/metering-socket.git
cd metering-socket
pio run -d firmware
.\tools\build-android.ps1
.\tools\test-core.ps1
```

Android需要JDK17、SDK36 / Build Tools36，详见[BUILD.md](docs/BUILD.md)。原生测试使用PlatformIO MinGW；先构建固件取得ArduinoJson依赖。

```text
firmware/   PIO Arduino固件、共享控制核心和校验
android/    App、Gradle wrapper、单元与模拟器测试
tests/      原生C++测试和模拟桥接
tools/      构建、测试、模拟、公开内容检查、HA验证
hardware/schematics/  原生原理图工程包、源码及网表（不含PCB）
docs/       使用、开发与验收文档
dist/       APK、固件、校验值和模拟界面截图
private/    本机凭据和broker配置，脚本首次生成，禁止提交
```

原创代码暂未指定开源许可；第三方组件按各自许可。公开可阅读源码，复用授权另行确定，见[第三方说明](THIRD_PARTY_NOTICES.md)。
