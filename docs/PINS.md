# ESP32-C3 GPIO 定义

[返回首页](../README.md)

GPIO号不是封装脚号；本固件假定4MB Flash。保留现有元件，使用原有KEY与LIGHT，不新增配网按钮或状态灯。

| 信号 | GPIO | 接线/备注 |
|---|---:|---|
| 板载K1 INB / INA | 0 / 1 | CN8023B驱动 FH44L-1AT-L1-DC5V；只允许限时脉冲 |
| LIGHT | 2 | LOW点亮原有LED；原有外部上拉保留 |
| 外接普通继电器 OUT | 3 | HIGH开启、LOW关闭，接3.3V兼容模块逻辑输入 |
| 微动开关1 / 2 | 4 / 5 | NO闭合到CTRL_GND为true，内部上拉，35ms去抖 |
| BL0942 UART RX / TX | 6 / 7 | 隔离后RX接BL0942 TX、TX送BL0942 RX；4800bps、8N1 |
| 原有KEY | 8 | LOW有效，短按切换板载开关，长按3秒配网；原有外部上拉保留 |
| BOOT | 9 | 保留下载按钮及原有外部上拉 |
| BL0942 CF1 | 10 | 隔离后上升沿输入；运行计数与Hz，不另计累计电量 |
| USB D- / D+ | 18 / 19 | 保留芯片原生USB下载/调试 |
| UART0 RX / TX | 20 / 21 | 保留3.3V串口下载/115200日志 |

GPIO2/8/9为启动相关脚。进入ROM下载时GPIO8必须保持HIGH，因此必须松开KEY，再按BOOT执行下载；勿在下载复位时长按KEY。GPIO12~17留给Flash，EN保留复位。GPIO4~7与外部JTAG复用，使用这些IO后不要再接外部JTAG；可用USB Serial/JTAG。

板载K1使用非阻塞默认100ms脉冲，`latch_pulse_ms`可设整数50~200ms；换向先将INA/INB都拉LOW，等待5ms后发脉冲，结束两脚LOW。脉冲由硬件定时器的 IRAM 中断限时撤掉驱动，状态机负责换向和估计。启动强制发OFF脉冲。`latch_on_ina=true`暂定INA为ON方向，需实板确认；可在App/网页反转。逻辑估计不能替代触点反馈，突然掉电时磁保持继电器可能保持原触点状态，不能保证OFF。

GPIO3模块输入可按实际电路评估10kΩ下拉；复位/烧录瞬态仍需测量，不能据此保证触点关闭。GPIO不能直接驱动裸线圈，5V逻辑模块需匹配驱动。

LIGHT在配网时快闪、Wi-Fi断连时慢闪、连接时常亮。GPIO2/8/9原有外部上拉及所有既有器件均保留。

开关、控制与继电器模块使用隔离后的CTRL_GND，不能跨接HOT_GND。BL0942的HOT_GND仍在火线域，UART/CF1必须经隔离进入ESP32。裸芯片仍需电源去耦、EN、电源时序、40MHz晶振、天线及Flash。

`hardware/schematics/`保存不含 PCB 的原生 `.epro2` 工程包、独立 `.esch` 导出及网表，PCB不包含在发布范围。当前主图仍有1项DRC错误；控制图0错误、3警告，旧客户端未做视觉审核。这些源文件不代表已完成投板验收。
