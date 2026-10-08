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

覆盖Endpoint和有界Streams，共三个单元用例。报告在 `android/app/build/reports/`。minSdk26静态API检查不等于Android8实机验收。

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
adb -s emulator-5554 shell am instrument -w com.topsai.meteringsocket.test/android.test.InstrumentationTestRunner
```

替换模拟器编号。1个集成用例检查实际UI开关按钮、AND条件、401、固定长度multipart及后台禁用控制，结束恢复原有Vault内容。模拟器通过10.0.2.2访问主机服务。固定32位演示token只用于测试，不在真实固件中。

模拟服务执行共享C++核心，但计量数值为演示数据，配置/网络/OTA响应为测试夹具；它不模拟所有真实协议行为，不能替代芯片UART和写Flash验收。

## HA模拟联测

前提和副作用见[HA文档](HOME_ASSISTANT.md)。执行 `python tools/bootstrap_mqtt.py`；验证九实体、ON命令状态回传和离线，随后清理模拟实体。模拟发布器运行Python载荷，不是ESP32执行MQTT固件，因此只证明对接约定和HA能接受该结构。

## 实板验收顺序

已完成的裸开发板测试、独立bench环境、脚本使用边界和失败项见[2026-10-08实板记录](HARDWARE_BENCH_2026-10-08.md)。默认构建不包含测试注入。

没有实物时不把以下项目标记通过。先使用隔离安全的低压测试环境验证MCU/继电器模块，再由具备市电测试条件的人完成计量端验收。

| 项目 | 期望 |
|---|---|
| 复位/烧录/供电不稳定 | GPIO3外部下拉保持继电器关闭 |
| BL0942正确接线 | 4800/8N1有效帧，单位和标准表吻合 |
| UART断线/停帧 | 五秒后关闭并锁定，恢复帧不自动重启 |
| 两开关与去抖 | NO闭合true；允许条件失效关闭且撤销请求 |
| 允许/跟随规则 | 七模式分别核对，OFF暂停跟随，复位才恢复 |
| 过载 | 真实故障保留，OFF/定时/输入变化不能自动解除 |
| 网络阻塞/无broker | 输入释放及计量截止仍关闭；记录实测响应时间 |
| 热点 | 长按三秒暂停输出，配对只允许热点，十分钟关闭 |
| 每日/跨午夜/同时刻 | 校时后执行，OFF优先，真实故障不能被ON清除 |
| 倒计时 | 到期关闭；跨millis回卷逻辑及重新上电取消 |
| OTA成功/失败/中断 | 输出关闭，重启应用有效，不错误恢复继电器 |
| 断电电量 | NVS保存损失范围及首帧基线符合实际需要 |
| HA | 真设备发现、命令约束、断网遗嘱、重连发现 |
| 手机 | Android8+实机配网、后台/前台、旋转、签名升级 |

将板卡版本、器件型号、负载、标准表、软件commit、日期及结果记录到新的验收报告，保留失败项，不覆盖现有模拟验证记录。
