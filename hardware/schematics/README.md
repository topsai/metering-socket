# 计量插座原理图

[返回首页](../../README.md) · [引脚配置](../../docs/PINS.md)

`metering-socket-schematics.epro2` 是嘉立创EDA原生工程包的原理图发布副本，包含主控、插座主体、隔离电源与通信三页，以及所需符号、器件和封装库。可在嘉立创EDA通过导入工程文件选择此包；本次已验证压缩包结构与网表，尚未验证导入后的完整往返和视觉显示。

发布包已去掉 PCB、拼板文档与缓存预览图。封装库仍保留，用于原理图器件关联；它们不是 PCB 布局。原工程的 PCB 没有改动。

| 文件 | 内容 |
|---|---|
| controller-c3.esch / .net.json | ESP32-C3 主控页源码与网表 |
| socket-main.esch / .net.json | 插座主体页源码与网表 |
| isolation.esch / .net.json | 隔离电源与通信页源码与网表 |
| metering-socket-schematics.epro2 | 原生工程包，三页原理图与库 |
| manifest.json | 导出边界和校验状态 |
| ESP32-C3-verification.json | 原理图连接及 DRC 分类结果 |
| ESP32-C3-原理图检查记录.md | 逐项检查及未解决事项 |

`.esch` 是文档源码快照，用于版本对比和 API 恢复；不要把它当成已验证可独立导入的完整工程。正常导入优先使用 `.epro2`。

板载磁保持继电器、原灯、原按键、CF1 和外接继电器均已接入。主控 U2 与主体 P2 必须按同号实际连接。GPIO0/1 只发限时驱动脉冲；默认 INA 为开启方向仍需对实物低压核对，可通过 App/网页反转。没有触点反馈，也没有进行市电负载试验。

主控 DRC 为 0 错误、3 警告；主体和隔离页属同一原理图，仍有一项既有错误及六项警告。旧版接口只返回数量，尚未定位该错误；不能宣称可直接投板。

检查发布文件：在仓库根目录执行 `python tools/verify_schematics.py`。重新打包已导出的原工程副本可执行 `python tools/package_schematics.py <本地完整epro2> hardware/schematics/metering-socket-schematics.epro2`，再更新源码、网表、清单和哈希。完整原工程含 PCB，不应直接提交。
