# 简单 MSAA

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

![](./media/image1.png)

# 说明

本示例使用 DirectX 12 为 3D 场景实现 MSAA 呈现目标和深度/模具缓冲区。

# 构建示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

有关详细信息，请参阅 GDK 文档中的"运行示例"。

# 使用示例

| 操作                        |  游戏手柄                               |
|-----------------------------|----------------------------------------|
| 切换 MSAA 与单个采样        |  A 按钮                                 |
| 退出                        |  "视图"按钮                             |

# 实现说明

UI 不是使用 MSAA 绘制的，它利用显式解析，不依赖 MSAA 交换链的隐式解析。

# 隐私声明

在编译和运行示例时，示例可执行文件的文件名将发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。
