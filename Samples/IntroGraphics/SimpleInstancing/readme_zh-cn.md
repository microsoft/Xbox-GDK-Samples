![](./media/image1.png)

# 简单实例化示例

*此示例可用于 Microsoft 游戏开发工具包（2020 年 6 月）*

# 说明

此示例演示了如何通过 Direct3D 12 API 使用实例化。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox One X|S 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅*__运行示例__，详见*GDK 文档。*

# 使用示例

![](./media/image3.png)

| 操作 | 游戏板 |
|---|---|
| 旋转相机 | 左控制杆 |
| 更改实例计数 | LB / RB |
| 重置模拟 | A |
| 退出 | &ldquo;视图&rdquo;按钮 |

# 实现说明

呈现实例几何图形时，需要考虑四个方面：

1. **几何数据**

> 在此示例中，这包括顶点和索引缓冲区
> 包含描述多维数据集人脸的顶点和索引。 它
> 还包括呈现此多维数据集所需的*管道状态对象*
> 到世界。 所有这些组件都
> 以与非实例化几何图形相同的方式进行设置和操作。 （请参阅
> SimpleInstancing.cpp 中的 *CreateDeviceDependentResources*）

2. **实例数据**

> 对于标准 D3D12 实例化呈现，
> 通过一个或多个顶点缓冲区提供每个实例数据。 这些顶点缓冲区
> 就像在任何其他顶点缓冲区一样中创建。 此示例使用两个
> 顶点缓冲区。 一个是静态的，包含每个实例的颜色数据
> （这在示例的生存期内是不变的）。 另一个是
> 动态并包含每个实例的位置和方向信息
> （更改每个帧）。

3. **实例化布局**

> 若要呈现几何图形实例化，
> D3D 需要关于如何解释所提供的顶点数据的信息。 这是使用 
> *D3D12_INPUT_ELEMENT_DESC* 结构数组完成的，
> 与标准呈现的方式非常相似。 但是，额外的元素会添加到此
> 结构。 几何数据标记为
> *D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA* 值
> 像往常一样，*InputSlotClass* 元素，但每个实例数据使用
> *D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA* 值。 *InputSlot*
> 元素还用于表示每个部分的顶点流
> 数据是从中拉取。
>
> 顶点着色器使用一个顶点结构，该结构的定义方式与
> 几何数据和每实例数据都汇集在一起（即
> 反映了 *D3D12_INPUT_ELEMENT_DESC* 中描述的布局
> 数组）。
>
> ***注意：** 此示例使用* D3D12_APPEND_ALIGNED_ELEMENT
> *AlignedByteOffset* 的常量 *元素以自动对齐
> 输入布局中的数据正确。 仅当结构正常运行时，
> 有问题的顶点缓冲区的包含正确对齐的数据。 如果
> 你正在跳过（或忽略）顶点数据中的元素，然后
> 需要精确对齐偏移量。*

4. **渲染**

> 在前面的点被固定后，呈现实例化数据就
> 很简单。 *ID3D12GraphicsCommandList::IASetVertexBuffers* API 用于
> 设置用作输入的顶点缓冲区（在本例中为输入
> 缓冲区） 和 *ID3D12GraphicsCommandList::D rawIndexedInstanced*
> API 用于呈现。 执行其余的呈现设置
> 与标准非实例化呈现相同。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


