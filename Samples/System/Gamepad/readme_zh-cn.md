![](./media/image1.png)

# 游戏板示例

*此示例兼容于 Microsoft 游戏开发工具包（2020 年 6 月）*

# 说明

此示例演示了如何通过 Xbox One 的游戏手柄读取输入

![](./media/image3.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S，请将可用解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

对于电脑，可以将可用解决方案平台设置为 Gaming.Desktop.x64。 **这需要 2022 年 6 月 GDK 或更高版本**。

*有关详细信息，请参阅* *GDK 文档中的*__运行示例__。

# 使用示例

按下按钮可显示内容，移动控制杆和扳机可查看其读数。

# 实现说明

此示例演示如何使用新的 GameInput API 读取来自游戏手柄(包括控制杆和扳机键)的数据。

# 版本历史记录

- 2018 年 10 月: 初始 GDK 版本

- 2020 年 2 月: 针对 GameInput API 的更改进行了更新。

- 2022 年 6 月: 已添加对个人电脑上的 GameInput 的支持 (2022 年 6 月 GDK 或更高版本)

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


