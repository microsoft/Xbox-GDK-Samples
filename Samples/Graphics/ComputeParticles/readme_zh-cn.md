  ![](./media/image1.png)

#   计算粒子

*此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容*

# 说明

此示例演示如何使用计算着色器和附加缓冲区来执行基本粒子模拟并有效地渲染数量众多的粒子。

![](./media/image3.png)

# 构建示例

如果使用 Xbox One devkit，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

*有关详细信息，请参阅 GDK 文档中的"*运行示例*"。*

# 使用示例

此示例使用以下控制。

| 操作                            |  游戏手柄                           |
|---------------------------------|------------------------------------|
| 退出示例。                      |  选择                               |
| 增加/减少粒子反弹               |  右/左扳机键                        |
| 旋转相机                        |  左/右摇杆                          |
| 移动粒子发射器                  |  左和右摇杆 + 右肩键                |
| 切换粒子渲染                    |  A 按钮                             |
| 切换粒子更新                    |  B 按钮                             |
| 增加/减少粒子数量               |  D-Pad 向上/向下                    |

# 实现说明

此示例演示 D3D11
和计算着色器可用的一些更深奥和有趣的技术。此示例包含三个相关部分。前两个部分与如何更新和剔除粒子有关，第三个部分是常规渲染管道如何消耗计算着色器阶段的结果。

1.  **粒子模拟**

> AdvanceParticleCS 计算着色器模拟阶段包含两个主要步骤。首先，从 UAV
> 缓冲区读取粒子位置、速度和年龄，并在世界坐标空间中进行模拟。然后使用一种强力方法计算与简化世界几何体（地平面和球体）的碰撞。新的位置、速度和年龄将被写回相同的
> UAV 缓冲区，覆盖以前读取的数据。

2.  **粒子剔除和写入**

对每个粒子应用一个简单的平面剔除算法，以考虑其在视体中是否可见。当某个粒子可见时，其位置将附加到附加缓冲区以进行渲染。

通过使用 ID3D12Device::CreateUnorderedAccessView(...) API
为缓冲区资源创建无序访问视图
(UAV)，并将第二个资源指定为"pCounterResource"参数，可以使用附加缓冲区。计数器资源必须至少为
4 个字节，才能存储附加缓冲区的当前计数（一个 32 位无符号整数）。
还为计数器资源创建了一个 UAV，以便我们可以使用
ID3D12GraphicsCommandList::ClearUnorderedAccessViewUint(...) API
清除每个帧的计数。创建完成后，我们只需将缓冲区绑定到声明为
AppendStructuredBuffer\<...\> 的UAV 着色器槽即可。

AdvanceParticlesCS
计算着色器可模拟活动粒子实例并将其添加到附加缓冲区。完成后，将使用
ID3D12GraphicsCommandList::CopyBufferRegion(...) API
将粒子计数从计数器资源复制到间接参数缓冲区资源，该资源可用作
ID3D12GraphicsCommandList::ExecuteIndirect(...) API
的输入。这样一来，我们就只能绘制由 AdvanceParticlesCS
的视体剔除测试决定的可见粒子。

3.  **渲染**

ID3D12GraphicsCommandList::ExecuteIndirect(...) API 用于调度粒子渲染。
需要使用 ID3D12Device::CreateCommandSignature(...) 创建的命令签名来指定
ExecuteIndirect
将分派的命令类型。该命令类型确定如何解释间接参数缓冲区的内容。在我们的示例中，命令类型为"Draw"，与
ID3D12GraphicsCommandList::DrawInstanced(...) API 相关联 - 四个 32
位无符号整数。我们将 VertexCountPerInstance 硬编码为
4，然后将粒子计数复制到每个帧的 InstanceCount 位置。

顶点属性在顶点着色器中被硬编码为常量查找表。顶点 ID (SV_VertexID)
用于在此查找表中建立索引，以访问每个顶点的属性。实例 ID (SV_InstanceID)
用于在粒子实例缓冲区中建立索引，以访问每个实例的属性。

# 更新历史记录

2019 年 3 月 -- 从旧 Xbox 示例框架移植到新模板。

# 隐私声明

在编译和运行示例时，示例可执行文件的文件名将发送给
Microsoft，以帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。
