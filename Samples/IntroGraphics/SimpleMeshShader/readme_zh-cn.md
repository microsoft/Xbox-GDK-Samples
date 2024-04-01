![](./media/image1.png)

# 简单网格着色器示例

*此示例兼容 Microsoft 游戏开发工具包（2022 年 3 月）和 Windows 10（版本 2004）2020 年 5 月更新*

# 说明

此示例与较旧的示例 SimpleTriangle 类似，不同之处在于它使用网格着色器。 它的目的只是展示获取在电脑和 Xbox Series X|S
| | |
|---|---|
|上初始化并运行网格着色器管道所需的所有部分。|

注意：Xbox One 主机系列上没有网格着色器支持，因此没有适用于该平台的生成配置。

![](./media/image3.png)

# 生成示例

如果使用 Xbox Series X|S 开发工具包，请将可用解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

如果使用具有适当硬件和 Windows 10 版本的电脑，请将活动解决方案平台设置为 Gaming.Deskop.x64。

示例不支持 Xbox One。

*有关更多信息，请参阅*&nbsp;__运行示例__（位于 *GDK&nbsp;文档）中。*

# 使用示例

| 操作 | 游戏板 |
|---|---|
| 退出 | &ldquo;视图&rdquo;按钮 |

# 实现说明

此示例演示的步骤如下所示：

1. 初始化 ID3D12Device 并请求 API 对象，以使用 DirectX12 进行呈现。

2. 使用 ID3D12Device::CheckFeatureSupport() 函数检查网格着色器功能支持。

3. 使用 ID3D12Device2::CreatePipelineState() 函数创建网格着色器管道。

4. 将根签名、管道状态和资源绑定到命令列表。 使用 ID3D12GraphicsCommandList6::DispatchMesh() 函数和必要参数调度网格绑定管道。

# 更新历史记录

2019/10/31 -- 示例创建。

2020/4/28 - 已更新为使用 D3DX12 帮助程序创建网格着色器管道

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


