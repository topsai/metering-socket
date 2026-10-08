# Home Assistant 接入

[返回首页](../README.md)

## 连接关系

手机 App → 设备 HTTP 是局域网直连方式，可在没有 MQTT 服务的情况下使用。ESP32 → MQTT broker（消息服务器）→ Home Assistant 是另一条读数与控制路径；两种方式可以同时启用。

HA 接入需要 HA 和 broker 保持运行。它们部署在电脑上时电脑需在线，也可以部署在 NAS 或其他可访问主机上。设备连接 2.4 GHz Wi-Fi，MQTT 地址填写设备能够访问的 broker 主机地址；本机测试使用电脑局域网 IP，不使用 127.0.0.1 或仅在 Docker 内部可解析的容器名。

## 手动配置（推荐给新环境）

1. 启动你自己的HA与认证MQTT broker，确保它们互通。已有broker可以直接复用，不必运行仓库测试脚本。
2. HA“设置→设备与服务→添加集成→MQTT”，填写broker地址、1883及账号密码。
3. App“Wi-Fi / Home Assistant 配置”填写**设备能访问的局域网地址**、1883、账号密码，保存。密码空白保留旧密码，broker空白关闭连接。
4. MQTT连接建立后发送discovery，HA将显示一个“计量插座 ms…”设备和十三个实体。
5. 实体名称可在HA界面自行修改；重连和HA出生online时重新发布发现配置。

| 实体 | 类型 | 单位/意义 |
|---|---|---|
| 板载继电器 / 外接继电器 | switch | 两路ON/OFF分别受设备保护和输入条件约束 |
| 微动开关1 / 2 | binary_sensor | 闭合true |
| 电压 / 电流 / 功率 | sensor | V / A / W |
| 累计电量 | sensor | kWh，energy，total_increasing |
| 频率 | sensor | Hz |
| 板载保护锁定 / 外接保护锁定 | binary_sensor | problem；各自真实故障或暂停 |
| CF1计数 / CF1脉冲频率 | sensor | 本次启动上升沿数 / Hz，非另一路电量 |

HA 累计电量实体可供能量面板选择，统计正确性仍依赖实板校准与 NVS 持久化。连接正常时约每两秒发布状态；V/A/W/Hz 实体设置 15 秒 `expire_after`，超时未收到状态会过期。计量失联时设备发送 null，本次联测对应四项读数为 unknown。

异常断线由 broker 检测后发布遗嘱 offline，实体变为 unavailable，不能保证瞬时完成。正常主动 DISCONNECT 不触发遗嘱；当前固件主动断开前未发送 offline，保留的 availability 可能仍为 online，不能仅凭该字段判断设备在线。

HA 开关 ON 是请求，条件不满足时回传控制状态仍为 OFF；跟随模式忽略 MQTT ON，OFF 暂停跟随。复位、规则、校准、定时和 OTA 可使用 App 或 HTTP API；设备App与网页提供两路控制、规则、定时、复位、校准、板载方向反转与脉冲宽度设置，以及Wi-Fi和OTA。当前没有对应的 HA 配置按钮或数字实体，也没有继电器触点反馈。

## 本仓库Docker联测脚本

`tools/bootstrap_mqtt.py`是**本机联测便利脚本，不是通用HA安装器**。它的前提：Docker已启动、已有名为 `homeassistant` 的运行容器、HA已完成初始化且有正常用户登录产生的refresh token，HA容器内有paho-mqtt。脚本不会帮你创建HA用户。

```powershell
python tools/bootstrap_mqtt.py
```

副作用如下：

- 创建或启动 `meteringsocket-mqtt` 容器，镜像eclipse-mosquitto:2，发布主机1883。
- 创建 `meteringsocket` 网络，将已有HA加入网络；创建持久化数据卷。
- 本机生成随机账号密码，保存在忽略的 `private/mqtt.json`；broker配置保存在 `private/mosquitto/`。
- 如果没有MQTT集成，通过容器内已有登录凭据换取临时access token，新增MQTT集成；不输出/保存HA token。
- 历史脚本发布仅用于旧版软件验收的九个 `ms_test` 实体（不能据此验收当前十三实体），用HA服务测试模拟继电器，再清理模拟discovery和状态。**不控制真实插座。**

已有MQTT集成时脚本不会改其配置；若它连接别的broker，新测试broker上的实体不会到达HA，测试可能失败。这时使用手动配置或在隔离测试环境运行，不盲目重建生产HA。

原开发机器已完成上述配置。克隆本仓库不会带来其broker密码或HA凭据；请使用自己的账号。端口占用、容器名称不同、HA用户未登录等见[故障排查](TROUBLESHOOTING.md)。不要公开private目录，不复制HA `.storage/auth`。

同步前的九实体固件在真实 ESP32-C3 已完成发现、控制约束、遗嘱和重连联测，见[实板报告](HARDWARE_BENCH_2026-10-08.md)。Windows 需允许设备访问已发布的 1883 端口，添加防火墙规则需要管理员 PowerShell；本机能连接 broker 不能证明设备也能连接。

保存设备配置会主动断开 MQTT。固件在 Wi-Fi 已连接且配置了 broker 时，按大于十秒的尝试间隔重连；该间隔不是连接完成时限，网络或认证异常时可能一直离线。确认 `mqtt_connected=true` 后再发送 HA 命令，离线期间的非保留命令不会补发。

## 远程使用

App 的 HA 入口打开你设置的网页地址。当前地址校验允许 HTTP/HTTPS 根地址和端口，不接受子路径、查询参数、片段或内嵌账号；使用带子路径的入口时可自行在浏览器打开。

离家使用需已配置的 HA 安全远程入口，也可先连接家庭 VPN 再访问设备或 HA。仓库没有预置云服务、路由器端口转发或证书配置；设备 HTTP 和 MQTT 面向可信家庭网络，不直接发布公网。

MQTT主题及载荷见[API](API.md)。协议以[HA MQTT官方文档](https://www.home-assistant.io/integrations/mqtt/)为准。
