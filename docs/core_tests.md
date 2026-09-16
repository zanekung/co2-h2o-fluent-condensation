# 不启动 Fluent 的 C 核心测试

这些测试检查凝结核心函数、相变贡献及准一维初场的一致性。它们不检查 Fluent 挂接、二维迭代或实验准确性。测试源代码与原记录运行的开发测试对应；准一维 `.inc` 副本必须与本包 UDF 实现逐字节相同，入口会先检查。

需要已经配置好的 C 编译器及其系统 SDK/链接库。编译器必须显式指定，工具不会安装任何东西。GCC/Clang 风格示例：

```text
python tools/run_core_tests.py --cc gcc
python tools/run_core_tests.py --cc clang
```

MSVC/clang-cl 使用已配置开发环境，并指定 `--style msvc`；可重复提供 `--cflag=VALUE` 与 `--ldflag=VALUE`。例如 MSVC 风格的包含路径使用 `--cflag=/I/path`，链接路径使用 `--ldflag=/libpath:/path`。安装目录和 SDK 版本由使用者提供，没有写死在公开入口中。

程序在临时目录编译、运行两组测试，输出 JSON，最后清理临时产物。只有编译和两个测试退出码均为零才报告 PASS。缺少编译器、SDK 或 Windows 应用控制阻止执行，都不能记为测试通过；不应绕过安全策略。

历史测试结果与本轮实际结果分别保存在 `verification/recorded_*verification.json` 和 [public_core_test_check.json](../verification/public_core_test_check.json)。本轮若无法运行，记录会明确原因，不借用历史结果代替。
