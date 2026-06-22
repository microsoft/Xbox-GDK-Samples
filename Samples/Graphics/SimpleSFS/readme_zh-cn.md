![](./media/image1.png)

# SimpleSFS 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

取样器反馈是一项 Direct3D 功能，用于在渲染过程中捕获和记录纹理采样信息和位置。 如果结合使用取样器反馈与平铺纹理，我们会在实现纹理流式处理的部分驻留纹理 (PRT) 时，获得一种管理磁贴驻留的简便机制。 此示例演示了流式处理取样器反馈 (SFS) 的简单实现，展示了如何配合使用 SamplerFeedback 映射与 MinMip 映射。 DirectStorage 用于直接加载和硬件解压磁贴到图形内存中。

该示例使用相机在四边形上呈现平铺纹理，该相机可以像非常简单的风景一样在四边形上移动。 当相机移动时，UI 将指示请求哪些磁贴驻留在内存中，以及当前哪些磁贴驻留在内存中。

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

下图显示了 SFS 实现中的数据流。 *MinMip map* 是保留有关当前驻留的最小 mip 级别（即具有最高详细信息的 mip）的信息的映射。 当流式处理系统加载或卸载磁贴时，它会更新 CPU 上的 MinMip 映射。 *反馈映射* 是保存关于在场景渲染期间请求驻留的最小 mip 级别的信息的映射。 在场景渲染期间，反馈映射在 GPU 上更新，而流式处理系统可以基于此信息生成新的流式处理请求。 有关更多详细信息，请参阅 GDK 文档和 Xfest 2019 演示文稿&ldquo;Scarlett 上的纹理流式处理&rdquo;。

![](./media/image4.png)

**平铺纹理**

此示例中使用的平铺纹理是使用 GDK 附带的 xbtc.exe 工具创建的。 该工具采用 DDS 文件作为输入，并生成压缩文件。 使用选项&ldquo;-tilemode SFS&rdquo;时，压缩文件将包含用于平铺纹理的 64KB 平铺。 TiledTexture.cpp 中的 `TiledTexture::Create()` 方法演示如何使用 .xbtc 文件。 加载此文件后，将创建一个磁贴列表，其中每个磁贴都保留有关数据应该加载到磁盘上的位置，以及加载后数据在内存堆中驻留的位置的信息。 PIX 可以可视化平铺纹理，从而显示提交的磁贴和保留的磁贴。

**反馈映射**

存在两种反馈映射格式：MinMip 和 RegionUsed。 此示例实现 MinMip 反馈，即使用 `DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE` 创建。若要为反馈映射创建 UAV，请使用 API `CreateSamplerFeedbackUnorderedAccessView()`。在场景呈现之前，必须清除反馈映射。 无法将其清除为值零，因为这意味着在场景呈现过程中已请求 mip 级别 0。 因此，该示例将映射清除为值 -1，这表示未请求 mip。 若要读取 CPU 上的值，首先需要使用 `ResolveSubresourceRegion()` 标志对纹理进行转码
反馈映射是 5.3 固定点。 该示例执行三个函数，用于在反馈值和 mip 值、`MipValueToFeedbackValue()`、`FeedbackValueToMipValue()` 和 `DecodeFeedbackMapValue()` 之间转换值。流式处理请求根据反馈值的小数部分进行排序。 PIX 可以可视化反馈映射。 **MinMip 映射**
| | |
|---|---|
`D3D12_RESOLVE_MODE_DECODE_SAMPLER_FEEDBACK`|。对于 Xbox Series X|S，值 |


MinMip 映射创建为采用 R8 格式的放置纹理。 若要创建 UAV，使用 API `CreateMinMipShaderResourceViewX()`。MinMip 映射中的值应编码为 5.3 固定点，即如果流式处理系统加载为 mip 2，则映射应包含值 (2 \<\< 3)。 Pix 可以可视化 MinMip 映射

**流式传输**

这是一个简单的示例，因此，流式处理是通过简单的流式处理逻辑同步实现的。 场景呈现后，反馈映射值用于在函数 GenerateStreamingRequests() 中生成流式处理请求。函数 ProcessStreamingRequests() 处理流式处理请求，使用 DirectStorage 将请求的磁贴直接加载并解压缩到图形内存中。

# 更新历史记录

2020 年 4 月 27 日 &ndash; 创建示例。

2021 年 10 月 15 日 - 添加了 1440p 支持。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


