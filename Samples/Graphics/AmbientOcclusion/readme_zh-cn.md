![](./media/image1.png)

# AmbientOcclusion 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

此示例演示了两种环境光遮挡技术：MiniEngine 的屏幕空间环境光遮挡 (SSAO) 和 Intel 的地面实况环境光遮挡 (GTAO)。 该示例还演示了一些着色器的 FP16 版本，这些着色器可在略微权衡质量的情况下提高性能。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* __运行示例__，*在 GDK 文档中。*

# 使用示例

左右控制杆可用于在场景中移动相机。 向上和向下 D-Pad 按钮选择当前选项。 左右 D-Pad 按钮可修改选项值。 A 按钮更改 AO 纹理和场景之间的显示。 B 按钮在 SSAO 和 GTAO 之间切换。 Y 按钮在场景呈现中打开或关闭 AO。 X 按钮在 FP16 和 FP32 着色器之间切换（仅限 Scarlett）。

![](./media/image2.bmp)

# 控件

| 操作 | 游戏板 |
|---|---|
| 仅切换 AO 呈现 | A button |
| 切换 SSAO/GTAO | B button |
| 打开/关闭 AO | Y button |
| 切换 FP16/FP32 着色器 | X button |
| 选择某个选项 | D-Pad 向上/向下 |
| 修改所选选项 | D-Pad 向右/向左 |
| 移动相机 | 左控制杆 |
| 旋转相机 | 右控制杆 |

# 实现说明

此示例演示了 2 种呈现环境遮挡的方法。

1. 屏幕空间环境遮挡 (SSAO)：SSAO 实现来自 [MiniEngine](https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/MiniEngine)。 它使用 HTile 直接从压缩的深度缓冲区读取数据，并创建向下采样版本。 然后，它会对深度纹理进行采样，并创建 AO 纹理的多个分辨率。 最后一个通道从 AO 纹理读取，向上采样并模糊处理它们。

2. 基本事实环境遮挡 (GTAO)：GPGA 实现来自 [Intel](https://github.com/GameTechDev/XeGTAO/tree/master)。 它首先向下采样深度缓冲区以创建 mip 链。 如果引擎尚未导出法线，则运行传递以从深度缓冲区生成法线。 然后，它从深度缓冲区运行到示例的主通道，以生成初始 AO 纹理和边缘纹理。 最后一次传递对 AO 纹理进行除扰。

这两种方法都支持 Scarlett 上的 FP16 着色器进行传递，与 FP32 版本相比，可改进性能。 FP16 的性能改进通常来自于占用率增加（由于将两个值打包到一个寄存器中）或更少的 VALU 操作（使用打包的数学指令）。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


