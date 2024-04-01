![](./media/image1.png)

# FastBlockCompress 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

此示例演示了如何使用 DirectCompute 在运行时对 BC1、BC3 和 BC5 格式基于经典的*快速块压缩*算法执行快速纹理压缩。 此示例还允许在运行时和脱机压缩模式之间进行切换，以比较可视质量。

![](./media/image2.jpeg)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* __运行示例__，*在 GDK 文档中。*

# 使用示例

| 操作 | 游戏板 |
|---|---|
| 上一张或下一张图像 | 左缓冲键或右缓冲键 |
| 上一个或下一个压缩方法 | 方向键向左或向右 |
| 上一个或下一个 mip 级别 | 方向键向下或向上 |
| 移动相机 | 右摇杆 |
| 放大或缩小 | 左扳机键或右扳机键 |
| 全屏与并排 | X |
| 突出显示块 | A |
| 循环差异模式 | Y |
| 退出 | &ldquo;视图&rdquo;按钮 |

# 背景

Xbox One 有 5 GB 的统一内存可用于专属应用，比 Xbox 360 上的 512 MB 内存大幅增加了 10 倍。 遗憾的是，IO 带宽和存储介质容量未完全跟上步伐。 蓝光介质将保留 49 GB，这仅是 Xbox 360 游戏光盘版本 3 提供的 7.8GB 的 6.3 倍。

这一事实，再加上媒体流式处理安装的引入，意味着高效的压缩方法对于最大限度地缩短加载时间和将游戏资产打包到可用存储空间中仍然非常重要。

游戏通常可以通过使用脱机图像压缩格式对游戏纹理进行编码来节省大量存储空间。 Xbox One 内置了硬件 JPEG 解码器，这使得 JPEG 成为极具吸引力的选择。 但是，JPEG 硬件将纹理解码为内存中未压缩的 YUV 格式，这不适合呈现。 如果使用此方法，游戏将需要在运行时将纹理重新压缩为 GPU 支持的块压缩格式之一。

此示例使用 GPU 将纹理高效压缩为 BC1、BC3 和 BC5 格式。 用于脱机块压缩的标准算法历来太慢，无法实时运行，并且该示例使用的算法出于速度利益而显著降低质量。

由于内存带宽瓶颈会导致当前算法出现瓶颈，因此使用其他技术，你或许能够通过较小的性能损失来实现显著的质量改进。

# 实现说明

示例中的每个 DirectCompute 压缩着色器有三种变体：一个 mip 版本、两个 mip 版本和尾部 mip 版本：

- 一个 mip 着色器压缩源纹理的单个 mip。

- 两个 mip 着色器读取源纹理的单个 mip，并向下采样 mip 到本地数据存储 （LDS） 内存。 接下来，着色器压缩原始和向下采样版本，然后着色器在输出纹理中写入相应的 mip 级别。

此过程通过避免从源纹理读取第二个 mip 级别来节省内存带宽。 但在实践中，由于 GPR 和 LDS 使用率较高，增大了着色器复杂性和占用率下降，从而大大降低了性能提升。

- 尾部 mip 着色器通过选择不同的线程来处理不同的 mip 级别，在单个调度调用中压缩源纹理的 16×16 到 1×1 mip 级别。

由于最小波面大小为 64 个线程，因此在单独的调度调用中压缩每个尾部 mip 的技术会浪费大部分可用线程。 通过仅使用一个波面和调度调用，尾部 mip 着色器可避免浪费大量工作。

Direct3D 不允许将 BC 格式纹理绑定为 UAV，因此不能直接从计算着色器写入块压缩纹理。 此示例通过将可写格式的中间纹理别名化为与块压缩纹理相同的内存位置来解决此限制。 中间纹理是大小的四分之一，每个纹素对应于压缩纹理中的块。

以这种方式对纹理内存进行别名要求两个纹理的平铺模式和内存布局完全匹配。 此外，Direct3D 不依赖于内存别名，因此 GPU 可以同时计划对别名到同一内存位置的不同资源执行的多个绘制或调度调用。

换句话说，写入中间纹理的着色器可以与从别名块压缩纹理中读取的绘图调用同时进行计划。 若要防止这些危险，应手动插入适当的围栏。

脱机行压缩算法在 [DirectXTex](https://github.com/Microsoft/DirectXTex/) 中实现。

# 备选方案

此示例的主要用途是提供一个测试用例，以将经典&ldquo;JPG/FBC&rdquo;解决方案与其他替代方法进行比较，以便在运行时最大程度地减少磁盘上的纹理存储和内存中消耗。

- 基础通用（[GitHub](https://github.com/BinomialLLC/basis_universal/)）- 此解决方案将磁盘上的纹理压缩为 [ETC1](https://github.com/Ericsson/ETCPACK) 的变体，该变体在运行时可以转码为多种不同的格式，包括 BC7（模式 6）。 与经典 JPG/FBC 管道相比，这可以实现更小的磁盘占用空间，并支持更广泛的目标 GPU 数组，其图像质量与经典 JPG/FBC 管道类似或更好。 虽然 .basis 的多 GPU 转码功能对移动设备的作用远大于主机游戏，但为了节省磁盘空间，这种格式还是值得评估。

- XBTC - 在 Xbox Series X|S 上，可以结合使用 DirectStorage 和 XBTC 压缩方案。 请参阅 **TextureCompression** 示例。

# 参考

Microsoft 高级技术组。 快速块压缩示例。 Xbox 360 SDK。 2010 年 2 月.

Narkowiez，Krzysztof。 &ldquo;GPU 上的实时 BC6H 压缩&rdquo;。 由 Wolfgang Engel 编辑的 *GPU Pro 7*（CRC 出版社）。 2016. （第 219-228 页）。

Tranchida，Jason。 [使用 GPU 实时压缩纹理](http://www.gdcvault.com/play/1012554/Texture-compression-in-real-time)。 GDC 2010。 2010 年 3 月。

van Waveren，J.M.P[. 实时 DXT 压缩](https://software.intel.com/sites/default/files/23/1d/324337_324337.pdf). Intel 软件网络。 2006年 5 月。

van Waveren、J.M.P.和 Castaño、Ignacio。 [实时 YCoCg-DXT 压缩](https://www.nvidia.com/object/real-time-ycocg-dxt-compression.html)。 NVIDIA 开发人员网站。 2007 年 9 月。

van Waveren、J.M.P.和 Castaño、Ignacio。 [实时普通映射 DXT 压缩](http://developer.download.nvidia.com/whitepapers/2008/real-time-normal-map-dxt-compression.pdf)。 NVIDIA 开发人员网站。 2008 年 2 月。

# 更新历史记录

2019 年 9 月发布

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


