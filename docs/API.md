# 直连 HTTP / MQTT

[返回首页](../README.md)

HTTP默认80端口。除首页及热点首次配对外，所有接口要求 `Authorization: Bearer <32 个十六进制字符的设备令牌>`，JSON响应。配置不回传Wi-Fi/MQTT密码。

| 方法 | 地址 | 请求/行为 |
|---|---|---|
| GET | /api/pair | 只允许配网模式且通过设备热点访问，返回id和token |
| GET | /api/state | 计量、relay/requested、input1/2、fault/reason、规则、计时、配置 |
| POST | /api/relay | `{"on":true}` / false；跟随模式ON拒绝，OFF暂停跟随 |
| POST | /api/reset | `{}`；仅计量健康且未超限时解除保护，返回当前状态 |
| POST | /api/timer | `{"seconds":600}`，0取消，最多86400 |
| POST | /api/config | 局部更新，校验整个请求后保存；字段见下 |
| POST | /api/ota | 有Content-Length的multipart/form-data文件 firmware.bin，成功重启 |

配置字段：`rule`为 manual/input1/input2/both/either/both_follow/either_follow；`max_current`(0,16]、`max_power`(0,3680]，`schedule_on/off`为0~1439分钟或-1禁用；`vref/iref/pref`正数；`ssid/password`；`broker/mqtt_port/mqtt_user/mqtt_password`。

不发送 `mqtt_password` 时保留原 MQTT 密码；发送空字符串则将 MQTT 密码置空。Wi-Fi 配置只有提交非 null 的 `ssid` 才会更新，此时须同时显式提交字符串 `password`，开放网络使用 `""`。当前实现中缺省或 null 的 `password` 经字符串转换会成为字面值 `"null"`，不会保留旧密码，也不表示空密码。单独提交 `password` 不会更新 Wi-Fi 密码。App 的空密码输入行为见下文，不能与 API 字段缺省混为一谈。

计量失效时 `meter_valid=false`，V/A/W/Hz 返回 null；`energy` 保留已累计值。`relay` 是受输入与保护约束的逻辑输出状态，没有触点反馈；ON 不保证 GPIO 输出 HIGH，OTA 期间另有强制关闭约束。401 未认证、400 参数错误、409 跟随 ON 或复位条件不足。

MQTT设备id为ms加MAC低32位，基础主题 `meteringsocket/<id>`。

- `/state`：retained JSON；连接正常且主循环可运行时，约每两秒更新，继电器命令处理后也发布状态。
- `/availability`：retained online，LWT offline。
- `/relay/set`：ON/OFF，HA发控制指令不应retain。
- `homeassistant/<component>/<id>/<key>/config`：retained discovery，连接broker及HA出生online时重新发送。

一个HA设备下9实体，累计电量标记energy/kWh/total_increasing，其他计量measurement；HA只提供继电器及读数，规则/复位/固件升级在设备App中配置。互联网远程用HA既有入口，不把本机HTTP/API放到公网。

## 请求示例

以下 Bash 示例使用示意 IP 和令牌，运行时替换为自己的值。不要将真实令牌提交到仓库或贴到 Issue。

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

Windows PowerShell 可使用以下示例，变量名称避免与系统变量冲突：

```powershell
$socketUrl = 'http://192.168.1.100'
$socketToken = '<设备令牌>'
$socketHeaders = @{ Authorization = "Bearer $socketToken" }
Invoke-RestMethod -Uri "$socketUrl/api/state" -Headers $socketHeaders
Invoke-RestMethod -Uri "$socketUrl/api/relay" -Method Post -Headers $socketHeaders -ContentType 'application/json' -Body '{"on":true}'
```

未知路径不属于版本契约；GET 配置不单独提供，读取 `/api/state` 中的公开字段。配置请求体最多 2048 字节，必须是 JSON 对象；一般配置字段的 null 表示不修改，但提交 `ssid` 时的 Wi-Fi 密码遵循上文特例。未知字段不新增配置，但有效请求即使只有未知字段或为空对象，仍执行保存的副作用。不支持在一个请求中批量执行继电器动作。

## 状态字段

| 字段 | 类型 | 说明 |
|---|---|---|
| id / name | string | ms加MAC低32位 / 设备显示名 |
| relay / requested | bool | 控制器逻辑输出 / 开启请求；没有触点反馈，OTA另有GPIO禁用约束 |
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

规则须为已知字符串；保护和参考值须是有限数值，不能用字符串/布尔值代替。参考范围 `(0,1e9]`，SSID 最多 32 字节，其余网络字符串最多 128 字节。`schedule_on` / `schedule_off` 和 `mqtt_port` 须为整数。null 的处理见上文 Wi-Fi 密码特例。

配置通过后返回 `{"ok":true}`，不返回完整状态；随后重新 GET 状态。保存撤销手动请求并取消倒计时，跟随规则在满足条件且未锁定时可重新开启。每次配置保存都会主动断开 MQTT，随后尝试重连；提交 SSID 时还会重新连接 Wi-Fi。

App 的 Wi-Fi 名称空白表示不发送 SSID，即不更改 Wi-Fi；输入 SSID 后会同时提交 Wi-Fi 密码，空白表示空密码。App 的 MQTT 密码空白表示不发送该字段，即保留旧密码。

## 响应与错误

| HTTP | 典型error/响应 |
|---|---|
| 200 | 状态对象、配对id/token或ok；relay=true不是请求必然结果 |
| 400 | invalid_json、object_required、numeric_range、schedule_range、port_range、string_range、unknown_rule、boolean_required、timer_range、update_failed |
| 401 | unauthorized |
| 403 | physical_setup_required |
| 409 | follow_mode；复位失败返回当前状态而非error字符串 |

OTA 认证在上传开始回调和结束处理校验，上传必须 multipart。完整的未认证请求返回 401，不开始写 Flash；上传中断回调会设置故障锁定。固件没有签名验证或版本防回退，不能验证文件发布者。

## MQTT载荷约定

state内容与HTTP状态一致。ON/OFF为大写文本，命令不retain。状态与发现retain；删除discovery用空retained payload，不要删除其他设备主题。

bool 实体模板使用 `{{ value_json.<字段> }}`，继电器的 `state_on` / `state_off` 为 Jinja 输出 True/False，命令 `payload_on` / `payload_off` 为 ON/OFF。LWT 用于异常断线，依赖 broker 检测连接丢失或超时；主动发送 MQTT DISCONNECT 不触发遗嘱。当前固件主动断开前未发布 offline，因此保留的 availability 可能仍为 online。一次 HTTP 失败也不能证明 MQTT 已离线。

MQTT不携带HTTP token，访问控制由broker账号实现。共享账号没有按设备分开的ACL；部署者可在broker增加主题ACL。
