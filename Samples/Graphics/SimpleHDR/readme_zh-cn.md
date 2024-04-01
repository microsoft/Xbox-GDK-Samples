# SimpleHDR 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

这是一个简单示例，演示如何在 Xbox 上实现 HDR。

![自动生成的形状描述](./media/image1.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Project Scarlett，请将可用解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* *GDK 文档中的*__运行示例__。&nbsp;

# 使用示例

该示例使用以下控制。

| 操作 | 游戏板 |
|---|---|
| 退出 | View button |

# 实现说明

对于 HDR，Xbox 主机需要将值发送到具有 ST.2084 伽马曲线的 Rec.2020 颜色空间中的电视。 关于交换缓冲区格式、颜色空间格式和着色器输出，游戏在 Xbox One 主机和 Xbox Series 主机上处理此问题时有着不同的要求和选项。 系统还需要用于 GameDVR 和流式处理目的的 SDR 图像。 游戏可以将此 SDR 图像渲染到第二个交换链缓冲区，也可以让系统使用自动色调映射自动生成此图像。

**Xbox One**

在 Xbox One 上，交换链缓冲区必须使用 `DXGI_FORMAT_R10G10B10A2_UNORM` 格式，并且交换链颜色空间标志必须是 DXGI_COLOR_SPACE_RGB_FULL\_**G2084**\_NONE\_**P2020**。 解读此标志的含义很简单，它会告诉图形驱动程序，游戏的着色器将在应用 **ST.2084** 伽马曲线的 **Rec.2020** 颜色空间中将值写入交换链。

若要选择自动色调映射，游戏会将标记 `D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP` 添加到其交换链创建中。 请注意，在这种情况下，驱动程序将在内部分配一个额外的交换链，并注入一个计算着色器来对游戏的 HDR 图像进行色调映射以生成 SDR 图像。 游戏通过组合着色器来执行自己的色调映射可能会更高效。 在 PIX 中，可以在 Present 调用之前使用 PIX 命名事件&ldquo;\[HDR Auto Tonemap\]&rdquo;来识别驱动程序的自动色调映射着色器。

**Xbox 系列**

在 Xbox Series 主机上，交换链可以是 `DXGI_FORMAT_R9G9B9E5_SHAREDEXP`，也可以是 `DXGI_FORMAT_R10G10B10A2_UNORM`。建议使用 DXGI_FORMAT\_**R9G9B9E5**\_SHAREDEXP，因为它允许游戏显示更高的精度值。

交换链颜色空间标志可以是众多标志之一，它控制将在 GPU 上执行的操作以及将卸载到显示硬件的内容。 建议使用 DXGI_COLOR_SPACE_RGB_FULL\_**G10**\_NONE\_**D65P3**。 解密此标志的含义与 Xbox One 类似。 例如，前面的标志告诉驱动程序，其着色器将在 P3-D65 颜色空间中输出线性值或伽马 1.0。 这意味着显示硬件会将颜色值转换为 Rec.2020 并应用 ST.2084 伽玛曲线。 此建议的最大原因是大多数 HDR 电视可以显示或几乎显示 P3 颜色值。 如果游戏仅输出 Rec.709 颜色，则它利用电视的显示功能。 该示例演示了一个简单的色域扩展，使明亮的颜色更加丰富多彩，并且大部分图像仍然保持相同。

若要使用自动色调映射，游戏可以简单地呈现单个交换链，无需像 Xbox One 那样指定额外的标志。 也没有额外的 CPU、GPU、内存或带宽成本，所有这些都在显示硬件中处理。

能够将一些处理卸载到显示硬件是非常有益的，尤其是在 Lockhart 控制台上。

# 已知问题

无

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


