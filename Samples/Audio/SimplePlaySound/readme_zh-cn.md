# 简单播放声音示例

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

# 说明

本示例演示如何在 Xbox One 上使用 XAudio2 播放 wav 文件。

![](./media/image1.png)

# 构建示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

有关详细信息，请参阅 GDK 文档中的"运行示例"。

# 使用示例

本示例只使用"视图"按钮执行退出操作，不涉及其他控件。当每个文件完成时，它会自动提前准备播放样本
wav 文件。

# 实现说明

本示例演示如何播放 PCM、ADPCM、xWMA 和 XMA2 格式的 wav 文件。它使用"ATG
工具包"文件 WAVFileReader.h/.cpp 中的帮助程序代码。这将实现简单的 wav
文件分析程序，以及用于计算支持的声音格式的播放时间的代码。

# 隐私声明

在编译和运行示例时，会将示例可执行文件的文件名发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。
