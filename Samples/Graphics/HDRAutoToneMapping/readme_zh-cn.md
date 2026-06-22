# HDR 自动音调映射示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

此示例对照渲染 SDR 图像本身的游戏，展示了 HDR 游戏如何使用 D3D 驱动程序的自动音调映射来生成 GameDVR 和屏幕截图的 SDR 图像。 游戏可以使用默认自动音调映射程序，也可提供自己的音调映射程序作为 3D 查找表。

![](./media/image1.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* __运行示例__，*在 GDK 文档中。*

# 使用示例

该示例使用以下控制。

| 操作 | 游戏板 |
|---|---|
| 更改自动音调映射方法 | A button |
| 将电视切换到 HDR 模式 | X button |
| 将音调映射器另存为 .DDS 文件 | Y button |
| 上一张图像 | 左侧肩按钮 |
| 下一张图像 | 右侧肩按钮 |

# Xbox One 的实现说明

Xbox One上的 HDR 游戏需要输出两个交换链，一个具有 10 位 HDR10 值呈现给 HDR 电视，另一个带有可用于 GameDVR、屏幕截图、流式传输和广播的 SDR 图像。 SDR 图像可以由游戏呈现，或者在使用交换链创建标志 D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP 启用自动音调映射时由 D3D 驱动程序自动呈现。 默认自动音调映射器生成良好的 SDR 图像，但很可能与游戏自身的音调映射 SDR 图像的外观不同。 API SetHDRToneMapperX() 使游戏有机会提供自己的音调映射器，以便 D3D 驱动程序的自动音调映射可以生成类似于游戏自己的 SDR 图像的图像。 可以安全地调用每个帧的 API，使游戏甚至可以在运行时生成 3D LUT，以便针对不同场景动态调整音调映射器。

有三种方法可以生成 SDR 映像：

1) 游戏呈现 SDR 图像。 这是建议的方法，因为这将导致最佳的映像质量和 GPU 性能。 缺点是它增加了渲染管道的额外复杂性，即额外的工程小时数来实现和维护，并且对于跨平台游戏来说可能是不需要的。 使用此方法，游戏将分配两个交换链，呈现 HDR 和 SDR 图像，然后使用 PresentX 呈现这两个图像。

2) D3D 驱动程序的默认自动音调映射。 此方法是最容易实现的方法。 游戏不知道 GameDVR 映像，D3D 驱动程序执行所有额外工作。 呈现管道没有额外的复杂性，但生成的图像很可能与游戏的 SDR 图像看起来不同。 使用此方法，游戏仅分配一个交换链，仅呈现 HDR 映像，但仍将 PresentX 与一个交换链一起使用。 D3D 驱动程序将从游戏内存中分配额外的交换链，并在 Present 调用期间注入计算着色器以生成 SDR 映像。

3) 使用游戏的音调映射器进行自动音调映射。 此方法与前面的方法类似，但游戏可以指定其自己的音调映射器，D3D 驱动程序将在自动音调映射期间使用该映射器。 此方法提供了两全其美的效果，呈现管道没有额外的复杂性，并且游戏仍然可以控制 SDR 图像的外观。

若要启用 HDR 自动音调映射，示例在创建交换链缓冲区时使用 `D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP` 资源创建标志。

示例启动时不会将电视切换到 HDR 模式，因为示例的目标是显示自动音调映射的 SDR 图像，而不是 HDR 图像。 按 X 按钮会将电视切换到 HDR 模式。

此示例演示如何使用游戏的音调映射器动态呈现 3D LUT，如何将其另存为 .DDS 文件，然后再次加载。 按 Y 按钮时，当前音调映射器将保存到游戏暂存文件夹，并可在此处访问：

请参阅白皮书&ldquo;[Xbox One 上的 HDR](http://aka.ms/hdr-on-xbox-one)&rdquo;和 Xfest 2018 演示文稿&ldquo;Xbox One 增强显示输出&rdquo;

# Xbox Series X 的实现说明

Xbox Series X 的显示输出硬件包括硬件 3D LUT。 这意味着游戏生成 SDR 映像不会产生 GPU 或带宽费用。 游戏仍然可以选择使用旧版 Xbox One 自动音调映射，该映射在 GPU 上使用计算着色器。

若要使用 Xbox Series X 硬件 3D LUT 使用系统自动音调映射器，游戏只需呈现 HDR10 交换缓冲区，并且*不应*指定任何标志。 若要使用旧版 Xbox One 自动音调映射，游戏 *应* 指定标志 `D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP`。请参阅 DeviceResources.cpp 中的代码

# 已知问题

无

# 更新历史记录

2020 年 2 月。
| | |
|---|---|
初始版本 2019 年 8 月。 添加对 Xbox Series X|S 的支持|

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


