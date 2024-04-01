![](./media/image1.png)

# 高级 ESRAM 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

此示例演示了如何使用高级 DirectX 12.x 内存功能有效地为 D3D 资源的内存添加别名。 此示例核心中的 API 是  ID3D12CommandQueue::CopyPageMappingsX 和 ID3D12CommandQueue::CopyPageMappingsBatchX。 这些功能提供将 CPU 页面表条目复制到 GPU 时间线上的 GPU TLB 的功能，从而允许虚拟 D3D 资源在动态情况下映射到内存页面。

此示例利用此功能实现一个临时资源分配器，该分配器将单个 ESRAM 和 DRAM 块映射到 64 KB 页粒度。 这使帧的 GPU 内存使用量保持最佳压缩，用于利用 ESRAM 的完整功能。 其接口镜像 XG 内存库中 XGMemoryLayout 的页面映射函数的指向方面。

此示例仅呈现禁用了所有 ESRAM 选项和可视化效果的场景。 ![](./media/image2.jpeg)
| | |
|---|---|
|注意：Xbox One X和 Xbox Series X|S 没有 ESRAM。 在这些平台上|


# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* *GDK 文档*中的__运行示例__。&nbsp;

# 使用示例

该示例的主要功能允许操作分配暂时性纹理资源的位置。 框架中使用的资源是场景颜色、场景深度、两种用于轮廓的纹理和两种用于开花的纹理。 资源的 ESRAM 组合在最左侧显示为资源内存的百分比。 ESRAM 布局的可视化效果提供对每个纹理的 ESRAM 和 DRAM 组合的更改的即时反馈。 每个资源的 ESRAM 占用量沿 Y 轴显示，而其生存期沿 X 轴可见。 可以使用按下按钮刷新沿临时轴使用的 GPU 计时。

## 控件

| 操作 | 游戏板 |
|---|---|
| 将摄像头移向/移离原点 | 左控制杆向上/向下 |
| 旋转摄像头 | 右控制杆 |
| 重置摄像头 | 右控制杆（单击） |
| 循环暂时性纹理 | 方向键向左键/向右键 |
| 更改 ESRAM 百分比 | 方向键上/下 |
| 循环突出显示的对象 | 左/右缓冲键 |
| 刷新时间线 | A 按钮 |
| 退出 | &ldquo;视图&rdquo;按钮 |

# 实现说明

创建一个大型虚拟地址空间，用于映射从 ESRAM 和 DRAM 分配的 64 KB 内存页（页池）块。 使用 ID3D12Device::RegisterPagePoolX 向 DirectX12 注册页面池，并在不再使用时使用 ID3D12Device::UnregisterPagePoolX 取消注册。 此映射用于暂存 CPU 页表条目，以便直接复制到 GPU 页表。

ID3D12CommandQueue::CopyPageMappingsX 或 ID3D12CommandQueue::CopyPageMappingsBatchX 函数使这些页池中的页面范围能够映射到 GPU 时间线上的指定 GPU 虚拟地址。 这使虚拟 D3D 资源可以灵活地动态映射到 64 KB 物理页。 此功能使资源之间的内存别名变得普通，整个堆非常有趣！

![](./media/image3.png)

图 1：示例中使用的内存映射范例。 ESRAM（如果可用）映射到虚拟地址空间的前 32 MB，同时根据需要追加 4 MB DRAM 页池。

页面块的创建和管理由 PageAllocator 类执行。 提供了虚拟地址范围，并且分配器根据需要按顺序将页面池映射到此范围。 然后，使用&ldquo;RegisterPagePoolX&rdquo;向 DirectX12 注册页面池。 分配器页的使用情况已完全跟踪 - 它分配从第一页到最后一页的页面，在释放后替换页面。

TransientCache 负责管理虚拟 D3D 资源。 这些资源是按需创建的，但会进行缓存，以避免重新创建常见资源时产生不必要的开销。 缓存这些资源的内存开销实际上为零，因为它们只分配虚拟地址空间。 每个资源每个帧只能分配一次。

此 TransientAllocator 类使用页面分配器和暂时性缓存来满足用户的资源请求。 请求资源时，它会从 TransientCache 中获取实例。 然后，它从 PageAllocators 分配所需的页数，分析令牌以确定是以页面级粒度使用 ESRAM 还是 DRAM。 然后生成适当的结构，以便稍后提供给&ldquo;CopyPageMappingsBatchX&rdquo;，即D3D12XBOX_PAGE_MAPPING_BATCH和D3D12XBOX_PAGE_MAPPING_RANGE结构的矢量。

![示例屏幕截图](./media/image4.png)

图 2：将虚拟资源映射到页面池中的页面范围以满足其内存需求。 这些映射是 CopyPageMappingsX 和 CopyPageMappingsBatchX 调用的结果。 为简单起见，此可视化效果中仅使用了两个资源，并且没有内存别名。 但是，内存别名是此技术的预期优势。

由于内存别名，TransientAllocator 还负责在必要时执行着色器和缓存刷新。 在 DirectX12 中，刷新作为资源屏障的一部分插入。 由于我们避开了此系统以执行内存别名，因此必须手动插入自己的刷新。 TransientAllocator 通过检查资源的关联视图来确定应刷新哪些着色器阶段和缓存。

最后，在将使用已分配的暂时性资源的命令列表提交到其命令队列之前，必须在 TransientAllocator 上调用&ldquo;Finalize&rdquo;以完成资源映射。 此时，CopyPageMappingsBatchX 调用放置在命令队列上，该队列设置要在后续命令列表中使用的资源的内存映射。

# 更新历史记录

2018/8/6 &ndash; 创建示例。

2019/12/17 -- 移植到 Microsoft GDK。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


