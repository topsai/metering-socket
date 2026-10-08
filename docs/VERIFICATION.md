# 验证记录 · 2026-10-08

裸ESP32-C3最新联测见[实板记录](HARDWARE_BENCH_2026-10-08.md)：实际烧录、Wi-Fi/HTTP、测试注入下联动与保护、NVS重启和真实Flash OTA通过，正式固件已恢复。安卓经ADB/HTTP转发控制实板通过。真实MQTT设备连接未通过，保留失败项。

此前构建与模拟验证：

- PlatformIO espressif32 7.0.1 / Arduino 2.0.17 编译成功，ESP32-C3，4MB Flash。最新正式应用829836字节，RAM41924字节。
- 原生 C++ 核心测试：启动关闭、AND允许/跟随、条件恢复不自启、超载与复位、暂停和真实故障独立、倒计时不清故障、millis回卷、35ms去抖、BL0942帧长度/校验/24位符号、能量计数回卷/回退。
- 用与固件相同的 ArduinoJson7 运行配置校验测试：字符串/布尔值不能替代数字，保护范围、校准、规则、时间、端口、Wi-Fi类型验证通过。
- Gradle :app:testDebugUnitTest（3个Java单元测试）、:app:lintDebug（0错误、6个样式/备份策略等警告）、:app:assembleDebug 成功。Android8兼容的有界读流代替readAllBytes。
- Android35模拟器安装并执行1个集成测试：App真实开启/关闭按钮调用HTTP；两路输入条件不满足时拒绝输出；错误令牌拒绝；App OTA使用有Content-Length的multipart上传，模拟端确认收到完整边界；退出界面禁用控制。模拟服务的继电器逻辑运行固件C++核心，没有真实继电器。
- 本机已有Home Assistant Docker容器启动；新增认证Mosquitto及MQTT集成。用模拟发布器测试9个实体自动发现、读数和bool状态、HA ON命令到模拟设备再回传ON、offline→unavailable。测试实体已移除，真实设备尚未上线。这不是在ESP32上执行MQTT固件。
- APK v2签名验证通过。首次烧录合并镜像生成成功，offset0。
- 独立只读代码审查发现并修复：网络阻塞安全路径、两种OTA传输格式、保护阈值字符串绕过、跟随关闭与每日关闭、Android热点网络选择、暂停不能替代真实故障、安全任务创建失败关闭。复查无剩余指定Important问题。

尚未实测：自焊ESP32-C3、真实BL0942串口响应及校准、真实继电器电气和上电复位瞬态、热点实机配网、突然断电电量保存、NTP获取、实际Android8手机及实际MQTT链路。需要画板并焊接后验证，不能用注入数据替代真实计量验收。

`dist/android-preview.png`为Android35模拟设备数据截图。
