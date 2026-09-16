# 方法来源与引用边界

以下方法文献在原始建模阶段已通过文献全文核对；本次案例整理保留书目信息和用途，不分发文献 PDF。它们不是对本案例全部工况的定量验证。

1. Ding et al. (2014). *Non-equilibrium condensation of water vapor in sonic nozzle*. Applied Thermal Engineering 71, 324–334. [DOI](https://doi.org/10.1016/j.applthermaleng.2014.07.008)。参考气相方程与 UDS/UDF 的耦合方法；其湿空气/湿氮气研究不等同于 CO₂–H₂O 工况，UDS 扩散处理也与本案例不同。
2. Yang et al. (2017). *CFD modeling of condensation process of water vapor in supersonic flows*. Applied Thermal Engineering 115, 1357–1362. [DOI](https://doi.org/10.1016/j.applthermaleng.2017.01.047)。参考 ICCT、Gyarmathy 及液相输运路线；必须独立统一 J 与 N 的单位及质量基准，不能直接移植原实验作为当前混合气验证。
3. Han et al. (2017). *Coupled Model of Heat and Mass Balance for Droplet Growth in Wet Steam Non-Equilibrium Homogeneous Condensation Flow*. Energies 10, 2033. [DOI](https://doi.org/10.3390/en10122033)。用于理解增长模型选择与适用区间，不证明当前 CO₂ 载气传质近似已验证。
4. Ivanov et al. (2022). *Application of the Moment Method for Numerical Simulation of Homogeneous-Heterogeneous Condensation*. Fluids 7, 68. [DOI](https://doi.org/10.3390/fluids7020068)。矩输运方法参考；本案例的两个矩和代表半径不等于完整液滴分布模型。

喷管曲线来源记录：[Case Studies in Thermal Engineering, DOI 10.1016/j.csite.2023.103089](https://doi.org/10.1016/j.csite.2023.103089)。本案例具体使用的几何表达式以 [model.md](model.md) 与网格生成代码为准。

物性关联式来源记录：[IAPWS saturation release](https://www.iapws.org/relguide/Supp-sat.html)、[IAPWS surface tension release](https://iapws.org/documents/release/Surf-H2O.download)。关联式适用区间与低温外推仍需检查。

软件文档用于理解接口和源项，不随包复制：[Fluent UDF 手册](https://ansyshelp.ansys.com/public/Views/Secured/corp/v261/en/flu_udf/flu_udf_ModelSpecificDEFINE.html)、[Fluent 质量转移源项](https://ansyshelp.ansys.com/public/Views/Secured/corp/v242/en/flu_th/flu_th_sec_mf_mass_transf_source.html)。部分文档访问可能需要相应账号或许可。
