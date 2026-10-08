# 故障排查

[返回首页](../README.md)

| 现象 | 检查与处理 |
|---|---|
| 找不到热点 | 没有Wi-Fi配置才默认开启；长按GPIO8原有KEY三秒，检查按钮接CTRL_GND，热点十分钟关闭 |
| 热点显示无互联网 | 这是预期；保持连接，不切换其他Wi-Fi；App优先用Wi-Fi网络 |
| 配对403 | 必须setup模式且从设备热点访问192.168.4.1；家庭局域网不允许读取token |
| 配网后App断连 | 家庭Wi-Fi IP变了；路由器查IP，更新当前设备地址，保留token |
| 域名无法连接 | 优先IP，固定DHCP；不同Android系统mDNS解析不同 |
| 401 unauthorized | token不对/设备NVS擦除；重新热点配对并更新设备 |
| 输入一直false | NO触点闭合到CTRL_GND；检查GPIO4/5、35ms去抖、没有接HOT_GND |
| 开启请求后仍OFF | 查看meter_valid、fault、paused和rule；条件不满足会撤销请求，解除故障后须重发开启 |
| 跟随模式ON报409 | HTTP接口预期返回409，MQTT ON会被忽略；OFF暂停，健康计量下解除锁定或每日ON可恢复暂停 |
| 解除锁定409 | 计量不健康或仍超限；修复串口/负载，等待有效读数后再解除 |
| meter_stale / 全部读数无效 | BL0942供电、UART隔离方向、电平、4800/8N1/地址0、RX/TX交叉、SEL/BPS设置；检查帧校验 |
| 电压/功率数量级不对 | 分流或分压与默认不同；按标准表校准vref/iref/pref，不调整阈值掩盖校准错误 |
| 电量重启后少一些 | 五分钟周期保存、首帧建立基线；不是按每帧写Flash |
| 定时没执行 | 需NTP校时，UTC+8、HH:mm；故障不能被定时ON清除，无漏执行补跑 |
| MQTT离线 | 确认broker主机在线、设备可访问其地址/端口、账号密码及防火墙；本机Docker部署使用主机LAN地址。保存配置后会重连，十秒是尝试间隔而非完成时限；以mqtt_connected确认连接 |
| HA没有发现 | HA和设备必须连同broker，MQTT集成已开启；broker重连或HA出生online会重新发现 |
| HA实体unavailable / unknown | 异常MQTT断线的遗嘱或状态过期可导致unavailable；计量null对应unknown。主动DISCONNECT可能保留旧online，另查mqtt_connected，不仅看availability |
| OTA失败 | 使用firmware.bin，不用factory/HTML/其他芯片镜像；令牌有效、分区匹配，App文件上限为1,900,000字节（含等号），并检查网络及供电 |
| 无法进入ROM下载 | 松开GPIO8 KEY使其保持HIGH，再按GPIO9 BOOT执行下载；GPIO2/8/9原有外部上拉保留 |
| 板载开启方向相反 | 核对INA/INB与实板触点，通过latch_on_ina反转；状态估计没有触点反馈 |
| CF1不计数 | 核对GPIO10、隔离后逻辑电平和上升沿；默认电量脉冲随负载变化，不能将其当作电网频率 |
| OTA后仍锁定 | 预期；有效计量后解除，手动模式再开启 |
| 缺Build Tools35错误 | 确认使用本仓库明确指定36.0.0的build.gradle，并安装Build Tools36 |
| SDK/JDK路径错误 | 设置JAVA_HOME为JDK17、ANDROID_HOME为SDK，见BUILD，不依赖原电脑路径 |
| Gradle离线缺依赖 | 首次去掉--offline联网构建；代理仅放本机环境 |
| APK签名不一致 | 新电脑debug key与旧APK不同；卸载会失去保存凭据，准备重新热点配对 |
| 原生测试编译器不存在 | 安装MinGW/g++，或修改test-core.ps1编译器路径；必须先pio build取得ArduinoJson |
| core_bridge.exe Permission denied | 模拟服务占用该程序，停止模拟服务再重新编译 |

## HA脚本专有问题

- 1883占用：已有broker时手动复用，脚本不会自动迁移端口。
- 找不到homeassistant：脚本假定该容器名称，不负责创建HA；新环境用手动接入步骤。
- 找不到normal refresh token：HA需先完成onboarding和用户登录；不要复制他人的auth文件。
- 已有MQTT连别的broker：脚本不会覆盖集成，模拟实体可能不可见；在隔离环境验证或调整手动接入方式。
- HA接口版本变化：查看容器实际config-flow结构，脚本仅记录已验证版本行为，不直接编辑HA `.storage` 配置。

报问题时提供版本、规则、meter_valid/fault/paused/reason、错误码和经脱敏的日志；不要贴token、Wi-Fi/MQTT密码或HA认证文件。见[SECURITY](../SECURITY.md)。
