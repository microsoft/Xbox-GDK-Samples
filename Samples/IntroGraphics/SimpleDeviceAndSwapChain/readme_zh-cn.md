# 简单的设备和交换链示例

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

# 说明

此示例演示如何创建适用于 Xbox One 应用的 Direct3D 12 设备和 PresentX
交换链。

# 构建示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

有关详细信息，请参阅 GDK 文档中的"运行示例"。

# 使用示例

本示例只使用退出控件，不涉及其他控件。

# 实现说明

虽然 Xbox One 应用的 Direct3D 设置与其他 Microsoft
平台非常相似，但此示例展示了几个关键区别：

-   使用 D3D12XboxCreateDevice，而不是标准 D3D12CreateDevice

-   使用 4K 本机交换链与1080p

-   这将使用新的 PresentX API，而不是使用 DXGI 进行演示。

有关 Direct3D 12 设备创建的最佳做法的详细信息，请参阅《[Direct3D 12
创建设备剖析](https://walbourn.github.io/anatomy-of-direct3d-12-create-device/)》。

有关如何使用循环计时器的详细信息，请参阅《[StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)》。

# 隐私声明

在编译和运行示例时，示例可执行文件的文件名将发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。
