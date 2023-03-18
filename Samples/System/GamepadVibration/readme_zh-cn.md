# 游戏手柄振动示例

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

# 

# 说明

本示例演示如何在 Xbox One 的游戏手柄上使用振动模式。

![](./media/image1.png)

# 构建示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

有关详细信息，请参阅 GDK 文档中的"运行示例"。

# 使用示例

使用向左和向右方向键在不同振动示例之间循环切换。
大部分示例都使用触发器来增加振动量。

# 实现说明

本示例演示如何使用 GameInput API 设置 Xbox One 游戏手柄的振动级别。

# 更新历史记录

初始发布：2019 年 4 月

2019 年 6 月更新了对 SetRumbleState 的次要中断性变更。

2020 年 2 月：更新了对 GameInput API 的更改。

# 隐私声明

在编译和运行示例时，会将示例可执行文件的文件名发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。
