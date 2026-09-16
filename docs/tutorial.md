# 从输入到结果：操作教程

本案例提供两条路径：在 Fluent 读取已经完成的原生结果，或按说明从网格和 UDF 手动建立自己的计算。公开包不包含 Fluent 自动准备程序，GUI 操作不要求安装 PyFluent。

记录中的完整计算使用 Windows Fluent 2026 R1 Student。Windows Fluent 2024 R2、跨版本恢复及不同机器上的完整复算均不能由该历史运行证明。

## 1. 先检查已有资料

解压案例，在案例根目录执行可选的离线检查：

```powershell
python tools/verify_case.py
python tools/analyze_results.py
```

这些工具只使用 Python 3.10+ 标准库，不启动 ANSYS。最终 CSV 应包含 30000 个单元、39 列，历史记录覆盖 0–100000 步，共 100001 行；15035 个单元满足半径显示条件。

也可用 [网格工具](../tools/mesh/build_paper_mesh.py) 重建到新的目录，比较其 SHA256 与 `mesh/paper_nozzle_30000.msh`。生成器拒绝覆盖已有文件。独立 C 测试的编译环境和适用范围见 [核心测试说明](core_tests.md)。

## 2. 在 Fluent 读取已有结果

### 文件和环境

`native/` 中应同时包含：

```text
final-100000.cas.h5
final-100000.dat.h5
co2_h2o_condensation.c
condensation_core.c
condensation_core.h
quasi1d_initialization.c
```

Case 保存网格、模型、边界、求解控制及 UDF 挂接；Data 保存该终态的场量。这里提供的是保留完整模型挂接的分享副本，不是删掉凝结源项的查看模型。未经改动的原始私有文件另外保留。

需要自己的适用 ANSYS 许可及 Fluent 可用的 C 编译环境。建议把整个 `native/` 目录复制到一个可写的 ASCII 工作目录，在 Launcher 中选择该目录，并选择 **2D、Double Precision、CPU solver**。历史运行使用 4 个进程。不要把 ANSYS 厂商头文件或别人机器的 DLL 当作本包的一部分复制。

目录内另有 Case 结束导出命令引用的 `export_final.jou` 及两个预留输出目录。它们不是准备程序，查看结果时无需执行；若以后继续计算，先检查输出位置。自动保存历史保留了原检查点名称，但原 200 对中间 Case/Data 未随包提供，不能通过这些历史条目恢复中间场。

### UDF 库与读取步骤

1. 在 Fluent 中选择 **File → Read → Case & Data...**，选择 `final-100000.cas.h5`；匹配的 `final-100000.dat.h5` 应位于同目录。也可先 **File → Read → Case...**，成功后再 **File → Read → Data...** 显式读取匹配文件。
2. Case 保留 `libfuel` 引用。若没有本机匹配库而报错，请按下一节在 Fluent 内手动 **Build → Load**，然后重新读取 Case 和 Data。不能用继续求解或初始化解决缺库问题。
3. 官方确有在读 Case 时自动编译缺失库的机制，但依赖保存的编译设置。本文件的 `udf/compile/files` 和 `udf/c-files` 源清单为空，**仅把源码放在同目录不能保证自动重建**，所以本教程提供手动编译步骤。[官方编译说明](https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/flu_udf/flu_udf_ChapCompilingUDFs.html)
4. 核对网格为 30000 个单元、2 UDS 和 26 UDM，终态步数为 100000、物理时间为 0.01 s。Console 可分别查询 `(rpgetvar 'time-step)` 与 `(rpgetvar 'flow-time)`。
5. 查看保存的 Contours 和 Setup 页面。仅查看已有场量不需要初始化或点击 **Calculate**。多个 Fluent 进程是并行会话组成部分，本身不证明正在迭代。

**本次实际验证边界（2026-09-16）：Windows Code Integrity 阻止 ANSYS 自带 `Qt5Widgets.dll` 加载，Fluent 在启动阶段失败，尚未进入 Case 读取。** 没有禁用或修改安全策略。本次公开副本的读取、编译加载、字段名称及六个云图对象恢复均未完成运行验证；以上是操作指引，不是即开测试通过声明。原始私有 Case/Data 在 2026-09-15 曾成功读入，但那不能代替当前副本的验证。Windows 2024 R2 未测试。

### 缺少 libfuel 时手动编译

这些步骤仅加载模型所需库，不运行准备程序，也不初始化或求解。当前公开副本的完整步骤尚未在新会话中测试通过。

