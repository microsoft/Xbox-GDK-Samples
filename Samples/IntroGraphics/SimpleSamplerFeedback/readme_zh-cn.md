![](./media/image1.png)

# 简单取样器反馈示例

*此示例兼容于 Microsoft 游戏开发工具包（2020 年 6 月）*

# 说明

取样器反馈是一项 Direct3D 功能，用于捕获和记录纹理采样信息和位置。 它可以用于纹理空间着色和纹理流式处理等内容。 此示例仅演示了一种非常简单的采样器反馈实现。

该示例使用可向四边形移动或远离四边形的相机呈现纹理四边形。 当相机靠近四边形时，在渲染过程中会使用更详细的 mip，即较低的 mip 级别。 采样器反馈将此信息写入 MinMip 反馈映射。

注意：Xbox One 不支持采样器反馈，因此这是
| | |
|---|---|
|仅 Xbox Series X|S 示例。|

![](./media/image3.png)

# 生成示例

平台将为 Gaming.Xbox.Scarlett.x64 *有关详细信息，请参阅* *GDK 文档*中的__运行示例__。&nbsp;
| | |
|---|---|
|此示例仅支持 Xbox Series X|S，因此活动解决方案 |


# 使用示例

| 操作 | 游戏板 |
|---|---|
| 移动相机 | 左拇指摇杆 |
| 退出 | &ldquo;视图&rdquo;按钮 |

# 实现说明

**创建**

存在两种反馈映射格式：MinMip 和 RegionUsed。 此示例实现 MinMip 反馈，即使用 DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE 创建的反馈。

取样器反馈通常与平铺资源一起使用。 因此，反馈映射的大小通常为其配对平铺纹理尺寸的一部分，即每 64KB 磁贴一个纹素。 在这个非常简单的示例中，我们创建了一个 1x1 反馈映射，即整个纹理的一个反馈映射值。

若要将反馈映射绑定到着色器，并将常规纹理与反馈映射配对，请使用 API CreateSamplerFeedbackUnorderedAccessView。

**场景渲染**

在场景呈现之前，必须清除反馈映射。 无法将其清除为值零，因为这意味着在场景渲染过程中已请求 mip 级别 0。 因此，该示例将映射清除为值 -1，这表示未请求 mip。

着色器模型 6.5 支持采样器反馈着色器说明。 此示例的像素着色器使用 WriteSamplerFeedback 方法。 文件 pixelshader.hlsl 还包含用于模拟取样器反馈的着色器代码，这在不支持采样器反馈的平台上可能很有用。

**回读**

若要读取 CPU 上的值，必须使用 ResolveSubresourceRegion 对反馈映射进行转码。 此示例创建回读纹理
反馈映射是 5.3 固定点。
| | |
|---|---|
用于回读的 |。 在 Xbox Series X|S 上， | 中的值

# 更新历史记录

2019/12/05 -- 示例创建。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


