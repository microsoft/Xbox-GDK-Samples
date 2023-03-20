# 简单的 WASAPI 播放声音示例

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

# 说明

本示例演示如何在 Xbox One 上播放设置并向 WASAPI
呈现器终结点播放简单的声音（正弦音）。

![](./media/image1.png)

# 构建示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

有关详细信息，请参阅 GDK 文档中的"运行示例"。

# 使用示例

使用键盘上的空格键或游戏板上的按钮 A 开始和停止播放。 使用键盘上的 ESC
键或使用"视图"按钮退出应用。

# 实现说明

有关 WASAPI 的详细信息，请参阅
[MSDN](https://msdn.microsoft.com/en-us/library/windows/desktop/dd371455.aspx)。

# 隐私声明

在编译和运行示例时，示例可执行文件的文件名将发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。
