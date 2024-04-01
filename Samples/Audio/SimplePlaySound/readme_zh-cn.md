# 简单播放声音示例

*此示例可用于 Microsoft 游戏开发工具包（2020 年 6 月）*

# 说明

此示例演示如何在 Xbox One 上使用 XAudio2 播放 wav 文件。

![](./media/image1.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Project Scarlett，请将可用解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* *GDK 文档中的*__运行示例__。&nbsp;

# 使用示例

除了通过&ldquo;视图&rdquo;按钮退出之外，该示例没有其他控件。 当每个文件完成时，它会自动浏览示例 wav 文件。

# 实现说明

此示例演示如何播放 PCM、ADPCM、xWMA 和 XMA2 格式 wav 文件。 它在 *ATG 工具包*文件 **WAVFileReader.h/.cpp** 中使用帮助程序代码。 这实现了一个简单的 wav 文件分析器，以及用于计算受支持声音格式的播放时间的代码。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


