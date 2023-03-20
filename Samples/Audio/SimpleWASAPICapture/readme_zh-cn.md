# 简单的 WASAPI 捕获示例

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

# 说明

本示例演示如何在 Xbox One 上使用 WASAPI 捕获音频。

![Sample Screenshot](./media/image1.png)

# 构建示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

有关详细信息，请参阅 GDK 文档中的"运行示例"。

# 使用示例

使用游戏手柄选择捕获设备。 本示例自动使用默认音频呈现器。
请注意，捕获与呈现之间无采样率转换，因此除非速率匹配，否则播放的声音将不正确。

# 实现说明

本示例演示如何在使用 WASAPI 捕获音频。
捕获的样本将放在一个循环缓冲区中，然后用于呈现样本。
本示例还在呈现器与捕获之间使用共享的 WASAPI 实例。 有关 WASAPI
的更高级用法，请参阅 [Windows WASAPI
示例](https://code.msdn.microsoft.com/windowsapps/Windows-Audio-Session-22dcab6b)

# 更新历史记录

初始发布：2019 年 5 月

# 隐私声明

在编译和运行示例时，会将示例可执行文件的文件名发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。
