# Fluent 手动设置与核对说明

本说明整理的是实际计算所依据的模型源码及设置记录，用于核对已加载的 Case，或在另一个工作目录从网格手动搭建同一候选模型。它不提供自动准备入口，也不要求查看者重新计算。

历史完整计算使用 Windows、Fluent 2026 R1 Student、二维双精度、4个CPU进程，完成100000个时间步至0.01 s。该运行被限定为学习及软件/方法验证，不能当作论文用科研许可重算，也未完成实验、网格或时间步独立性验证。**Fluent 2024 R2 未经实机验证。** 本说明依据历史源码与保存设置整理；本次公开副本的读取验证在启动阶段受阻，详见下文。界面菜单名称可能随版本变化。

从建目录、点击 Build/Load 到正式 Calculate 的完整操作已经写入 [README 的 Fluent 设置章节](../README.md#fluent-manual-run)。本文件保留字段定义与来源细节。

## 1. 查看现有 Case/Data：先保留结果

1. 按本仓库原生文件说明选择配套的 Case 与 Data；在 Fluent 中先读 Case，再读同一导出时刻的 Data。Case 保存设置/网格，Data 保存场值，不能混用不同版本的配对文件。
2. 仅查看既有结果时，**不要按 Initialize、不要执行准一维初场函数、不要开始 Calculate**。这些操作会改变或重置当前场。原始完整结果应显示时间步100000、物理时间0.01 s；初始准备场则为0步、0 s，二者用途不同。
3. 本公开原生文件保留原计算的全部UDF挂接与设置，仅调整路径；没有另提供移除UDF挂接的查看版。配套文件及读取顺序见 [native/README.md](../native/README.md) 和 [操作教程](tutorial.md)。即使成功读入最终Case/Data，也不要执行初始化或准一维初猜，以免覆盖已保存结果。
4. `native/` 中配有三个C源文件与一个头文件，用于在当前Fluent版本下重建 `libfuel`。库名字出现在Case中不等于已经成功加载；自动重建及公开副本重读仍需实际验证，不应借用另一版本的DLL。**本次尝试启动Fluent时，Windows Code Integrity拦截了 `Qt5Widgets.dll`，尚未实际读入打包后的公开Case/Data副本。** 因此不能把历史原工程通过记录写成公开副本已经测试通过。
5. 本说明未显式设置的选项，不应凭推测补成“原始确定值”。仅核对时以保存的 Case 为准；从网格搭建时还需对照目标版本默认选项及重新读取后的设置。

## 2. 几何、网格与通用模型

| 项目 | 原计算设置 |
|---|---|
| 空间/精度 | 2D、Axisymmetric、Double Precision；计算域为上半平面 |
| 单元 | 30000个四边形单元，400个轴向×75个径向单元；30476个节点 |
| 长度 | 收缩段0.100 m，扩张段0.150 m，无入口直段 |
| 半径 | 入口0.01995 m，喉部0.00645 m，出口0.00775 m |
| 流体区 | `fluid-domain` |
| 边界 | `inlet`：pressure-inlet；`outlet`：pressure-outlet；`wall`：wall；`axis`：axis |
| 内面 | `interior-fluid`、`throat-partition`，均为内部面，不是额外固壁或边界条件 |
| Operating Pressure | 0 Pa；所以本案例所填压力数值也等于绝对压力 |
| Gravity | 关闭 |
| Energy | 开启 |
| Viscous | k-omega SST；Wall Omega Treatment为correlation |
| Species | Species Transport，H2O/CO2二元气体；未设置化学反应 |
| 正式计算求解器 | Pressure-Based、Transient；压力速度耦合PISO |

网格是程序生成的结构化网格；读取后应执行 Mesh Check。上下镜像只用于显示，不要把显示镜像当作又增加一倍物理单元或质量。历史搭建过程曾暂设密度基选项，随后切换压力基完成标准初始化与干气预处理；正式凝结计算的最终配置是压力基PISO。

## 3. 材料与组分

流体区指定 `mixture-template`，组分顺序必须是 **`h2o`在前，`co2`为最后组分**。UDF通过species-0读取水蒸气，因此不能交换顺序。

| 物性 | 设置与单位 |
|---|---|
| 混合物密度 | Ideal Gas |
| 混合物比热 | Mixing Law |
| 混合物黏度 | Ideal-Gas-Mixing-Law |
| 混合物导热系数 | Ideal-Gas-Mixing-Law |
| H2O分子量 | 18.01528 kg/kmol |
| CO2分子量 | 44.00950 kg/kmol |
| H2O定压比热 | 常数1864 J/(kg·K) |
| H2O黏度 | 常数1.34×10⁻⁵ Pa·s |
| H2O导热系数 | 常数0.0261 W/(m·K) |
| CO2黏度 | 常数1.37×10⁻⁵ Pa·s |
| CO2导热系数 | 常数0.0145 W/(m·K) |
| CO2定压比热 | 下表的温度分段多项式，单位J/(kg·K) |

CO2比热以温度T（K）为自变量，依次输入常数项、T、T²、T³、T⁴项，即 `cp=a0+a1*T+a2*T²+a3*T³+a4*T⁴`：

| 温度范围/K | a0 | a1 | a2 | a3 | a4 |
|---|---:|---:|---:|---:|---:|
| 300–1000 | 429.92889 | 1.8744735 | −0.001966485 | 1.2972514×10⁻⁶ | −3.9999562×10⁻¹⁰ |
| 1000–5000 | 841.37645 | 0.59323928 | −0.00024151675 | 4.5227279×10⁻⁸ | −3.1531301×10⁻¹² |

源码中虽然还包含 `co2h2o_h2o_cp`、`co2h2o_co2_cp` 两个可编译比热函数，**原设置并没有把它们挂到材料**。特别是后者返回846常数，不能用它替代上述CO2分段多项式。液体在本模型中由两个UDS表示，没有另建Eulerian液相材料或开启另一套Wet Steam模型；不要在这些源项之外再叠加一套相变质量传递。

最终Case还保留以下关键继承值，原配置代码未主动修改它们，不能统一称为非默认改项：

| 核对项 | 保存值 |
|---|---|
| 气相Mass Diffusivity | constant-dilute-appx，2.88e-5 m²/s |
| Turbulent Schmidt Number | 0.7 |
| 气相Thermal Diffusion、Full Multicomponent Diffusion、Inlet Diffusion | 均关闭 |
| Diffusion Energy Source | 开启，`energy/species-diffusion?=#t` |
| Viscous Heating | 关闭，`viscous-energy-dissipation?=#f` |
| 焓参考温度 | 298.15 K |

气相扩散与UDS扩散是不同设置。核对来自最终Case的记录见 [README设置审阅记录](../verification/readme_settings_review.json)，不使用中途密度基快照推定正式PISO的全部默认值。

## 4. 边界条件

入口摩尔组成是H2O 0.2、CO2 0.8。Fluent原设置按质量分数输入，水蒸气值为：

`0.2×18.01528 / (0.2×18.01528+0.8×44.00950) = 0.09283677142689885`。

因此不能在质量分数框中填0.2。

| 边界/项目 | 原设置 |
|---|---|
| inlet：Gauge Total Pressure | 500000 Pa |
| inlet：Supersonic/Initial Gauge Pressure | 498500 Pa |
| inlet：Total Temperature | 390 K |
| inlet：组分基准 | 质量分数，H2O=0.09283677142689885，CO2为余量 |
| inlet：湍流指定方式 | Intensity and Hydraulic Diameter |
| inlet：Turbulent Intensity (%) | GUI填 **1**（1%）；API/Case比例值为0.01 |
| inlet：Hydraulic Diameter | 0.0399 m |
| inlet：UDS0、UDS1 | Specified Value，两者均0 |
| outlet：Gauge Pressure | 100000 Pa |
| outlet：Backflow Total Temperature | 390 K |
| outlet：回流H2O质量分数 | 0.09283677142689885 |
| outlet：回流湍流指定方式 | Intensity and Hydraulic Diameter |
| outlet：Backflow Turbulent Intensity (%) | GUI填 **1**（1%）；API/Case比例值为0.01 |
| outlet：Backflow Hydraulic Diameter | 0.0155 m |
| outlet：UDS0、UDS1 | Specified Flux，两者均0 |
| wall：动量 | No Slip |
| wall：热边界 | Heat Flux=0 W/m²，即绝热 |
| wall：UDS0、UDS1 | Specified Flux，两者均0 |
| axis | 保持轴对称边界 |

**湍流强度单位说明：**已结合原源码、最终Case与 [ANSYS官方PyFluent示例](https://fluent.docs.pyansys.com/version/stable/examples/00-fluent/species_transport.html)核对：示例将10%对应为API值0.1，本案例的比例0.01因此对应GUI **1%**。在标有(%)的输入框填1，不填0.01；这项单位核对不代表本次已重新打开Fluent或测试2024 R2。

未主动改写但保留在最终Case中的边界核对项：入口及出口回流为Absolute/Normal to Boundary；出口回流压力类型为Total Pressure；两边Prevent Reverse Flow=False；壁面静止、粗糙度高度0，H2O为Specified Flux (Mass)=0。原值一致时无需再改。

## 5. 两个UDS、26个UDM与UDF编译

先分配 **2个User-Defined Scalars、26个User-Defined Memory Locations**，再加载UDF，使自定义名称能正确登记。两个UDS均选择所有流体区、Mass Flow Rate通量、Default Unsteady项。

上述UDS选项在历史legacy TUI路径中是分配后继承并核对的值，不宣称全部是非默认设置。普通UDM为26、Node Memory为0；UDS Inlet Diffusion在最终Case中保持开启，与气相Species入口扩散关闭不同。完整编译按钮顺序和命令见 [README](../README.md#fluent-manual-run)。

| UDS索引 | 定义 | 原自定义名称 |
|---|---|---|
| 0 | 液水负载L，kg液水/kg气体；不是湿混合物液相质量分数 | `liquid-loading-kg-liquid-per-kg-gas` |
| 1 | 液滴数N（#/kg气体）除以10¹⁵ | `droplet-number-per-kg-gas-div-1e15` |

实际湿度诊断β为 `max(L,0)/(1+max(L,0))`，存于UDM18；实际数密度为气体密度乘 `max(UDS1,0)×10¹⁵`，存于UDM19。查看原始UDS时保留其负尾值，不能用诊断中的非负处理掩盖数值误差。

在新工作目录中放置网格、三个C源文件和头文件，使用当前Fluent内置编译器建立名为 `libfuel` 的库：

- C源文件：`co2_h2o_condensation.c`、`condensation_core.c`、`quasi1d_initialization.c`。
- 头文件：`condensation_core.h`。

这些文件在仓库的 `udf/` 中。不要只编译外层文件而漏掉核心或准一维文件。历史Windows二维双精度并行库同时有host和node部分；不随资料复制另一台机器的编译物。网格、源码与生成库最好位于同一ASCII工作目录，并从该目录启动工作会话，避免Case内相对库名在重读时失效。

### 5.1 七个流体区源项

在 `fluid-domain` 的Source Terms开启源项。每个方程仅添加下面这一项；库名后缀在Fluent界面通常显示为 `::libfuel`。

| 方程 | 选择的函数 | 作用 |
|---|---|---|
| Mass | `co2h2o_mass_source::libfuel` | 气相质量源−Γ |
| X/Axial Momentum | `co2h2o_xmom_source::libfuel` | 轴向动量源−Γu |
| Y/Radial Momentum | `co2h2o_ymom_source::libfuel` | 径向动量源−Γv |
| Energy | `co2h2o_energy_source::libfuel` | 返回同次诊断缓存的能量源，并使用缓存温度Jacobian |
| h2o / species-0 | `co2h2o_h2o_source::libfuel` | 水蒸气质量源−Γ |
| UDS-0 | `co2h2o_liquid_source::libfuel` | 液水负载方程源+Γ |
| UDS-1 | `co2h2o_number_source::libfuel` | 成核与消亡液滴数源，按10¹⁵缩放 |

CO2为最后组分，不另加相变源项。不要另外手填潜热源；现有能量UDF已经按本候选模型的质量传递焓闭合处理。

### 5.2 扩散及全局函数挂接

| Fluent位置/用途 | 函数 | 备注 |
|---|---|---|
| Mixture材料的UDS Diffusivity，全局User-Defined | `co2h2o_uds_diffusivity::libfuel` | 两UDS使用 `μt/0.9+1e-12`；仅设置不活动的单UDS子项可能并未真正覆盖全局默认值 |
| Function Hooks → Initialization | `co2h2o_init::libfuel` | 初始化时将两个UDS及26个UDM清零 |
| Function Hooks → Adjust | `co2h2o_adjust::libfuel` | 每次求解调整阶段计算同一组源项/诊断缓存 |
| Function Hooks → Execute At End | `co2h2o_refresh_at_end::libfuel` | 在求解结束阶段刷新诊断 |
| 库加载时自动执行 | `co2h2o_on_loading` | 登记字段名称；无需加入初始化/adjust列表 |
| Execute On Demand：诊断刷新 | `co2h2o_refresh_diagnostics::libfuel` | 从当前场重算诊断，不推进物理时间；会更新UDM，保留原始结果时不要无目的重复操作 |
| Execute On Demand：干气初猜 | `co2h2o_quasi1d_initial_guess::libfuel` | 仅用于新模型初始准备；会改压力/温度/速度并清零UDS，不用于打开已有结果 |

加载成功后应在对应下拉列表看到函数。若从Case重读后只剩默认 `uds-0`/`uds-1` 名称，先检查是否在加载库前分配了UDS/UDM。历史测试中，自定义UDS未正确登记时某些原生ASCII导出发生过崩溃；不能仅凭索引别名存在就认定恢复过程正常。

### 5.3 UDM索引对照

| 索引 | 字段含义 | 单位/解释 |
|---:|---|---|
| 0 | H2O过饱和度S | 无量纲 |
| 1 | 露点温度 | K |
| 2 | 过冷度 | K |
| 3 | 实际施加成核率J | m⁻³·s⁻¹ |
| 4 | 临界半径 | m |
| 5 | 液滴半径 | m |
| 6 | 液滴增长率 | m/s |
| 7 | 未限幅凝结质量源 | kg/(m³·s) |
| 8 | 实际施加凝结质量源Γ | kg/(m³·s) |
| 9 | 液态水密度 | kg/m³ |
| 10 | 表面张力 | N/m |
| 11 | 理想逸度系数 | 固定1 |
| 12 | 理想压缩因子Z | 固定1 |
| 13 | 质量源限幅标志 | 诊断标志 |
| 14 | 模型风险标志 | 包括任何负原始UDS等，不等于物理失效体积分数 |
| 15 | 原始成核率 | m⁻³·s⁻¹ |
| 16 | 实际正凝结量 | kg/(m³·s) |
| 17 | 实际蒸发量 | kg/(m³·s) |
| 18 | 液相质量分数β | 无量纲，乘100才是百分数 |
| 19 | 液滴体积数密度 | m⁻³ |
| 20 | 空液滴数消亡项 | m⁻³·s⁻¹ |
| 21 | 成核率上限标志 | 诊断标志 |
| 22 | 实际出生液滴质量源 | kg/(m³·s) |
| 23 | 实际增长质量源 | kg/(m³·s) |
| 24 | 能量源温度Jacobian | W/(m³·K) |
| 25 | 能量源缓存 | W/m³ |

源码将成核修正系数设为1.0，最大成核率为10³⁵ m⁻³·s⁻¹，单步最大蒸汽消耗比例为0.10；没有为拟合旧图而施加整体经验倍率。半径和源项有源码规定的保护/限幅，应查看相应诊断字段。这些设置是候选模型定义，不是已经完成物理验证的证据。

## 6. 从网格搭建时：干气准备与正式凝结分开

本节只用于新的复算工程，不用于查看已经保存的最终结果。

### 6.1 冻结组分干气初场

1. 完成上述材料、边界、UDS/UDM、编译和初始化函数挂接后，暂时关闭流体区Source Terms，暂时清空Adjust及Execute At End函数列表，关闭两条UDS方程。
2. 选择Pressure-Based、Steady、SIMPLE。先做Standard Initialization；默认场明确填压力498500 Pa、温度390 K、H2O质量分数0.09283677142689885、轴向速度20 m/s、径向速度0，两UDS为0。标准初始化用于建立一致的初始/历史存储，不应省略后直接跨求解器补写内部场。
3. 执行一次 `co2h2o_quasi1d_initial_guess::libfuel`，得到变比热准一维干气初猜。它覆盖压力、温度、密度、速度、可用焓存储、组分及湍流初场，并清零UDS/UDM；其中 `k=1.5×0.01²×speed²`、`omega=√k/(0.09^0.25×0.07×2R)`。这不是凝结结果，也不是把标准初始化的均匀k/omega值原样保留到正式计算。
4. Pressure离散选Standard；Density、Momentum、k、omega、Temperature、species-0选First-Order Upwind。Temperature与species-0松弛因子均0.9；请求最多500次稳态迭代。
5. Pressure改Second Order；上述对流项改Second-Order Upwind；再请求最多1000次稳态迭代。若Fluent自动提前收敛，实际次数可以低于1500，不能把“请求上限”写成必定完成的迭代数。
6. 将干气初场另存为一对Case/Data，物理时间仍为0 s。这一阶段水蒸气存在，但冻结组分、不做相变源计算。

### 6.2 正式凝结瞬态的最终设置

恢复流体区全部七项源、两条UDS方程、Adjust及Execute At End挂接，并设：

| 项目 | 值 |
|---|---|
| Solver/Time | Pressure-Based / Transient |
| Pressure–Velocity Coupling | PISO |
| Pressure空间离散 | Second Order |
| Density、Momentum、k、omega、Temperature、species-0、UDS0、UDS1 | Second-Order Upwind |
| 时间离散 | Second-Order Implicit，即原设置的unsteady-2nd-order |
| UDS0、UDS1松弛因子 | 各0.3 |
| Temperature松弛因子 | 0.3 |
| species-0松弛因子 | 0.5 |
| 固定时间步 | 1×10⁻⁷ s |
| 计划时间步数 | 从0 s开始100000步，总时长0.01 s |
| 每时间步最大内迭代数 | 250 |
| Residual Absolute Criteria：Energy、h2o、UDS0、UDS1 | 1×10⁻⁶ |
| 其余已列残差方程 | 1×10⁻⁴ |

历史基础设置中还记录过Courant Number=5；这不改变最终压力基PISO选择，不应据此改回密度基求解器。未在上述源码中显式指定的PISO校正次数、梯度方法及其他松弛选项，应读取原Case核对，不能把本说明未记录的默认值视为跨版本恒定。

执行一次诊断刷新，确认物理时间0 s，再保存为初始ready Case/Data。重新读取这对文件时确认时间、UDS/UDM数量、源项、函数挂接、PISO、时间步和监控设置保留。**Ready只说明准备场与设置已经保存，不代表正式计算已完成或已通过数值验证。** 若决定开始新计算，再由操作者在Run Calculation点击Calculate；本说明不自动执行该动作。

若打开的是0.01 s完整结果，不要再把“100000”当剩余步数重复运行。若确需继续某个中途检查点，先确认保存的时间步索引，再计算所需剩余步数，并核对恢复后的诊断名称、库与输出路径。

## 7. 监测、检查点与原生输出

### 7.1 27项Report Definitions

表面报告：

| 报告名 | 类型 | 位置/变量 |
|---|---|---|
| gas-inlet | Mass Flow Rate | inlet |
| gas-outlet | Mass Flow Rate | outlet |
| outlet-water | Mass-Weighted Average | outlet，h2o |
| outlet-co2 | Mass-Weighted Average | outlet，co2 |
| outlet-loading | Mass-Weighted Average | outlet，UDS0 |
| outlet-beta | Mass-Weighted Average | outlet，UDM18 |
| outlet-radius | Mass-Weighted Average | outlet，UDM5 |
| outlet-temp | Mass-Weighted Average | outlet，temperature |
| wall-yplus-max | Facet Maximum | wall，y-plus |

以下18项均针对 `fluid-domain`：

| 报告名 | 类型 | 变量 |
|---|---|---|
| max-s | Maximum | UDM0 |
| max-j | Maximum | UDM3 |
| max-j-raw | Maximum | UDM15 |
| max-radius | Maximum | UDM5 |
| max-beta | Maximum | UDM18 |
| min-temp / max-temp | Minimum / Maximum | temperature，各一项 |
| min-h2o | Minimum | h2o |
| min-loading | Minimum | UDS0 |
| min-number | Minimum | UDS1 |
| limiter-volume-fraction | Volume Average | UDM13 |
| risk-volume-fraction | Volume Average | UDM14 |
| phase-transfer | Volume Integral | UDM8 |
| positive-condensation | Volume Integral | UDM16 |
| evaporation | Volume Integral | UDM17 |
| liquid-inventory | Mass Integral | UDS0 |
| vapor-inventory | Mass Integral | h2o |
| co2-inventory | Mass Integral | co2 |

`physics-history`报告文件包含全部27项，并开启Write Instantaneous Values。**2026 R1实际读回是每1个时间步写入**；旧设置过程先填10，开启瞬时值后被Fluent强制为1，所以不能照旧文字宣称每10步。`outlet-liquid`报告图只画`outlet-beta`，显示频率每10步。在目标版本核对实际报告文件频率，而不是只看曾经输入的值。

流量报告保留Fluent原生符号约定；做守恒重构时同时考虑气体流量、液水负载、库存变化和相变源，不能仅把入口与出口气相流量的差视为总质量不守恒。严格risk标志可能由近零负UDS尾值触发；最大半径也可能受近零液量/数目比值影响，应结合库存量级判断。

### 7.2 自动保存与结束导出

- 自动保存Data频率：每500个时间步；每次配套保存Case；保留所有检查点，不启用“只保留最近文件”。文件名追加时间步号。
- 输出目录由操作者选择，避免沿用别人机器的路径。保存Case/Data可能改变Fluent自动保存根文件名；保存并重读后再次核对检查点目录和根名。
- 原完整工作流在一次Calculate结束时执行原生导出：1个Case、1个Data、1个全域CSV、6张PNG。结束触发也会在短测或中断返回时出现，所以存在`final`文件不等于已到100000步；应读取时间步和物理时间确认。
- 如手动重新设置Calculation Activities中的结束命令，应先填写本机有效的导出命令/Journal路径，再启用Execute At End。公开原生文件保留完整挂接与设置并调整路径；实际输出位置及读取要求见 [native/README.md](../native/README.md)，不要重新填写原机器的绝对路径。
- 不同时间步可追加步号避免互相覆盖；同一步再次导出仍可能覆盖，建议使用新名称或新目录。

### 7.3 六幅原生Contour

| 原对象名 | Fluent字段 | 原生数值单位 |
|---|---|---|
| fig2-a-pressure | absolute-pressure | Pa |
| fig2-b-temperature | temperature | K |
| fig2-c-supersaturation | UDM0 | 无量纲 |
| fig2-d-nucleation | UDM3，实际施加成核率 | m⁻³·s⁻¹ |
| fig2-e-liquid-fraction | UDM18，β | 无量纲 |
| fig2-f-droplet-radius | UDM5 | m |

选择整个二维单元域，而不是只选择内部面线；Filled、Node Values、Smooth开启，Contour Lines关闭，使用自动全局线性色限；沿axis镜像显示。历史单图原生PNG分辨率为2600×800、横向、彩色。Node Values会造成节点插值显示极值与单元中心CSV极值不同，不能把色标读数直接当作CSV最大值。

原始六张输出未使用论文拼图的半径筛选。后续展示若采用 `L>1e-12且N>0`、0–150 nm色限，必须在图注说明该筛选，且它只是另一个后处理显示定义；不要修改保存的原始UDS/UDM。Pa转kPa、β转百分数、m转nm也应明确单位换算。

### 7.4 全域CSV

使用Fluent原生ASCII导出，选择全物理域、逗号分隔、**Cell-Centered**。输出8项气相字段（绝对压力、温度、密度、轴向速度、径向速度、Mach、h2o、co2）、两个UDS、全部26个UDM，共36个场字段；历史输出连同单元定位列共39列、30000行单元数据。不为镜像显示再复制下半域的数据行，不删除负值、risk或limiter字段。

## 8. 阅读结果与重算前的最低核对

- 当前Data与Case配对，时间索引正确；仅查看时保持保存场不被初始化或初猜覆盖。
- 当前模型确有2 UDS、26 UDM，species-0是h2o；若继续计算，全部七项源、扩散与全局挂接恢复且库实际加载成功。
- 正式计算为压力基PISO，dt=1e-7 s；干气阶段的SIMPLE/steady/关闭相变不能遗留到正式阶段。
- 初始场与已有最终场用途分明；完成100000步、残差满足阈值只是数值检查的一部分，还需检查守恒、漂移、负UDS量级、源项限制、网格和时间步敏感性。
- 本手册记录候选模型的可核对设置，不将软件运行完成、库编译成功或图像导出成功等同于论文结果已获验证。

## 来源与整理边界

设置来源为历史 `rebuild_model.py`、`prepare_and_open.py`、`configure_outputs.py`，并与仓库 `udf/` 中的实际C函数名称核对。三个历史源文件的SHA-256依次为：

- `7cb5c10e4eae733b23b0144cf04911818d78bf516fe4f225d66e1fe658f1d85d`
- `5971ee4ff34cae27301f129dba57acef3e1fae0436c4ab3ca28a2de950584f8a`
- `7bb2731e1130b38f3b86cea82a93f8652e209faba2517b5fa623efbbe02b988b`

历史完整运行的report读回用于纠正history频率说明。本文未更改物理设置，也没有把未核实的GUI显示单位、默认设置或目标版本兼容性补写成已确认事实。
