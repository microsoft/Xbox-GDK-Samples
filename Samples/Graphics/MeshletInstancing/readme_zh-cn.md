![](./media/image1.png)

# Meshlet 实例化示例

*此示例兼容 Microsoft 游戏开发工具包（2022 年 3 月）和 Windows 10（版本 2004）2020 年 5 月更新*

# 说明

Mesh 着色器管道发生在输入汇编程序之前，它负责执行索引处理和实例化功能。 因此，Mesh 着色器管道不会像在传统图形管道中那样公开用于实例化的高级接口，而是由开发人员负责使用 Mesh 着色器线程 ID 来实现自己的实例化解决方案。

GPU 在固定大小的、称为*波形*的线程块中调度工作负载。 此大小是特定于体系结构的，但是可调度的最小线程量。 输入汇编程序执行的一项功能是将这些波形与工作打包，以优化其线程利用率*。 波次利用率*是指处理实际数据的波形线程与调度的所有波形的比值之比。

由于 Mesh 着色器管道省略了输入汇编程序的使用，因此开发人员会担心使用工作打包波次。 Meshlet 是一种工具，用于将 Mesh 预处理为易记大小的工作区块，从而优化波次利用率。 但是，对 meshlet 没有完全满的硬约束，从而导致波次利用率欠佳。 具体而言，Mesh 的最后一个 meshlet 很可能不完整（因为 Mesh 在填充基元之前已用尽。） 随着 Mesh 方法中的 meshlet 数目变为零，这就带来了一个不断增长的问题。 这些小 Mesh 的粗实例化很常见，例如树叶、毛发、粒子等。

此示例通过将最后一个未归档的 meshlet 的多个实例打包到单个线程组中，提供了一种有效实例化的通用方法。 这样可以最大程度地减少调度的死线程数，使我们的波次利用率接近 100%。

![](./media/image3.png)

# 生成示例

如果使用 Xbox Series X|S 开发工具包，请将可用解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

如果使用具有适当硬件和 Windows 10 或 Windows 11 版本的电脑，请将活动解决方案平台设置为 Gaming.Deskop.x64。

此示例不支持 Xbox One。

*有关更多信息，请参阅*&nbsp;__运行示例__（位于 *GDK&nbsp;文档）中。*

# 使用示例

该示例提供了在两个实例化布局之间进行选择的选项：同心圆圈和多维数据集。 这允许以不同的速率进行缩放。 还可以在平面底纹或可视化实例的基础 meshlet 结构之间切换。

# 控件

| 操作 | 游戏板 | 键盘 |
|---|---|---|
| 移动相机 | 左控制杆 | WASD 或箭头键 |
| 旋转相机 | 右控制杆 | 按住 LMB + 鼠标 |
| 重置摄像头 | 右控制杆（推） | \- |
| 更改实例化模式 | A | Tab |
| 切换 Meshlet 可视化效果 | X | 空格键
 |
| 提高实例化级别 | 右侧肩部 | \+ |
| 降低实例化级别 | 右扳机键 | \- |
| 退出 | &ldquo;视图&rdquo;按钮 | 退出 |

# 实现说明

Mesh 着色器实例化是调度足够多的着色器实例以完成工作并使用提供的 ID 确定要处理的正确 meshlet 和实例的简单问题。 所选索引方案确定哪个线程组处理哪种工作位。

假设每个线程组有一个 meshlet，最直接的实现是调度 MeshletCount \* InstanceCount 线程组。 我们可以确定这是足够的线程组来完全处理我们的工作负荷。 这也提供了一个非常简单的索引方案：

- *MeshletIndex* = *GroupID.x* / *InstanceCount*

- *InstanceIndex* = *GroupID.x* % *InstanceCount*

这意味着第一个 *InstanceCount* 线程组将全部处理 Meshlet *0*，但会处理不同的实例。 更重要的是，最后一个 *InstanceCount* 线程组将全部处理 Meshlet *(MeshletCount -- 1)*。 我们假定第一个 *(MeshletCount -- 1)* meshlet 为&ldquo;full&rdquo;（具有接近最大顶点和/或基元），而最后一个 meshlet 的填充较少。 因此，最后一个 *(MeshletCount -- 1)* 线程组的波次利用率低于第一个 *(MeshletCount -- 1) \* InstanceCount* 线程组。 如果可以将足够稀疏的多个实例打包到单个线程组中。

实际上，假设线程组大小等于最大 meshlet 大小，我们可以将 $\lfloor MaxMeshletSize\ /\ LastMeshletSize\rfloor$ 实例容纳到单个线程组中。 因此，我们需要 $\lfloor（LastMeshletSize*InstanceCount）/MaxMeshletSize\rfloor$ 个打包的线程组来处理最后一个 meshlet。 这将以着色器中这些线程组的一些额外 ALU 为代价，提供最佳波形效率。

# 更新历史记录

2019/10/31 -- 示例创建。

2020/4/28 - 已更新为使用 D3DX12 帮助程序创建网格着色器管道

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


