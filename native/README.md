# 最终 Fluent Case/Data

本目录包括同一步保存的 `final-100000.cas.h5`（网格、模型、材料、边界、数值方法、UDF 挂接）和 `final-100000.dat.h5`（最终场量与求解状态）。这是原学习运行的完整模型，未移除 UDF 源项或挂接。读取结果不需要重新计算，也不要初始化。

原始保存环境为 Windows Fluent 2026 R1 Student，二维、双精度；最终状态为第 100000 步、0.01 s。2024 R2 尚未验证。请先看[详细打开步骤](../docs/tutorial.md)，按 Case → Data 的顺序读入，或使用 Fluent 的 Case & Data 读取功能。

**这不是完全自包含的二进制包。** 凝结模型依赖 `libfuel`。本目录的三个 `.c` 和一个 `.h` 文件与 `udf/` 中的源代码逐字节相同，供使用者在本机 Fluent 中编译、加载；不分发已编译 DLL。原 Case 的编译文件列表为空，因此不能保证同目录放置源码后就会自动编译。必要时按教程手动 Build/Load，再读取这一对文件，无需自动准备程序。

本次已核查 HDF 数据集：网格、全部数值场以及除文件路径外的设置均与原始文件一致。只把旧电脑的目录前缀改为相对路径；详见[路径与数值核查](../verification/native_files_check.json)。因此整文件 SHA-256 与私有原件不同。

**本次公开副本尚未通过 Fluent 实际读取验证。** 启动测试被本机 Windows 代码完整性策略拦截的 ANSYS `Qt5Widgets.dll` 阻断，未进入读文件阶段；见[读取测试记录](../verification/native_read_attempt.json)。HDF 核查通过不代表已验证 Fluent 或跨版本兼容。

`export_final.jou` 是原 Case 结束导出设置引用的原生 Fluent journal，只将输出路径改为相对路径，不是准备程序。查看结果时无需执行。`checkpoints/`、`final_exports/` 为继续计算时预留的输出目录；原来的 200 对中间检查点没有打包。Case/Data 内保留的自动保存历史因此不是一组可逐个打开的随包文件。继续计算前在自己的工作副本中检查输出路径，避免覆盖既有文件。

只想重新手动建立模型时，参考[手动设置手册](../docs/manual_setup.md)。本案例仍是学习与方法检查记录，不是已完成物理验证的论文结果。
