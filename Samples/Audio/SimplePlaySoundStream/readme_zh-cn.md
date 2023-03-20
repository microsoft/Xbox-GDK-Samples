# 简单播放声音流示例

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

# 说明

本示例演示如何在 Xbox One 上使用 XAudio2 流式处理 wav 文件。

![](./media/image1.png)

# 构建示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

有关详细信息，请参阅 GDK 文档中的"运行示例"。

# 使用示例

本示例只使用"视图"按钮执行退出操作，不涉及其他控件。

# 实现说明

本示例演示如何使用自己的 WAV 文件分析程序流式处理 wav 文件。

有关使用 XAudio2 进行流式处理的其他示例，请参阅
[GitHub](https://github.com/walbourn/directx-sdk-samples/tree/master/XAudio2)：

-   XAudio2AsyncStream 准备磁盘上的 .WAV 数据以支持 Win32 非缓冲重叠 I/O

-   XAudio2MFStream 使用 Media Foundation Source Reader 从 WMA
    文件解压缩数据。

# 已知问题

本示例不支持流式处理 xWMA .wav 文件。

# 隐私声明

在编译和运行示例时，会将示例可执行文件的文件名发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。
