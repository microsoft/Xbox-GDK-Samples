  ![](./media/image1.png)

#   OfflineRT 示例

*此示例兼容于 Microsoft 游戏开发工具包(2021 年 6 月)。*

# 

# 说明

此示例演示如何使用 Scarlett 平台 Microsoft
游戏开发工具包中提供的脱机加速结构 （BVH） 生成器和光线跟踪管道状态对象
（RTPSO） 序列化功能。

游戏可以将这些功能集成到现有内容和着色器生成管道中，以 Xbox
本机格式生成资产数据，以防止在运行时调用成本高昂的着色器编译器，同时提高
BVH 质量并减少暂存内存需求。

# 生成示例

示例由两个单独的解决方案组成：RTBuilder 和 OfflineRT。RTBuilder
是一个电脑应用程序，包含生成脱机 BVHs 和 RTPSO 所需的代码，OfflineRT
是一个 Scarlett 应用程序，可以使用脱机生成的数据并将其可视化。

**由于这种生成者与使用者关系，尝试在 Scarlett 上编译和运行 OfflineRT
示例前，必须先编译并运行 RTBuilder 项目。** 否则将导致编译和部署错误。

运行 RTBuilder 时，将看到如下所示的主机输出：

![Text Description automatically generated](./media/image3.png)

工具完成后，应在示例的 Build 目录看到以下生成的文件：

![Graphical user interface, text, application Description automatically generated](./media/image4.png)

现在可以在 Scarlett 开发工具包上继续编译和运行 OfflineRT 示例。

# 使用示例

![A picture containing text Description automatically generated](./media/image5.png)

示例使用简单的 RTPSO
对单个模型进行光线跟踪。可以使用游戏板在红色运行时（.sdkmesh
文件）和绿色脱机（.mdat 文件）生成的模型 BVH
之间切换，并通过运行时、基于脱机集合和完全脱机 RTPSO 的运行时进行循环：

| 操作                                         |  游戏板                |
|----------------------------------------------|-----------------------|
| 切换模式 （BVH）                             |  方向键向左/向右       |
| 切换 RTPSO                                   |  方向键向上/向下       |
| 旋转相机                                     |  右控制杆              |
| 重置相机                                     |  右控制杆按钮          |
| 缩放/滚动相机                                |  左控制杆              |
| 退出                                         |  视图按钮              |

此示例显示所选 BVH 和 RTPSO 的一些基本统计信息。请注意，脱机生成的 BVH
在内存中明显较小，并且在运行时也不需要'任何暂存空间。重要的是，它还会更快地进行跟踪。对于脱机
RTPSO，可以看到几乎可以将完整的创建时间成本转移到生成器。

# 实施说明

RTBuilder 应用程序利用 PC 版 Scarlett 图形驱动程序 （UMD）
中存在的功能，将 OBJ 模型和 HLSL 着色器库转换为 Xbox 本机 BVH 和
RTPSO。驱动的程序电脑 API 在
%GXDKLatest%\\toolKit\\include\\Scarlett\\d3d12_xs.h 中可用，并能够在
%GXDKLatest%\\bin\\Scarlett
中找到运行时所需的二进制文件。此示例使用自定义生成步骤将二进制文件部署到输出目录。

虽然脱机 BVH 是使用与生成运行时 BVH 相同的
ID3D12GraphicsCommandList6::BuildRaytracingAccelerationStructure API
生成的，但请务必注意，这些操作在 CPU 上（调用时立即执行）而不是在
ExecuteCommandLists 时间的 GPU
上发生。在同一行中，作为输入和输出传递的所有
D3D12_GPU_VIRTUAL_ADDRESS参数都解译为常规 CPU 虚拟内存地址。在内部，电脑
UMD 将使用 [Intel Embree](https://www.embree.org/)
生成器，而不是在运行时在驱动程序中使用的基于 GPU 计算的解决方案来生成
BVH。Embree 生成器生成更高质量的 BVH，这通常会导致遍历时间加速 5% -
10%（可能更高，具体取决于模型数据）。

尽管运行时和脱机生成的 BVHs
之间的性能增量在此示例的内容上很小，但典型的游戏内容将显示更大的性能优势。Intel
Embree
脱机生成器采用"三角形拆分"技术来减少长细三角形的有效外围应用，但运行时生成器不使用。脱机生成器还会生成较小的（内存）BVH
结构，因为可以生成比运行时生成器更高的四色叶。最后，运行时生成不需要暂存内存，这对于
Xbox Series S 主机特别有用。

构造 BVH 后，使用 CopyRaytracingAccelerationStructure API 及复制模式
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE
序列化结构并写出至磁盘。

在 OfflineRT 示例中（参见 AddModelFromMDat 函数），可使用
CopyRaytracingAccelerationStructure API 及复制模式
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE 在 GPU
上将加速结构重新反序列化。建议在异步计算管道上与其他图形工作并行执行反序列化操作。

脱机 RTPSO 在 RTBuilder 中分三个步骤生成：

-   使用 Scarlett 着色器编译器 （DXC） 将 HLSL 编译为 DXIL
    库（lib_6\_6目标）

-   使用 Scarlett PC UMD 创建 RTPSO

-   使用 Scarlett PC UMD 序列化 RTPSO

初始编译由 Visual Studio 中的 *HLSL
编译器*自定义工具处理。如果右键单击项目 *Assets* 文件夹中的任何 .hlsl
文件，可以看到用于此过程的参数。中间 DXIL 库保存在示例 Build\\Int
目录中。第二步和第三步在运行 RTBuilder
时完成。此示例支持集合和完全链接的 RTPSO
创建和序列化。在这两种情况中，如果 ShaderConfig、PipelineConfig
和根签名已知，调用 CreateStateObject 时，PC UMD 将完全编译 Xbox
本机着色器（参阅 DXR 规范中的 [Collection
状态对象](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#collection-state-object)
部分）。同样需要注意的是，序列化过程（通过 SerializeStateObjectX
调用启动）将从状态对象中剥离所有 DXIL
和其他元数据，从而使得任何运行时子对象关联都无法实现。但是，仍然可以查找着色器绑定表的着色器标识符，因为着色器不是子对象。

OfflineRT 示例演示了构造 RTPSO 的 3 种不同方法：

-   从 DXIL 库创建运行时（请参阅 AddPipelineFromEmbeddedDXILLib
    函数）：此方法具有最大的灵活性，因为它允许完整的子对象关联行为，但在运行时会产生完整的编译和链接成本。

-   基于脱机集合创建运行时（请参阅
    AddPipelineFromSerializedCollections）：状态对象创建速度很快，因为所有着色器都已完全预编译并在内部链接。此方法通过允许
    RTPSO 从多个（可能共享的）集合中"组合"来保持一定程度的灵活性。

-   运行时反序列化（请参阅 AddPipelineFromSerializedRTPSO
    函数）：状态对象创建速度很快，因为所有内容都预先编译和关联。没有真正的灵活性。

# 已知问题

相关 Scarlett 显卡驱动程序 bug：

-   Bug 33668549:显卡：XDXR 无法通过
    D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION 正确添加从 RTPSO
    引用的集合

# 更新历史记录

-   2021 年 6 月：初始版本。

# 隐私声明

在编译和运行示例时，将向 Microsoft
发送示例可执行文件的文件名以帮助跟踪示例使用情况。若要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。
