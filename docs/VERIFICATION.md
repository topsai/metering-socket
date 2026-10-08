# 验证记录 · 2026-10-08

当前 1.1.0 双路验证见[本次同步记录](DUAL_CHANNEL_VERIFICATION_2026-10-08.md)。
> 历史记录：以下结果属于本次原理图同步前的单路/GPIO0配网/九实体版本。不能作为现有GPIO8 KEY、磁保持双路、CF1或十三实体版本的实测证据；本次新增验证结果应另行记录。
同步前裸ESP32-C3联测见[实板记录](HARDWARE_BENCH_2026-10-08.md)：实际烧录、Wi-Fi/HTTP、测试注入下联动与保护、NVS重启和真实Flash OTA通过。正常管理员提权添加LocalSubnet-only TCP1883入站规则后，真实MQTT九实体发现、HA开关/约束、遗嘱和重连通过。安卓模拟器直接访问开发板IP控制通过。当时正式固件已恢复、MQTT在线，未接计量时HA ON不能开启，四种瞬时读数unknown。恢复流程两个失败注入用例通过。

首版阶段的构建与模拟验证（以下描述的是当时结果；当时实板结果见上文）：

- PlatformIO espressif32 7.0.1 / Arduino 2.0.17 编译成功，ESP32-C3，4MB Flash。当时正式应用829836字节，RAM41924字节。
- 原生 C++ 核心测试：启动关闭、AND允许/跟随、条件恢复不自启、超载与复位、暂停和真实故障独立、倒计时不清故障、millis回卷、35ms去抖、BL0942帧长度/校验/24位符号、能量计数回卷/回退。
- 用与固件相同的 ArduinoJson7 运行配置校验测试：字符串/布尔值不能替代数字，保护范围、校准、规则、时间、端口、Wi-Fi类型验证通过。
- Gradle :app:testDebugUnitTest（3个Java单元测试）、:app:lintDebug（0错误、6个样式/备份策略等警告）、:app:assembleDebug 成功。Android 8.0（API 26）兼容的有界读流代替readAllBytes。
- Android 15（API 35）模拟器安装并执行1个集成测试：App真实开启/关闭按钮调用HTTP；两路输入条件不满足时拒绝输出；错误令牌拒绝；App OTA使用有Content-Length的multipart上传，模拟端确认收到完整边界；退出界面禁用控制。模拟服务的继电器逻辑运行固件C++核心，没有真实继电器。
- 当时启动本机已有 Home Assistant Docker 容器，新增认证 Mosquitto 和 MQTT 集成。模拟发布器测试九实体发现、读数/bool状态、HA ON到模拟设备的回传以及offline→unavailable。模拟实体随后移除；当时还未接入真实ESP32，该结果不代表实际MQTT固件已运行。后续实板连接现已通过，见上文。
- APK v2签名验证通过。首次烧录合并镜像生成成功，offset0。
- 独立只读代码审查发现并修复：网络阻塞安全路径、两种OTA传输格式、保护阈值字符串绕过、跟随关闭与每日关闭、Android热点网络选择、暂停不能替代真实故障、安全任务创建失败关闭。复查无剩余指定Important问题。

尚未实测：自焊ESP32-C3、真实BL0942串口响应及校准、真实继电器电气和上电复位瞬态、热点实机配网、突然断电电量保存、NTP获取及实际Android 8.0（API 26）手机。需要画板并焊接后验证，不能用注入数据替代真实计量验收。

`dist/android-preview.png`为Android 15（API 35）模拟设备数据截图。
