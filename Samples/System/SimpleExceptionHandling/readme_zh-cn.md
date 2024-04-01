![](./media/image1.png)

# SimpleExceptionHandling 示例

*此示例兼容于 Microsoft 游戏开发工具包(2020 年 6 月)*

# 说明

此示例演示处理游戏中可能发生的异常的几种不同方式。

- 未处理的异常筛选器 -- 演示如何使用&ldquo;[未经处理的异常筛选器](https://docs.microsoft.com/windows/win32/api/errhandlingapi/nf-errhandlingapi-setunhandledexceptionfilter)&rdquo;来捕获和处理游戏的常规异常。

- 结构化异常 -- 演示如何使用&ldquo;[结构化异常处理](https://docs.microsoft.com/cpp/cpp/structured-exception-handling-c-cpp)&rdquo;系统。

- 矢量异常处理程序 -- 演示如何使用&ldquo;[矢量异常处理](https://docs.microsoft.com/windows/win32/debug/vectored-exception-handling)&rdquo;系统。

- C++ 语言异常 -- 演示如何使用 [C++ 语言](https://docs.microsoft.com/cpp/cpp/try-throw-and-catch-statements-cpp)中内置的异常系统。

- 建议的模式 -- 演示使用其他系统组合的建议模式。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

如果使用桌面，请将可用解决方案平台设置为 `Gaming.Desktop.x64`。

*有关详细信息，请参阅* *GDK 文档中的*__运行示例__。

# 使用示例

对于每个演示，按控制器上的相应按钮。 屏幕将显示发生异常时代码中发生的操作顺序。

注意: 如果已附加调试程序，则&ldquo;未处理的异常筛选器&rdquo;示例的行为将有所不同，注释中有其他详细信息。

# 实现说明

所有示例都包含在 Examples 文件夹中。 其中详细介绍了每个系统及其工作原理。

# 更新历史记录

2021 年 4 月初始版本

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


