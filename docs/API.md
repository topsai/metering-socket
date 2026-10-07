# 直连 HTTP / MQTT

HTTP默认80端口。除首页及热点首次配对外，所有接口要求 `Authorization: Bearer <32位设备令牌>`，JSON响应。配置不回传Wi-Fi/MQTT密码。

| 方法 | 地址 | 请求/行为 |
|---|---|---|
| GET | /api/pair | 只允许配网模式且通过设备热点访问，返回id和token |
| GET | /api/state | 计量、relay/requested、input1/2、fault/reason、规则、计时、配置 |
| POST | /api/relay | `{"on":true}` / false；跟随模式ON拒绝，OFF暂停跟随 |
| POST | /api/reset | `{}`；仅计量健康且未超限时解除保护，返回当前状态 |
| POST | /api/timer | `{"seconds":600}`，0取消，最多86400 |
| POST | /api/config | 局部更新，校验整个请求后保存；字段见下 |
| POST | /api/ota | 有Content-Length的multipart/form-data文件 firmware.bin，成功重启 |

配置字段：`rule`为 manual/input1/input2/both/either/both_follow/either_follow；`max_current`(0,16]、`max_power`(0,3680]，`schedule_on/off`为0~1439分钟或-1禁用；`vref/iref/pref`正数；`ssid/password`；`broker/mqtt_port/mqtt_user/mqtt_password`。不发送密码字段表示保持原密码。

计量失效时meter_valid=false，V/A/W/Hz返回null；energy是已累计值，不把失联表示为零。继电器命令响应是最终受输入/保护约束的状态，不保证请求ON能输出HIGH。401未认证、400参数错误、409跟随ON或复位条件不足。

MQTT设备id为ms加MAC低32位，基础主题 `meteringsocket/<id>`。

- `/state`：retained JSON；每2秒更新。
- `/availability`：retained online，LWT offline。
- `/relay/set`：ON/OFF，HA发控制指令不应retain。
- `homeassistant/<component>/<id>/<key>/config`：retained discovery，连接broker及HA出生online时重新发送。

一个HA设备下9实体，累计电量标记energy/kWh/total_increasing，其他计量measurement；HA只提供继电器及读数，规则/复位/固件升级在设备App中配置。互联网远程用HA既有入口，不把本机HTTP/API放到公网。

## 请求示例

以下为curl示意，IP和令牌使用你自己的值，不把真实令牌提交或贴到Issue。在Windows PowerShell使用curl.exe，变量和引号按本机shell调整。

```sh
export SOCKET_URL='http://192.168.1.100'
export SOCKET_TOKEN='<设备令牌>'
curl -H "Authorization: Bearer $SOCKET_TOKEN" "$SOCKET_URL/api/state"
curl -H "Authorization: Bearer $SOCKET_TOKEN" -H 'Content-Type: application/json' \
  -d '{"on":true}' "$SOCKET_URL/api/relay"
curl -H "Authorization: Bearer $SOCKET_TOKEN" -H 'Content-Type: application/json' \
  -d '{"rule":"both","max_current":10,"max_power":2200}' "$SOCKET_URL/api/config"
curl -H "Authorization: Bearer $SOCKET_TOKEN" -H 'Content-Type: application/json' \
  -d '{}' "$SOCKET_URL/api/reset"
curl -H "Authorization: Bearer $SOCKET_TOKEN" -H 'Content-Type: application/json' \
  -d '{"seconds":600}' "$SOCKET_URL/api/timer"
curl -H "Authorization: Bearer $SOCKET_TOKEN" \
  -F 'firmware=@dist/firmware.bin' "$SOCKET_URL/api/ota"
```

未知路径不属于版本契约；GET配置不单独提供，读取/api/state中公开字段。配置请求不超过2048字符，必须是JSON对象；null字段忽略，未知字段没有更新作用。不支持批量执行多个继电器动作。

## 状态字段

| 字段 | 类型 | 说明 |
|---|---|---|
| id / name | string | ms加MAC低32位 / 设备显示名 |
| relay / requested | bool | 实际输出 / 开机请求，两者可不同 |
| input1 / input2 | bool | 35ms去抖后的闭合状态 |
| fault / paused | bool | 合并锁定 / 暂停标志；paused不代表没有真实故障 |
| reason | string | on/off/interlock/meter_stale/overload/paused；瞬时timer可能被后续tick更新 |
| meter_valid | bool | 已收有效帧且距离最后一帧小于五秒 |
| voltage / current / power / frequency | number或null | V/A/有符号W/Hz，失联null |
| energy | number | 累计kWh，保留上次已知累计值 |
| rule | string | 七种规则代码之一 |
| max_current / max_power | number | 保护上限A/W |
| countdown | integer | 剩余秒数；0可能表示取消或不足一秒 |
| ip | string | Wi-Fi station IP（热点未连家庭网络时可能0.0.0.0） |
| mqtt_connected | bool | broker连接状态 |
| schedule_on / schedule_off | integer | 北京时间分钟0~1439，-1禁用 |
| vref / iref / pref | number | counts/V、counts/A、counts/W |
| broker / mqtt_user | string | 公开连接配置，无密码 |
| mqtt_port | integer | 1~65535 |

## 配置限制

规则须为已知字符串；保护和参考值须是有限数值，不能用字符串/布尔值代替。参考范围 `(0,1e9]`，SSID最多32字节，其余网络字符串最多128字节。schedule_on/off须整数；port须整数。JSON中的null表示不修改。

配置通过后返回 `{"ok":true}`，不返回完整状态；随后重新GET状态。保存撤销手动请求，但跟随规则在满足条件且未锁定时可重新开启。MQTT或Wi-Fi配置改变会断开并重连。

网络密码未发送表示保留MQTT旧密码；提交SSID时需同时提交Wi-Fi password，否则会用空密码。App对应“Wi-Fi名称空白不改，MQTT密码空白保留”。

## 响应与错误

| HTTP | 典型error/响应 |
|---|---|
| 200 | 状态对象、配对id/token或ok；relay=true不是请求必然结果 |
| 400 | invalid_json、object_required、numeric_range、schedule_range、port_range、string_range、unknown_rule、boolean_required、timer_range、update_failed |
| 401 | unauthorized |
| 403 | physical_setup_required |
| 409 | follow_mode；复位失败返回当前状态而非error字符串 |

OTA认证在上传回调和结束处理校验，上传必须multipart。固件无法验证“这个文件来自可信发布人”，没有签名验证/回退防护。

## MQTT载荷约定

state内容与HTTP状态一致。ON/OFF为大写文本，命令不retain。状态与发现retain；删除discovery用空retained payload，不要删除其他设备主题。

bool实体模板使用 `{{ value_json.<字段> }}`，继电器的state_on/off为Jinja输出True/False，command的payload_on/off为ON/OFF。正常断线LWT依赖broker检测超时；仅一次HTTP失败不代表MQTToffline。

MQTT不携带HTTP token，访问控制由broker账号实现。共享账号没有按设备分开的ACL；部署者可在broker增加主题ACL。
