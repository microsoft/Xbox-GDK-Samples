# 简单播放声音流示例

*此示例兼容于 Microsoft 游戏开发工具包（2020 年 6 月）*

# 说明

此示例演示如何在 Xbox One 上使用 XAudio2 流式传输 wav 文件。

![](./media/image1.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Project Scarlett，请将可用解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* *GDK 文档中的*__运行示例__。&nbsp;

# 使用示例

除了通过&ldquo;视图&rdquo;按钮退出之外，该示例没有其他控件。

# 实现说明

此示例演示如何使用自己的 WAV 文件分析器流式传输 wav 文件。

有关使用 XAudio2 进行流式传输的其他示例，请参阅 [GitHub](https://github.com/walbourn/directx-sdk-samples/tree/master/XAudio2)：

- **XAudio2AsyncStream** 用于在磁盘上准备 .WAV 数据，以支持 Win 32 非缓冲重叠 I/O

- **XAudio2MFStream** 使用 Microsoft 媒体基础数据源阅读器来解压 WMA 文件的数据。

- *DirectX 工具包的* **SoundStreamInstance**，它为所有 XAaudio2 格式实现非缓冲重叠 I/O。

# 已知问题

此示例不支持流式处理 xWMA .wav 文件。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


