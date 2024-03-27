![](./media/image1.png)

# 简单照明示例

*此示例可用于 Microsoft 游戏开发工具包（2020 年 6 月）*

# 说明

此示例演示如何创建静态 Direct3D 12 顶点、索引和常量缓冲区，以使用静态和动态 Lamber constant 照明绘制点亮的索引几何图形。

此示例呈现一个较大的立方体，该立方体由两个灯照亮，一个白色和一个红色，也表示为立方体。 白色光是静止的，而红色的光位于中央立方体周围。 中央立方体也会旋转。 通过运动，您可以从不同角度观察彩色光的效果。

![](./media/image3.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox One X|S 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅*__运行示例__，详见*GDK 文档。*

# 使用示例

| 操作 | 游戏板 |
|---|---|
| 退出 | &ldquo;视图&rdquo;按钮 |

# 实现说明

## &ldquo;着色器&rdquo;

## 该示例使用三个着色器呈现场景 - 一个顶点着色器（&ldquo;TriangleVS&rdquo;）和两个像素着色器（&ldquo;LambertPS&rdquo;、&ldquo;SolidColorPS&rdquo;）。 编译的着色器 Blob 加载到 CreateDeviceDependentResources 中，然后在为每个着色器组合创建管道状态对象时引用。 所有着色器都在同一 HLSL 包含文件&ldquo;SimpleLighting.hlsli&rdquo;中定义，三个存根着色器包括此文件。 为创建三个着色器 Blob，将针对不同的入口点编译每个存根着色器。

## 管道状态对象 (PSO)

简单照明示例具有两种独特的着色器组合：第一种是与 LambertPS 结合使用的三角形VS，第二种是具有 SolidColorPS 的 TriangleVS。 在 DirectX 12 中，需要为每个唯一着色器组合创建管道状态对象 (PSO)。 顾名思义，PSO 会使用一组特定的着色器封装可能多个绘图调用所需的所有管道状态。 PSO 组合了根签名、管道各个阶段的着色器、光栅器状态、深度模具状态、混合状态等状态设置。 （有关更多详细信息，请参阅 MSDN 上的文档。）

## 根签名

根签名定义绑定到图形管道的资源类型以及资源的布局方式。 根签名类似于 API 函数签名，它描述参数类型、参数顺序和布局，但不定义任何实际的参数实例。 根参数是与根签名的元素相对应的实际数据实例。 示例的顶点着色器只需要着色器常量的单个结构，因此根签名非常简单。 根签名包含一个 ConstantBufferView 类型的根参数。

## 几何

## 场景的几何图形由静态顶点和索引数组组成，这些数组包含表示多维数据集六个象限的 24 个顶点的数据。 这两个数组在 Sample::CreateDeviceDependentResources 中声明，ID3D12Device::CreateCommittedResource 会在其中立即使用它们为缓冲区创建 ID3D12Resources。 为简单起见，该示例使用 D3D12_HEAP_TYPE_UPLOAD，因为这样就可以在单个步骤中使用数据初始化每个资源。 但是,\_UPLOAD 堆是几何图形数据的次优位置。 更有效的实现将使用几何图形数据的 D3D12_HEAP_TYPE_DEFAULT。 若要初始化 \_DEFAULT 堆，需要使用 \_UPLOAD 堆，以便最终使用两个堆，这会使实现变得复杂。

创建几何图形的缓冲区后，该示例可以为顶点创建D3D12_VERTEX_BUFFER_VIEW，并为索引创建D3D12_INDEX_BUFFER_VIEW。 通过调用 ID3D12GraphicsCommandList::IASetVertextBuffer 和 ID3D12GraphicsCommandList::IASetVertextBuffer 设置输入汇编程序时，在 Sample::Render 中使用这些视图。

## 管理着色器常量

对于这一非常简单的场景，所有着色器常量将整合到包含以下内容的单个常量缓冲区中：

- 世界、视图和投影矩阵

- 光线方向和颜色

- 纯色

对于更复杂的场景，通常会将常量拆分为多个缓冲区，具体取决于更新常量的频率。

由于大型多维数据集和红色指示灯经过动画处理，因此在绘制调用之间每个帧需要更新着色器常量多次。 回想一下，根签名包含一个 ConstantBufferView 类型的根参数，该参数需要引用着色器常量的副本，以便在每次绘图调用中使用。 由于 CPU 和 GPU 并行运行，因此在使用 GPU 完成之前，CPU 不应尝试更新任何常量缓冲区。 如果只有一个常量缓冲区，则在 GPU 完成绘制之前，必须阻止 CPU，这不切实际，因为需要为多个绘图调用更新常量。 因此，该示例使用多个常量缓冲区，以便 CPU 可以在 GPU 绘制时继续将常量发送到 GPU。

该示例很简单，每个帧都有固定数量的绘图调用，这在编译时是已知的。 该示例为每个绘图调用创建一个缓冲区，并乘以交换链中的后退缓冲区数。 此数字可保证始终有一个可供 CPU 写入的可用缓冲区。 所有常量缓冲区都存储在 Sample::CreateDeviceDependentResources 中创建的单个连续上传缓冲区中。 将立即映射上传缓冲区，以获取 CPU 地址空间和 GPU 地址空间的基本内存地址。

在 Sample::Render 方法中，常量被写入到从常量上传堆的 CPU 基址索引的位置。 相同的索引与 GPU 地址结合使用，通过调用 ID3D12GraphicsCommandList::SetGraphicsRootConstantBufferView 将缓冲区绑定到管道。 索引必须考虑后台缓冲区节奏和绘图计数。 有关更多详细信息，请参阅 Sample::Render 实现。

## CPU/GPU 同步

如果 CPU 能够以比 GPU 处理它们的速度更快地向 GPU 发出命令列表，则最终 CPU 必须等待 GPU 跟上。 此示例以轮循机制方式使用常量缓冲区内存，这意味着将在固定时间段后重复使用缓冲区槽。 通常，在重复使用共享资源之前，请务必使用某种同步策略来确保资源未在使用中。 该示例使用 ID3D12Fence 同步 CPU 和 GPU。 CPU 将命令插入命令队列，以使用帧索引值向围栏对象&ldquo;发出信号&rdquo;。 在 GPU 上处理信号命令后，提供的帧索引值就会对 CPU 可见。 这使 CPU 能够将当前帧索引与 GPU 发出信号的最后一帧索引进行比较，以确定 GPU 与 CPU 的比较距离。 如果 GPU 帧计数和 CPU 帧计数之间的差异超过后台缓冲区数，则 CPU 将需要等待。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


