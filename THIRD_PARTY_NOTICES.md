# 第三方组件和协议资料

本仓库尚未为原创代码提供许可证文件。第三方组件各自的许可证独立适用，下表列出使用版本、来源及随仓库保留的许可证副本。

| 组件 | 使用方式/版本 | 许可与来源 |
|---|---|---|
| ArduinoJson | PIO依赖7.4.2，JSON解析 | MIT；[许可证副本](licenses/ArduinoJson-MIT.txt)，[源码](https://github.com/bblanchon/ArduinoJson) |
| PubSubClient | PIO依赖2.8，MQTT | MIT；[许可证副本](licenses/PubSubClient-MIT.txt)，[源码](https://github.com/knolleary/pubsubclient) |
| Arduino-ESP32 | PIO Arduino框架2.0.17 | 框架及各组件按各自许可；[上游源码/许可](https://github.com/espressif/arduino-esp32/tree/2.0.17)，编译产物包含框架/ESP-IDF代码 |
| Gradle wrapper | 8.13，仓库带wrapper脚本与JAR | [Gradle上游许可](https://github.com/gradle/gradle/blob/v8.13.0/LICENSE)，Apache-2.0及其第三方通知 |
| Android Gradle Plugin | 8.13.2，构建工具依赖 | [Android构建工具源码](https://android.googlesource.com/platform/tools/base/)，按上游许可 |
| JUnit | 4.13.2，仅测试 | [上游许可](https://github.com/junit-team/junit4/blob/r4.13.2/LICENSE-junit.txt)，EPL-1.0 |
| Mosquitto | Docker测试broker，不复制源码 | [上游许可](https://github.com/eclipse-mosquitto/mosquitto)，按镜像/上游组件许可 |
| paho-mqtt | 在已有HA容器执行验证 | [上游](https://github.com/eclipse-paho/paho.mqtt.python)，不随本仓库打包 |

依赖下载在忽略的PIO/Gradle缓存中，不将第三方全量源码复制进仓库。固件包含依赖编译代码，重分发时同时保留对应上游许可与源码获取方式；当前PIO配置固定平台/库，框架2.0.17可从上游获取并自行重建。

BL0942字段、数据校验及换算参考[上海贝岭官方手册](https://www.belling.com.cn/media/file_object/bel_product/BL0942/datasheet/BL0942_V1.1_en.pdf)及[ESPHome BL0942文档](https://esphome.io/components/sensor/bl0942/)。这里的控制/解析实现为本项目代码，未复制ESPHome组件源码；文档参考不意味着包含其GPL源码。

MQTT discovery依据[Home Assistant官方MQTT文档](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)。本项目不包含美居/洗碗机私有协议、账号或密钥。
