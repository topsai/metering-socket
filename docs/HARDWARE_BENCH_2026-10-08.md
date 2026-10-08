# ESP32-C3 裸开发板联测 · 2026-10-08

[返回首页](../README.md) · [全部验证记录](VERIFICATION.md)

## 测试对象与边界

COM3 接入 ESP32-C3 QFN32 rev0.4、4MB XMC Flash、40MHz 晶振开发板，只有 USB 供电。没有接 BL0942、微动开关、继电器或负载，也未修改原理图。首次操作前完整备份原始 4MB Flash 到本机忽略目录。

先串口烧录正式环境，确认不存在 USB 测试指令；再烧录独立 `esp32c3_bench` 环境，通过 USB 注入输入状态和 BL0942 格式帧。注入帧经过与实际 UART 共用的 `acceptMeter` 和校验/单位换算，控制逻辑、安全任务、GPIO3 输出寄存器、HTTP、NVS 和 OTA 均运行在真实 ESP32 上。GPIO 寄存器回读不能代替引脚电压、继电器动作或市电计量验收。

## 已通过

| 项目 | 证据和范围 |
|---|---|
| 首次烧录和 Wi-Fi | esptool 写入并校验成功；连接指定 2.4GHz 网络，本机直接读取设备 HTTP 状态 |
| 无计量时安全关闭 | 启动缺帧时输出关闭，ON 无法绕过保护，复位返回409 |
| 认证和配置 | 错误令牌401，站点网络配对403；非法配置400且不部分写入 |
| 帧解析 | USB 格式帧通过校验与单位换算，损坏校验拒绝，负功率正确；未测试实际 UART 电气链路 |
| 七种联动 | 四组输入组合逐一测试，API状态与GPIO3寄存器一致；允许条件恢复不自动开启 |
| 跟随和保护 | OFF暂停跟随，复位恢复；注入过流/超功率后锁定，异常未恢复时不能复位 |
| 倒计时和计量超时 | 实时时钟倒计时关闭；停帧超过五秒关闭并锁定 |
| HTTP阻塞 | 不完整POST阻塞主循环六秒，计量超时仍由独立任务触发，释放请求后确认输出关闭、meter_stale锁定；未用仪器测量最坏截止延迟 |
| 每日定时 | 注入设备系统时间测试ON/OFF、次日恢复、同分钟OFF优先；未验证公网NTP获取 |
| NVS与重启 | 强制保存后重启，配置、令牌、电量保留，输出关闭；未测突然断电损失窗口 |
| Android控制实板 | Android35模拟器直接连接开发板LAN IP，真实App开启/关闭按钮和ESP32状态回传通过；没有ADB反向端口或HTTP转发，测试用例8.425秒 |
| HA与真实MQTT固件 | 九实体自动发现、HA ON/OFF到真实ESP32及状态回传；输入条件不满足/过流时不能开启，跟随OFF暂停且ON不能绕过 |
| 真实离线遗嘱和重连 | bench关闭ESP32无线电且不发送MQTT DISCONNECT；broker超时发布遗嘱，HA九实体unavailable；恢复Wi-Fi后重新发现并控制成功 |
| 正式固件HA保护 | 最终正式固件MQTT在线，未接BL0942时HA ON仍被拒绝，开关回传OFF、四种瞬时计量unknown |
| OTA真实写Flash | 错误令牌401、非法镜像400；有效bench固件写入、重启、保留凭据；随后OTA正式固件成功 |
| 结束状态 | 正式固件已恢复，无bench_mode；未接计量时读数无效、输出OFF；合成累计电量清零，Wi-Fi凭据保留 |

`tools/bench_test.py` 的22组结果保存在本机 `private/bench/logic-results.json`，OTA结果在 `private/bench/ota-results.json`。这些文件及凭据、原始Flash均不提交公开仓库。

![Android模拟器直接访问真实ESP32，数值和输入为USB注入](../dist/android-hardware-preview.png)

## 连接障碍的定位与修复

首次真实连接失败：设备 `mqtt_connected=false`，broker未收到设备连接。普通PowerShell进程没有管理员令牌，添加入站规则返回“拒绝访问”。随后通过 Windows 正常 `Start-Process -Verb RunAs` 提权，成功添加仅限LocalSubnet的TCP1883入站规则；ESP32随即连接，broker日志确认真实设备客户端上线。HA真实实体发现和完整联测现已通过。

Windows WLAN 属于Public网络；没有关闭防火墙或开放公网来源。该规则保留供本机HA正常使用。其他电脑如需要此规则，应在管理员PowerShell执行：

```powershell
New-NetFirewallRule -DisplayName 'MeteringSocket MQTT LAN' -Direction Inbound -Action Allow -Protocol TCP -LocalPort 1883 -RemoteAddress LocalSubnet -Profile Any
```

Docker须发布1883、broker启用认证，设备填写电脑实际LAN地址。规则不代表其他环境必然修复，仍需确认实际设备连接、发现和命令回传。

最初Android模拟器直接访问LAN超时，只经转发测试成功。清除模拟器启动进程继承的HTTP_PROXY/HTTPS_PROXY/ALL_PROXY并重新启动后，直接使用开发板IP执行App测试通过；这次没有转发通道。真实Android手机仍需单独验收。

HA脚本现等待实际 `mqtt_connected=true` 后发命令，避免配置保存后最长约十秒重连期间的命令丢失。遗嘱测试关闭无线电；正常主动MQTT DISCONNECT不会发布遗嘱，不能拿它代替异常断线。

恢复步骤彼此独立，任何一步失败仍尝试停止注入、清电量、关闭USB和正式OTA；OTA失败再尝试串口PIO恢复，最后验证正式状态。两个无硬件失败注入用例验证Wi-Fi恢复失败不能跳过OTA、OTA失败仍尝试串口恢复；本次完整HA脚本退出码0、恢复验证通过。

## 待验证

还需实测：真实BL0942串口与精度、隔离通信、两开关机械去抖、GPIO电压和复位瞬态、继电器带载、电量突然断电保存、真实手机热点配网和Wi-Fi直连、实际Android8兼容、NTP获取。

## 复现测试

仅对裸低压测试板运行，bench环境会合成计量并可能拉高GPIO3。不要连接继电器、负载或市电。正式构建默认只选择 `esp32c3`；bench环境必须显式指定：

```powershell
pio run -d firmware -e esp32c3_bench -t upload --upload-port COM3
```

`tools/bench_client.py` 使用pyserial；凭据由本机 `private/bench/network.json`、`private/mqtt.json` 提供，USB读取的设备令牌写入忽略目录 `private/bench/device.json`，不打印。`tools/bench_test.py` 执行实板逻辑测试；`tools/bench_ota.py` 要求已构建两种环境，测试OTA后恢复正式固件并清除合成电量。中途失败时务必串口重烧正式环境，并确认状态不含bench_mode、输出关闭。

`HardwarePanelTest` 只有在App私有目录存在 `bench-device.json` 且目标为bench固件时执行实板控制，否则跳过；默认运行的“OK”不能作为真实设备测试证据。

`tools/bench_ha_test.py` 需现有HA容器、认证broker和可访问的LAN，执行真实发现、控制约束、关闭无线电的遗嘱和重连，最后恢复正式固件。`ha_hardware_verify.py` 在HA容器执行各阶段断言，本次完整脚本通过。恢复错误会使测试失败，并报告步骤名；需要核对板卡最终状态，不能只看前面的PASS行。

恢复流程无硬件测试：`python tests/bench_restore_test.py`。Python环境需安装pyserial才能使用其他USB bench脚本。
