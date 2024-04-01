# 简单设备和交换链示例

*此示例可用于 Microsoft 游戏开发工具包 （2022 年 3 月）*

# 说明

此示例演示如何为 Xbox 游戏创建 Direct3D 12 设备和 PresentX 交换链。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox One X|S 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅*__运行示例__，详见*GDK 文档。*

# 使用示例

该示例除了退出之外，没有其他控制。

# 实现说明

尽管 Xbox 游戏 Direct3D 设置与其他 Microsoft 平台非常相似，但此示例演示了一些主要区别：

- 使用 **D3D12XboxCreateDevice** 而不是标准 D3D12CreateDevice

- 将 4k 用于 Xbox Series X/Xbox One X，将 1440p 用于 Xbox Series S，将 1080p 用于 Xbox One

- 这将使用新的 **PresentX** API，而不是使用 DXGI 进行演示

有关 Direct3D 12 设备创建和交换链的最佳做法的详细信息，请参阅对 [Direct3D 12 Create Device 的剖析](https://walbourn.github.io/anatomy-of-direct3d-12-create-device/)以及[现代交换链的保养和馈送](https://walbourn.github.io/care-and-feeding-of-modern-swapchains/)。

有关使用循环计时器的详细信息，请参阅 [StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。

# 更新历史记录

2018 年 10 月 -- Microsoft GDK 的初始版本

2019 年 10 月 -- 切换到 XSystemGetDeviceType 以进行控制台检测

2021 年 10 月 -- 更新为在 Xbox Series S 上使用 1440p

2022 年 8 月 -- 针对在何处等待原点事件改进了 PresentX 最佳做法。


