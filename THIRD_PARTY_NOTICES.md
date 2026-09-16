# 第三方来源和权利边界

ANSYS、Fluent、EnSight 及相关名称属于其权利人。本案例不是 ANSYS 官方教程，也不声称获得其个别审核或背书。

**Images used courtesy of ANSYS, Inc.**

上述致谢依据 [ANSYS Academic Terms — Fair Use and Copyright](https://ansys.synopsys.com/academic/terms-and-conditions)。项目没有复制手册全文、软件图标库、系统字体、许可文件或求解器安装内容。

UDF 依赖由自己的合法 ANSYS 安装提供的 `udf.h` 和接口；仓库不含该头文件本体或已编译 DLL。PyFluent/PyEnSight 作为外部依赖获取，其自身许可和依赖许可不会被本项目 MIT 覆盖。

`native/final-100000.cas.h5` 和同名 `.dat.h5` 提供完整算例设置和保存的数值结果。机器路径通过 HDF5 对象语义复制清理；没有以解除库依赖为由删除模型挂接，也不分发原机器的编译库。同目录的 `co2_h2o_condensation.c`、`condensation_core.c`、`condensation_core.h`、`quasi1d_initialization.c` 为项目配套源码，其中 `condensation_core.h` 是项目头文件，不是厂商 `udf.h`。配套源码不会因位于同目录就保证自动编译；运行者仍需自己的 ANSYS 安装和相应编译环境，手动 Build/Load `libfuel`，见 [手动设置说明](docs/manual_setup.md)。EnSight 原生备份暂不分发。

`quasi1d_initialization.c` 和建模模块中的 CO₂ 比热多项式/部分输运设置来自原建模时采用的 Fluent 物性值，相关注释保留。它们不是本项目原创实验物性；少量数值的来源本身也不等于已经确认存在禁止披露的版权条款。本次未改变这些数值以处理许可，也未分发数据库整体。关联式和研究方法来源见 [参考文献](docs/references.md)。

截至本地整理日 2026-09-16，官方 [Student 页面](https://ansys.synopsys.com/academic/students/ansys-student)和 [Academic Usage](https://ansys.synopsys.com/legal/terms-and-conditions/academic-usage)描述教育学习及不同学术许可用途。项目没有据此声称“所有 Student 输出都禁止分享”，也没有声称已获 ANSYS 对本案例全部输出或 UDF 再许可的个别许可。具体安装时接受的 clickwrap 与个案权利范围不由项目开源许可证替代。

开源声明仅覆盖有权许可的项目贡献。对公共领域事实/数据不新增排他权，也不将第三方作品重新声明为项目原创。
