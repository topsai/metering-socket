# 测试与验收

[返回首页](../README.md) · [已有验证记录](VERIFICATION.md)

## 原生核心和配置

Windows先执行 `pio run -d firmware`，再：

```powershell
.\tools\test-core.ps1
```

脚本使用 `%USERPROFILE%\.platformio\packages\toolchain-gccmingw32\bin\g++.exe`，构建core_test/core_bridge/config_test；缺该工具时安装MinGW并改本机编译器路径。测试共享固件头文件，不是重新实现控制算法。

Linux/macOS可使用已安装g++：

```sh
g++ -std=c++11 -I firmware/include tests/core_test.cpp -o /tmp/core_test
/tmp/core_test
g++ -std=c++11 -I firmware/include -I firmware/.pio/libdeps/esp32c3/ArduinoJson/src tests/config_test.cpp -o /tmp/config_test
/tmp/config_test
```

断言失败退出非零。覆盖启动、允许/跟随、撤销请求、故障/暂停分离、timer/millis回卷、去抖、帧校验/符号、能量回卷/重置、配置类型和范围。

## Android单元和静态检查

```powershell
.\android\gradlew.bat -p android :app:testDebugUnitTest :app:lintDebug :app:assembleDebug
```

覆盖Endpoint和有界Streams，共三个单元用例。报告在 `android/app/build/reports/`。minSdk26静态API检查不等于Android 8.0（API 26）实机验收。

## Android模拟器集成

仅针对测试模拟器运行，不对已保存真实设备的手机运行。Windows流程：

1. 执行原生测试生成core_bridge.exe。
2. 一个终端运行 `python tools/device_simulator.py`，测试服务绑定127.0.0.1:8765；Ctrl+C停止。
3. 启动Android模拟器，建议API35，确认 `adb devices`。
4. 构建应用与测试APK：

```powershell
.\android\gradlew.bat -p android :app:assembleDebug :app:assembleDebugAndroidTest
adb -s emulator-5554 install -r android/app/build/outputs/apk/debug/app-debug.apk
adb -s emulator-5554 install -r android/app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb -s emulator-5554 shell am instrument -w -e class com.topsai.meteringsocket.PanelTest com.topsai.meteringsocket.test/android.test.InstrumentationTestRunner
```

替换模拟器编号。上述命令只执行 `PanelTest` 的一个模拟集成用例，检查实际 UI 开关按钮、AND 条件、401、固定长度 multipart 及后台禁用控制，结束恢复原有 Vault 内容。模拟器通过 10.0.2.2 访问主机服务。固定的 32 字符演示令牌只用于测试，不在真实固件中。

`HardwarePanelTest` 是另一个可选实板用例，需手动准备 App 私有目录内的设备配置；没有配置文件时该用例直接返回。若不指定测试类，测试运行器可能报告两个用例完成，但不能据此认定实板用例已执行。实板测试方法和范围见[实板记录](HARDWARE_BENCH_2026-10-08.md)。

模拟服务执行共享C++核心，但计量数值为演示数据，配置/网络/OTA响应为测试夹具；它不模拟所有真实协议行为，不能替代芯片UART和写Flash验收。

## HA模拟联测

前提和副作用见[HA文档](HOME_ASSISTANT.md)。执行 `python tools/bootstrap_mqtt.py`；旧版模拟流程验证九实体、ON命令状态回传和离线，随后清理模拟实体。此历史流程不能替代当前十三实体及双路发现验收。模拟发布器运行Python载荷，不是ESP32执行MQTT固件，因此只证明对接约定和HA能接受该结构。

## 实板验收顺序

已完成的裸开发板测试、独立bench环境、脚本使用边界和失败项见[2026-10-08实板记录](HARDWARE_BENCH_2026-10-08.md)。默认构建不包含测试注入。

没有实物时不把以下项目标记通过。先使用隔离安全的低压测试环境验证MCU/继电器模块，再由具备市电测试条件的人完成计量端验收。

| 项目 | 期望 |
|---|---|
| 复位/烧录/供电不稳定 | 测量GPIO0/1脉冲、GPIO3和两路触点；确认上电OFF脉冲；验证突然断电磁保持触点可保留状态 |
| 磁保持脉冲 | INA/INB不同时为HIGH、换向5ms死区、50~200ms范围及方向反转；检查busy/known/estimated只是估计 |
| 原有KEY/LIGHT | GPIO8低有效3秒配网，下载时松开；GPIO2低有效快闪/慢闪/常亮 |
| CF1 | GPIO10隔离后上升沿、运行计数和Hz；重启清零，不重复累计UART电量 |
| 双路独立性 | 各自规则/请求/复位/倒计时/每日定时，共享计量与输入；旧请求默认板载，external命令不改板载配置 |
| BL0942正确接线 | 4800/8N1有效帧，单位和标准表吻合 |
| UART断线/停帧 | 五秒后关闭并锁定，恢复帧不自动重启 |
| 两开关与去抖 | NO闭合true；允许条件失效关闭且撤销请求 |
| 允许/跟随规则 | 七模式分别核对；OFF暂停跟随，计量健康时解除锁定或每日ON可恢复暂停，真实故障需主动解除 |
| 过载 | 真实故障保留，OFF/定时/输入变化不能自动解除 |
| 网络阻塞/无broker | 输入释放及计量截止仍关闭；记录实测响应时间 |
| 热点 | 长按三秒暂停输出，配对只允许热点，十分钟关闭 |
| 每日/跨午夜/同时刻 | 校时后执行，OFF优先，真实故障不能被ON清除 |
| 倒计时 | 到期关闭；跨millis回卷逻辑及重新上电取消 |
| OTA成功/失败/中断 | 输出关闭，重启应用有效，不错误恢复继电器 |
| 断电电量 | NVS保存损失范围及首帧基线符合实际需要 |
| HA | 真设备十三实体发现、两路命令约束、CF1状态、断网遗嘱、重连发现 |
| 手机 | Android 8.0（API 26）及以上实机配网、后台/前台、旋转、签名升级 |

将板卡版本、器件型号、负载、标准表、软件commit、日期及结果记录到新的验收报告，保留失败项，不覆盖现有模拟验证记录。

## 当前双路测试入口

本次已运行的结果见[1.1.0 验证](DUAL_CHANNEL_VERIFICATION_2026-10-08.md)。原生检查运行 `tools/test-core.ps1`，原理图与引脚检查运行 `python tools/verify_schematics.py`。裸板完整测试为 `python tools/bench_dual_test.py`（需先烧录测试固件和本机凭据），补充测试为 `python tools/bench_dual_extra.py`（实际 OTA 切换测试／正式固件）。仅用于没有外围设备或市电的裸开发板，测试注入不是实物计量。

`tools/ha_dual_verify.py` 在已配置的 HA 容器内部使用 stdin 提供本机 device JSON，验证当前十三实体和两路开关。`mode=production` 时只验证未接计量的正式固件保持关闭。不要把凭据作为命令行参数公开。
