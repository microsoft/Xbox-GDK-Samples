![](./media/image1.png)

# AdvancedExceptionHandling 示例

*此示例可用于 Microsoft 游戏开发工具包（2020 年 6 月）*

# 说明

此示例演示处理游戏中可能发生的异常的几种高级方式。

- 使用单独的进程保存故障转储&ndash;演示如何使用单独的进程创建故障转储。 这是用于创建故障转储的建议模式。

- 将自定义数据添加到 Windows 错误报告&ndash;演示如何将数据添加到 Windows 错误报告系统，随后与故障转储一起上传到 Microsoft 服务器，以便进行后续分析。

- 上传故障转储&ndash;演示如何将故障转储上传到自己的服务器，让它们不会干扰游戏执行，避免导致更多异常。

- 在挂起/恢复 (PLM) 期间处理异常&ndash;演示如何处理 PLM 挂起/恢复路径中发生的异常。

- 完全异常系统&ndash;将所有部分整合到完整的异常系统中。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

如果使用桌面，请将可用解决方案平台设置为 `Gaming.Desktop.x64`。

*有关详细信息，请参阅* __运行示例__，*在 GDK 文档中。*

# 使用示例

对于每个演示，按控制器上的相应按钮。 屏幕将显示发生异常时代码中发生的操作顺序。

# 实现说明

所有示例都包含在 Examples 文件夹中。 其中详细介绍了每个系统及其工作原理。

# 更新历史记录

2021 年 6 月初始版本

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


