![](./media/image1.png)

# AdvancedPSO 示例

*此示例与 2022 年 3 月 GDK 兼容。*

# 说明

此示例演示了 Xbox 特定的管道状态对象 （PSO） 生成方法，这些方法相对于已有的 DirectX 12 方法 (CreateGraphicsPipelineState) 提高了内存使用率和运行时性能。 此示例配置为两个 Visual Studio 解决方案：在个人电脑上运行的 PSOGen 和在 Xbox 上运行的 AdvancedPSO。

PSOGen 解决方案演示了脱机 PSO 生成。 这在电脑上运行，并为给定的着色器组合生成一组序列化的 PSO 组件。 源代码演示了如何使用 PSO 序列化 API。 (SerializeGraphicsPipelineStateX)

AdvancedPSO 解决方案在主机上运行。 PSO 集在运行时按需加载并反序列化 (DeserializeGraphicsPipelineStateX)。 该集是所有组合所需的最少数据量，使用称为重复数据删除的过程。

AdvancedPSO 示例还演示了*派生* PSO。 这是一种性能和内存效率极高的方法，用于制作因 API 定义的某些属性而有所不同的新 PSO (CreateDerivedGraphicsPipelineState)。

![示例屏幕截图](./media/image3.png)

# 生成示例

AdvancedPSO 项目使用 PSOGen 生成的预编译 PSO 微代码。 着色器编译器在 GDK 版本之间可能发生更改，因此其输出也是如此。 因此，AdvancedPSO 项目要求预编译 PSO 与其生成环境之间的 GDK 版本匹配。

如果示例检测到版本控制失败，则将发出运行时错误。 如果针对其他 GDK 进行的编译只是生成并运行 PSOGen 工具来更新反序列化的 PSO，则重新运行 AdvancedPSO 示例。

生成并运行解决方案时，Visual Studio 将

- 生成 PSOGen 项目
- 在个人电脑上运行 PSOGen 项目以生成脱机 PSO
- 生成 AdvancedPSO 项目
- 在主机上运行 AdvancedPSO 可执行文件以使用脱机 PSO

# 使用示例

要使用该示例，只需运行 AdvancedPSO 示例。 它将在屏幕上显示每个 PSO 创建方法的指标。 要生成不同的着色器组合或编辑着色器源，请重新运行 PSOGen 可执行文件，然后运行 AdvancedPSO 示例。

如果要在电脑上调试 PSOGen 项目，需要将 Path 环境变量指向 GDK 二进制文件，如下所示：

![PSOGen 设置](./media/PSOGen-settings.png)

# 实现说明

PSOGen 项目必须与为电脑、d3d12_x[s].h、.lib 和 .dll 生成的 D3D12.x[s] UMD 驱动程序的自定义版本链接。 这可以分别在 `<GDK root\>\bin\XboxOne` 或 `<GDK root\>\bin\Scarlett` 的 GDK 安装目录中找到。 此库链接到该目录中的多个其他自定义生成库。 这些必须位于 **D3D12CreateDevice** 的路径上才能成功返回。 PSOGen 项目使用自定义生成步骤进行配置，用于将这些库克隆到项目的目录。 请注意，有不同的集
| | |
|---|---|
|Xbox One 和 Xbox Series X\|S 的库。|

| Xbox One | Scarlett (Xbox Series X\|S) |
|---|---|---|
| d3d12_x.dll | d3d12_xs.dll |
| xgs12_pc_x.dll | xgs12_pc_xs.dll |
| xg.dll | xg_xs.dll |
| dxcompiler_x.dll | dxcompiler_xs.dll |
| sc_dll.dll | xbsc_xs.dll |
| scdxil.dll | newbe_xs.dll |

预编译着色器 Blob 和序列化管道状态数据包为
将编译的 PSO 输出到项目配置设置的特定于平台的目录，然后由 AdvancedPSO 项目将其部署到相应的目标平台。 PSOSet 类用于生成和加载一组 PSO 组件。 此实现并非特别理想，因为它的主要目的是为了演示必要的步骤。 它将组件存储为单个文件，二实际上它们通常存档到 Blob 存储中。 此外，不会尝试预加载着色器组合，或使用多线程来序列化或取消序列化。
| | |
|---|---|
|Xbox One 和 Xbox Series X|S 之间不可互换。 PSOGen |


该示例使用 XMemAlloc 挂钩（在 Minitracker 类中）来度量各种创建方法的驱动程序内存使用情况。 创建第一个 PSO 时，驱动程序将预分配额外的内部存储。 该示例使用 ObjectId 属性从统计信息中排除这些分配。 ObjectId 属性的语义不会公开给公共标头，并且可能会发生更改。 它们可能会在将来的 GDK 版本中导出。

# 更新历史记录

2020/12/14 -- 从 Xbox One XDK 移植到 GDK。 2023/11/28 -- 解决方案重新配置为自动生成 PSO


