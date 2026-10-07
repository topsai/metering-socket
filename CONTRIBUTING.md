# 参与开发

先阅读[架构](docs/ARCHITECTURE.md)、[API](docs/API.md)和[验证范围](docs/VERIFICATION.md)。本仓库不包含PCB，改引脚/电气假定时同时改PINS和用户文档。

1. 新建分支，修改共享核心时补真实行为回归测试，尤其故障优先级、暂停、输入恢复和时间回卷。
2. 固件运行 `pio run -d firmware`；Windows运行 `tools/test-core.ps1`，其他平台见TESTING。
3. Android运行testDebugUnitTest、lintDebug和assembleDebug；改变控制/API/OTA时用隔离模拟器验收，不点击真实负载。
4. 更新相关文档和CHANGELOG，明确模拟验证与实板验证，不能把构建成功写成硬件验收。
5. 若更新dist，确保APK/bin来自当前源码，重新合并factory镜像和更新SHA256SUMS。
6. `git add`后运行 `python tools/check_public.py`，检查暂存清单；不提交private、签名密钥、.storage/auth或机器配置。

PR说明包括问题、行为变化、验证命令/结果和未测试项。用户界面优先中文，协议字段保持稳定；不未经版本说明改变既有字段或故障复位语义。

项目当前未指定原创代码许可；贡献前确认你有权提交该内容，第三方代码需保留许可和来源。依赖见[THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES.md)。
