# Home Assistant 接入

[返回首页](../README.md)

## 连接关系

手机App→设备HTTP是独立直连。ESP32→MQTT broker→Home Assistant用于HA控制和读数；HA和broker运行在电脑上时电脑需在线。设备连接家庭2.4GHz Wi-Fi，使用电脑的局域网IP，不使用127.0.0.1或Docker内部容器名。

## 手动配置（推荐给新环境）

1. 启动你自己的HA与认证MQTT broker，确保它们互通。已有broker可以直接复用，不必运行仓库测试脚本。
2. HA“设置→设备与服务→添加集成→MQTT”，填写broker地址、1883及账号密码。
3. App“Wi-Fi / Home Assistant 配置”填写**设备能访问的局域网地址**、1883、账号密码，保存。密码空白保留旧密码，broker空白关闭连接。
4. MQTT连接建立后发送discovery，HA将显示一个“计量插座 ms…”设备和九个实体。
5. 实体名称可在HA界面自行修改；重连和HA出生online时重新发布发现配置。

| 实体 | 类型 | 单位/意义 |
|---|---|---|
| 插座继电器 | switch | ON/OFF受设备保护和输入条件约束 |
| 微动开关1 / 2 | binary_sensor | 闭合true |
| 电压 / 电流 / 功率 | sensor | V / A / W |
| 累计电量 | sensor | kWh，energy，total_increasing |
| 频率 | sensor | Hz |
| 保护锁定 | binary_sensor | problem；真实故障或暂停 |

HA累计电量实体可供能量面板选择，最终统计正确性仍依赖实板校准与NVS持久化。V/A/W/Hz有15秒expire_after；每两秒发送状态。失联计量返回null，连接断开则availability offline（异常断线由broker遗嘱检测，不保证瞬时）。

HA开关ON只是请求，条件不满足时状态仍OFF；跟随模式拒绝ON，OFF暂停跟随。复位、规则、校准、定时、OTA在App/网页或HTTP API完成，当前不提供对应HA按钮/数字实体。

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
- 发布仅用于软件验收的九个 `ms_test` 实体，用HA服务测试模拟继电器，再清理模拟discovery和状态。**不控制真实插座。**

已有MQTT集成时脚本不会改其配置；若它连接别的broker，新测试broker上的实体不会到达HA，测试可能失败。这时使用手动配置或在隔离测试环境运行，不盲目重建生产HA。

原开发机器已完成上述配置。克隆本仓库不会带来其broker密码或HA凭据；请使用自己的账号。端口占用、容器名称不同、HA用户未登录等见[故障排查](TROUBLESHOOTING.md)。不要公开private目录，不复制HA `.storage/auth`。

## 远程使用

App的HA入口只打开你设置的网页地址。使用已有HA安全远程入口或先连接家庭VPN；仓库不配置云服务、路由器端口转发或证书。设备HTTP、MQTT均面向可信家庭网络，不直接发布公网。

MQTT主题及载荷见[API](API.md)。协议以[HA MQTT官方文档](https://www.home-assistant.io/integrations/mqtt/)为准。
