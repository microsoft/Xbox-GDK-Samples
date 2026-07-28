![](./media/image1.png)

# SimpleDirectStorageCombo 示例

*此示例与 Microsoft 游戏开发工具包（2023 年 10 月）兼容*

# 说明

此示例演示如何使用 PIXBeginCapture API 的循环模式始终将性能数据记录到循环缓冲区中，然后按需将缓冲区保存到捕获文件。

# 生成示例

此示例支持以下平台

- Gaming.Scarlett.xbox.x64
- Gaming.XboxOne.xbox.x64

*有关详细信息，请参阅* *GDK 文档*中的__运行示例__。&nbsp;

# 使用示例

该示例将自动开始在循环缓冲区中捕获。 每 1000 帧，或者按下&ldquo;a&rdquo;按钮时，示例会将捕获保存到文件，然后重启捕获。 如果按下&ldquo;b&rdquo;按钮，将刷新缓冲区，但会放弃捕获。

# 实现说明

在 Xbox 上，API 生成必须通过 PIX UI 转换为计时捕获的&ldquo;pevt&rdquo;文件。

PIXEndCapture 的 Xbox 版本是异步的，返回 E_PENDING 直到捕获完全停止。

# 更新历史记录

初始版本 2023 年 10 月

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


