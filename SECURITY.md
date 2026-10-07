# 安全与凭据

## 公开仓库范围

源码、公开文档、演示截图、APK、固件和校验值可提交。`private/`、`.env`、HA认证存储、签名密钥、机器专用SDK配置和构建缓存不提交。`.gitignore`只是防误操作，发布前还需检查实际Git内容。

本版提供 `python tools/check_public.py`，检查暂存文件、文档本地链接、产物哈希、禁止路径与本机MQTT密码；同时解压APK/ZIP/JAR扫描。它不是所有秘密格式的完整检测器，Git推送前仍应检查暂存清单。

测试固定token `0123456789abcdef0123456789abcdef`只在本机模拟器/test夹具使用，是公开演示值。真实固件token由esp_random随机生成，测试APK不作为正式App发布。

## 网络与配对

设备HTTP和MQTT为可信家庭网络设计，未实现设备HTTPS或MQTT TLS。令牌认证不等于加密传输；不把设备80端口、broker1883直接映射公网。远程使用HA既有安全入口或家庭VPN。

热点密码socketsetup为公开默认值，配对只在十分钟setup窗口且从热点入口可读。长按物理按钮进入setup会暂停输出；在附近且能加入热点的人可能于窗口内取得令牌，完成后关闭热点。当前没有独立轮换token接口，擦除NVS会生成新token并丢失其他配置。

Wi-Fi、MQTT密码及token在设备NVS保存，未配置Flash加密或secure boot。Android配置使用Keystore AES-GCM，禁止应用备份；debug APK仍可被已授权ADB调试。不要分享设备配对信息、private/mqtt.json或HA .storage/auth。

## 固件和硬件

OTA要求token，未验证数字签名、发布者或版本防回退；仅上传可信来源匹配芯片/分区的firmware.bin。上传和结束锁定输出。

故障关闭、输入联动、安全任务均是软件措施，不是经认证的市电安全控制；GPIO复位前电平、继电器驱动、保险丝、爬电距离和隔离必须在硬件落实。HOT_GND不能与用户可接触的CTRL_GND跨接。

## 报告问题

普通功能问题通过GitHub Issue附版本、脱敏日志、规则和复现步骤。安全问题只描述影响和版本，不在公开Issue贴真实密码/token或可复用的HA认证数据；可使用GitHub提供的私下安全报告入口（若仓库启用）。没有预置个人邮箱或漏洞响应时限。
