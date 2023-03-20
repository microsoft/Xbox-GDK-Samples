# SystemInfo 示例

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

# 说明

本示例演示用于查询系统信息和硬件功能的大量 API。

# 构建示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

如果在电脑上使用 Windows 10 2019 年 5 月更新（版本 1903，内部版本
18362）版本或更高版本，请将活动解决方案平台设置为 Gaming.Deskop.x64。

有关详细信息，请参阅 GDK 文档中的"运行示例"。

# 使用示例

本示例提供一系列包含技术信息的文本页面。

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

若要使用游戏手柄控制器在页面之间切换，请使用 A 按钮或向右方向键/B
按钮或向左方向键。

在键盘上，请使用向左键或 Enter 键/向右键或 BackSpace 键。

# 实现说明

关键代码是 Render 函数中的开关。

# 更新历史记录

2018 年 10 月：初始发布 GDK

2020 年 4 月 - 更新以支持 Gaming.Desktop.x64

# 隐私声明

在编译和运行示例时，会将示例可执行文件的文件名发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。
