  ![](./media/image1.png)

#   DrawIndexedX 示例

*本示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容*

# 

# 说明

本示例演示如何使用 Xbox 特有功能--
DrawIndexedX。此功能将多个连续绘制调用线程组合到单个波中并将它们作为单个绘图执行，而且可以用于代替
DrawIndexedInstanced
来支持单个实例。在多个实例的情况下，可以为每个实例单独调用DrawIndexedX，实例数据可以传递到一个缓冲区中，且通过索引缓冲区的某些位来标识正确的实例。只有在绘制调用之间没有状态变化时，才会进行打包。如果存在状态变化，则该绘制调用所进行的操作与
DrawIndexedInstanced 相同。对于每个 DrawIndexedX
调用，只能更改索引缓冲区和索引缓冲区格式。系统可以提供到顶点缓冲区的偏移量。顶点缓冲区需要合并到一个包含所有顶点数据的缓冲区中。索引缓冲区的一些位可用于传入数据，以帮助标识着色器中的绘制调用（绘图
ID）。在结构化缓冲区中，可以使用绘图 ID
针对某个绘制调用传入和提取更多数据。使用此参数并查看一些增益的最简单的地方是在深度传递期间，因为这期间通常不需要更改状态，因而可以组合线程。在颜色传递过程中，可能会导致增益，但具体取决于线程的组合所导致的发散量。**由于某些硬件设置，此功能在
Xbox One X 上所进行的操作与 DrawIndexedInstanced 相同。**

void DrawIndexedX(

> D3D12_GPU_VIRTUAL_ADDRESS *IndexBufferLocation*,
>
> DXGI_FORMAT *IndexFormat*,
>
> UINT *IndexCount*,
>
> UINT *IndexOffset*

);

为了支持 DrawIndexedX 而需要对现有着色器代码所做的更改：

-   现需从 SV_VERTEXID 提取绘图的顶点
    ID，因为可能已填充索引缓冲区数据以包括其他信息，如
    drawID、materialID 等。通过结构化缓冲区（可基于 drawID
    对其编制索引），可将更多数据传递到着色器中。

-   因绘图而异的描述符需要移动到数组中，如 SRV 的像素着色器中所示。

-   在有可能针对波内的各线程使用不同的值时，可能需要使用
    NonUniformResourceIndex
    关键字。如果不使用该关键字，则着色器将假设波内所有线程的值都是一致的。

示例中的简单场景发生的可见变化：

-   深度传递会在使用 DrawIndexedX 时略有减少。

-   为场景生成的 VS 波的数量会有所下降（从 2438 到 2183）。

-   在为 SRV 使用纹理数组时，像素着色器中只有大约 1-2% 的波出现发散。

# 构建示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

Project Scarlett 不支持此示例。该功能仅适用于 Xbox One 和 Xbox One S。

*有关详细信息，请参阅 GDK 文档中的*"运行示例*"。*

# 使用示例

## 屏幕截图

![Sample Screenshot](./media/image3.png)

对波进行调试：

![DrawIndexedX_Debug_DIX](./media/image4.png)

| 操作                                   |  游戏手柄                    |
|----------------------------------------|-----------------------------|
| 在 DrawIndexedX 和 DrawIndexedInstanced 之间进行切换 |  A |
| 调试屏幕 -- 查看发散波数               |  Y                           |
| 显示使用"图集"文理的模型               |  X                           |
| 显示深度缓冲区                         |  左肩按钮                    |

# 

# 实现说明

示例将执行下列传递：

-   仅深度传递

> 此传递从 DrawIndexedX
> 中受益最大，因为顶点着色器不需要分支，并且不存在像素着色器波。因此，可以将所有调用打包在一起，作为单个绘图执行。

-   仅调试 -- 总波数计数

用于对传递到相应帧的顶点和像素着色器的总波数进行计数的传递。

-   网格呈现器（颜色）传递（带 Depth EQUALS）

    -   此传递将网格绘制到最终缓冲区，并将纹理应用于该网格。绘制调用将使用不同的材料。此材料数据将与索引缓冲区信息一起传递。32
        位索引缓冲区中的多余闲置位可用于识别着色器正在执行的绘制调用。其他信息（比如材料
        ID）可以基于 drawID
        提取，并且可以传递到常量缓冲区中的像素着色器中。

> uint actualVertexID = vertexID & 0xFFFFF;
>
> uint materialIndex = (vertexID \>\> 20) & MAX_MATERIALS;

-   为每个模型传递不同的材料可能会导致为 DrawIndexedX
    生成的波产生发散。可以使用由多个纹理组成的图集来减少发散。该示例使用了由所有纹理组成的图集，并显示了在这种情况下如何完全消除发散。

若要查看减少的波数，请检查 PIX 中的以下计数器：

-   SPI_PERF_VS_WAVE

为相应绘制调用生成的 VS 波的数量。检查 DrawIndexedXPerformanceGroup
的数量。使用 DrawIndexedInstanced
时该值应小于生成的波数。此外，还可在着色器中对此进行计算，如示例中所示。

-   SPI_PERF_PS_CTL_WAVE

为相应绘制调用生成的 PS 波的数量。

-   VGT_PERF_VGT_PA_CLIPP_EOP / 2

表示执行的绘制调用的数量。如果发生状态更改，则绘图将被分割成单独的组，此数字表示创建的单独批处理（绘图）数。

-   你还可以查看 PIX，了解相应上下文如何用于绘制调用。

使用 DrawIndexedX 时：

> ![](./media/image5.png)

使用 DrawIndexedInstanced 时：

> ![](./media/image6.png)

# 已知问题

由于某些硬件设置，此功能在 XboxOneX 上所进行的操作与
DrawIndexedInstanced 相同。

# 更新历史记录

**初始发布：***2016 年 9 月 30 日*
