# XAPU 简单示例

*此示例与 Microsoft 游戏开发工具包预览版（2020 年 11 月）兼容*

# 说明

此示例演示如何使用 Scarlett 上的 XAudio2 流式传输 OPUS/OGG 文件。

![](./media/image1.png)

# 生成示例

此示例仅适用于 Project Scarlett。

*有关详细信息，请参阅* *GDK 文档中的*__运行示例__。&nbsp;

# 使用示例

除了通过&ldquo;视图&rdquo;按钮退出之外，该示例没有其他控件。

# 实现说明

本示例演示如何使用基本 OGG 解析来流式传输 Opus 文件。

有关使用 XAudio2 进行流式传输的其他示例，请参阅 [GitHub](https://github.com/walbourn/directx-sdk-samples/tree/master/XAudio2)：

- **XAudio2AsyncStream** 用于在磁盘上准备 .WAV 数据，以支持 Win 32 非缓冲重叠 I/O

- **XAudio2MFStream** 使用 Microsoft 媒体基础数据源阅读器来解压 WMA 文件的数据。

# 已知问题

此示例不支持 OGG 元数据帧。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