1. 保持上述六个文件在自己的可写工作目录。若失败的 Case 读取没有留下网格，可先通过 **File → Read → Mesh...** 读取本包的 `mesh/paper_nozzle_30000.msh`；不要初始化。
2. 在 **User-Defined → Scalars** 确认数量为 **2**，在 **User-Defined → Memory** 确认普通单元 UDM 数量为 **26**，节点内存为 **0**。这为本模型的加载时字段命名提供存储；不要清零或减少已有的存储。
3. 打开 **User-Defined → Functions → Compiled...**。不同界面布局可能列在 **Parameters & Customization → User Defined Functions**。将 Library Name 设为 `libfuel`；Source Files 添加 `co2_h2o_condensation.c`、`condensation_core.c`、`quasi1d_initialization.c`，Header Files 添加 `condensation_core.h`。这些是本案例文件，不需要手工复制厂商 `udf.h`。
4. 使用当前 Fluent 支持的编译器；Windows 可启用 **Use Built-In Compiler**，然后点击 **Build**。仅在 Console 确认编译成功后点击一次 **Load**，检查函数名称列出且无错误。[官方 GUI 编译流程](https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/flu_udf/flu_udf_sec_compile_gui.html)
5. 重新 **Read Case**，再 **Read Data**，核对终态步数、时间、UDS/UDM 和图形字段。若提示丢弃当前临时网格，确认你正在使用专门的查看会话后再继续。不要覆盖保存的结果文件。

也可在 Fluent Console 采用下列编译/加载命令。文件名相对于当前工作目录；末尾的空字符串分别结束源文件和头文件列表。该命令形式来自官方 TUI 与历史配置记录，当前公开副本未完成运行验证。

```text
/define/user-defined/use-built-in-compiler? yes
/define/user-defined/compiled-functions compile "libfuel" yes "co2_h2o_condensation.c" "condensation_core.c" "quasi1d_initialization.c" "" "condensation_core.h" ""
/define/user-defined/compiled-functions load "libfuel"
```

原模型曾遇到 UDF 在 UDS/UDM 分配之前加载而未注册完整字段名称的问题；历史成功路径是先读原网格、分配 2 UDS/26 UDM、加载已有库，再显式读 Case 和 Data。这是本模型的排查记录，不是所有 Fluent Case 都必须执行的通用步骤。新的手动编译步骤需要在接收电脑上核查结果。

库的进一步说明见 [手动设置指南](manual_setup.md) 和[官方 TUI 编译文档](https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/flu_udf/flu_udf_sec_compile_tui.html)。不要把从零初始化流程用于已经读入的最终结果；若本机应用控制阻止编译器或库加载，应交由有权限的系统管理人员处理，教程不要求修改或关闭安全策略。

`co2h2o_refresh_diagnostics` 会重新计算并写入诊断 UDM；它不是纯显示刷新。为查看保存的原始终态，不要执行该函数、初始化、Patch 或额外迭代。若需要修改设置，保存到自己的新文件并记录改变。

## 3. 从零手动建立自己的计算

按 [完整手动设置指南](manual_setup.md) 导入网格、编译和挂接 UDF，设置物性、边界和数值方法。自己的运行目录与本包参考结果分开；不使用最终 Data 代替时间零点初始场。

历史准备过程先建立冻结组成的干气初场，请求最多 500 次一阶和 1000 次二阶定常迭代；这不同于正式凝结时间推进。准备结束时恢复凝结源项、两个 UDS 和所需 hooks，切换为 pressure-based transient PISO 及二阶格式。新的手动操作并不自动继承历史初场或通过验证。

开始正式计算前逐项核对：

- 时间零点、固定时间步 `1e-7 s`、计划 100000 步、每步最大内迭代 250。
- 网格及工况与 [模型说明](model.md) 一致，两个 UDS/26 个 UDM 和所有必需 UDF 挂接有效。
- 每步写入所需监测值，每 500 步保存 Case/Data；输出路径指向自己的运行目录。
- 已保存初态，并确认 Case/Data 和本地编译库能够在匹配环境中读回。

源项和输出设置须按手动指南实际建立，仓库不会自动替你配置。保存终态或导出 CSV 的动作也须由自己执行，不能依赖已经移除的准备程序。

## 4. 启动正式计算并核对完成状态

在 **Run Calculation** 页面确认准备完毕后，点击一次 **Calculate**。历史正式运行连续推进 100000 步至 0.01 s，没有中途重新读 Case/Data；4 进程耗时约 15 小时 05 分。这不是对其他机器或新初场的耗时保证。

历史记录监测 27 个量并逐步保存瞬时值，包含时间零点。绘图频率 10 不等于 history 每 10 步写一次。无论使用手动导出还是结束命令，出现文件都不能单独证明跑满；核对实际 step 和 flow-time。

完成自己的运行后，保存匹配的 Case/Data、完整监测及全单元 CSV，检查有限性和收敛情况，并阅读 [验证范围](validation.md)。不得将新数据覆盖本包参考数据后继续沿用旧来源哈希。

## 5. 在 ANSYS 后处理与拓展

按 [EnSight 后处理教程](postprocessing.md) 读取配对 Case/Data，并核对变量含义后建立六联图。改变图例、字体、掩膜和视口不要求重新初始化或求解。公开 PNG 使用有说明的灰色半径掩膜，避免把没有有效液滴群的区域解释为零半径。

`requirements-postprocess.txt` 仅供选择 Python/PyEnSight 自动化示例的使用者参考；Fluent 和 EnSight GUI 操作不需要它，也没有安装该依赖组合即可在任意电脑运行的保证。

若将学习案例拓展为研究，应建立可追溯的物理基准，完成网格与时间步敏感性、限幅与负尾值影响、混合气增长及能量模型核验。换用科研许可重算是环境变化，不能代替科学验证。
