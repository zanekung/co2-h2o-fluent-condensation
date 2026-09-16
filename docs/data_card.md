# 数据说明与来源

## 类型和范围

这里是数值模拟数据，不是实验测量。软件来源为 Windows Fluent 2026 R1 Student 的一次学习运行，网格 30000 个单元，步长 10⁻⁷ s、终态 0.01 s。程序生成几何，UDF 与数值模型来源见 [模型说明](model.md)。

|文件|内容|处理|
|---|---|---|
|`native/final-100000.cas.h5`|最终网格、完整求解设置及 UDF 挂接配置|按 HDF5 对象语义复制，仅清理文本中的机器路径；不是删除 UDF 挂接的查看专用 Case|
|`native/final-100000.dat.h5`|第 100000 步、0.01 s 的完整保存场|按 HDF5 对象语义复制，数值数据集已与原文件逐集对比一致；不重新求解、不改写数值|
|`native/` 中的四个 UDF 源文件|与原生文件配套的项目 C 源码及项目头文件|供运行者在自己的 Fluent 环境编译；不包含已编译库或厂商头文件|
|`data/final/all-cells-100000.csv.gz`|最终 30000 单元，39 列|仅无损压缩，解压字节与原 CSV 相同|
|`data/history/physics-history.out.gz`|0–100000 步、27 项报告|原数值 history 无损压缩件，数据未抽样|
|`logs/solver_transcript.redacted.log.gz`|完整求解 transcript|替换本机路径、用户名和主机名后压缩；Windows 文本编码保留|
|`figures/figure2_cn.png`、`figure2_en.png`|ANSYS 六联图|原灰色掩膜图按字节复制，没有重新绘图或 AI 生图|
|`verification/recorded_*.json`|历史核查|移除/替换机器身份与路径；不改变科学数值|

科学数值没有为了贴近图片或改善结果被修改。原生 HDF5 的对象语义复制可能改变文件内布局，文本路径清理也会改变整文件 SHA-256；因此公开原生文件不声称与原文件逐字节相同。路径变更范围、数值数据集对比和软件读取是三种不同检查，状态见 [验证说明](validation.md) 与 [verification](../verification/)。数据与来源哈希关系见 [public_source_manifest.json](../provenance/public_source_manifest.json)。公开清单使用相对来源标签，未暴露原机器目录。

CSV 首列为导出单元标识，其后含 x/y 坐标和 36 个物理字段。按字段名称读取；该 CSV 没有完整网格连通性、真实轴对称单元体积和全部边界通量，不能从普通行平均得到质量流加权量。原生 Case/Data 可用于核对网格拓扑和保存场，但这不自动补足全部历史边界通量记录。UDS/UDM 定义见 [field_dictionary.csv](field_dictionary.csv)。

原生文件保存的是已完成状态。仅查看时不要 Initialize、调用初场函数或 Calculate；查看完整 UDF 模型及继续求解前须在本机手动 Build/Load `libfuel`，并核对全部挂接。四个配套文件是 `co2_h2o_condensation.c`、`condensation_core.c`、`condensation_core.h`、`quasi1d_initialization.c`，其副本与 Case/Data 位于同一 `native/` 目录。原 Case 的 `udf/compile/files` 列表为空，不能承诺同目录源码会在读取 Case 时自动编译。操作见 [使用教程](tutorial.md) 和 [手动设置核对](manual_setup.md)。本包不再提供自动准备程序。

当前公开副本的 Fluent 实际读取状态为 **`NOT_VERIFIED_RUNTIME_BLOCKED`**：Windows 代码完整性策略在启动阶段拦截 `Qt5Widgets.dll`，读取测试未能开始。这不构成原生文件读取成功或失败的结论；原私有文件此前读取成功也不能代替本副本的测试。

## 未进入公开包的材料

EnSight 原生备份仍未纳入公开包；可用配套 Case/Data 在 EnSight 中按 [后处理教程](postprocessing.md) 建图。200 对中间检查点也未纳入公开包，历史监测和完整 transcript 则已包含。Case/Data 的路径清理按 HDF5 对象结构处理，不对二进制做盲目字节替换；完整设置和原始数值须保持，不能以解除动态库依赖为由删除物理模型挂接。

未发表稿件、原始生成式参考图片、Zotero 文库/文献 PDF、软件安装包、库、头文件、字体和连接凭据未公开。公开案例不要求使用者接触私人原稿或原机器。

## AI 辅助与软件计算

模型代码、自动化、核查和文档整理过程中使用了 AI 辅助。共享的流场数值来自 Fluent 求解与导出；图像来自 ANSYS 原生后处理。生成式图像没有作为定量标定目标，公开图不是由图像生成工具合成的流场。

已完成的软件流程并不能证明 AI 辅助的实现或建模选择全部正确。需要结合源码、独立实现测试、边界/单位、数值诊断和物理基准评估。

## 引用、许可和再利用

项目对有权许可的原创说明、数据汇编与图形贡献采用 CC BY 4.0。请保留案例名称、版本/日期、许可链接、[来源链接](https://github.com/zanekung/co2-h2o-fluent-condensation)及修改说明，示例见 [CITATION.md](../CITATION.md)。许可不把学习数据提升为经过验证的科研基准，也不将 ANSYS 软件或第三方保留权利转授给使用者。
