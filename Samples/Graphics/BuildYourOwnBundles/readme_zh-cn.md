![](./media/image1.png)

# 生成自己的捆绑包

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

在 Xbox One 上，可以直接将数据写入缓冲区，GPU 可以使用 ExecuteIndirectBundleX API 直接读取和执行这些数据。 此示例涵盖了用于写出此数据并将其与常规绘图和绘图捆绑包进行比较的各种技术。 缓冲区数据可以由 CPU 或 GPU 写入，并且可以在后续阶段中重新送入 GPU。 这种功能称为创建你自己的捆绑包 (BYOB)。 所有&ldquo;高频率&rdquo;API 调用都可以直接写入缓冲区。 代码与 d3d12_x.h 中提供的代码相同，但它会写入所提供的缓冲区，而不是直接将其写入命令缓冲区。

将数据写入要在 GPU 上执行的缓冲区时，需要注意几个事项：

- 应使用 PAGE_GPU_EXECUTE 标志创建缓冲区

- 要执行的缓冲区大小不应超过 4MB

- 如果传入了错误的数据包信息，则很容易挂起 GPU

- 有 NO_OP 数据包可用于提供 GPU 忽略的一些自定义数据

- 如果使用写入组合缓冲区从 GPU 进行写入，请注意所有写入组合规则。

此示例显示多个绘制网格实例的方法。 使用多个随机分配给实例的 PSO。 每个方法的 GPU 时间相同。 对于 16384 网格，它
80.3 毫秒而不进行剔除，39.2 毫秒（绘制了 7439 个网格）。 CPU 时间各不相同，并随方法说明一起列出。 绘图方法：

- **BYOB - 更新每个实例并绘制**

此技术更新每个实例的 BYOB 捆绑数据，然后为该实例调用 ExecuteIndirectBundleX。 使用此技术的缓冲区大小较小，因为它仅包含单个实例的数据。 它将每个实例的绘图数据分别发送到 GPU，一个接一个地发送到 GPU。 CPU： 无 Cull = 7.01 毫秒，Cull = 6.65 毫秒

- **BYOB - 更新所有实例，然后绘制**

更新所有实例的 BYOB 捆绑数据并将其写入单个缓冲区，最后将缓冲区传递给 GPU。 用于此技术的缓冲区将具有比上一种技术更大的大小，但所有数据都会在单个 ExecuteIndirectBundleX 中发送到 GPU。 CPU：无 Cull = 33.5 毫秒，Cull = 19 毫秒

- **BYOB - 在运行时生成捆绑包**

在运行时生成整个缓冲区，然后在单个 ExecuteIndirectBundleX 命令中发送缓冲区。 CPU： 无 Cull = 7.4 毫秒，Cull =
6.8 毫秒

- **BYOB - 使用 GPU 绘制**

编写 GPU 数据包数据，以使用计算着色器将所有网格实例绘制到缓冲区中，然后使用 ExecuteIndirectBundleX 将该缓冲区传递给 GPU。 在 GPU 上执行剔除。 CPU： 无 Cull = 6.0 毫秒，Cull = 6.0 毫秒

- **使用捆绑包绘图**

此技术使用普通捆绑包使用 ExecuteBundle API 绘制数据。 CPU： 无 Cull = 6.9 毫秒，Cull = 6.5 毫秒

- **直接绘图**

使用 DrawIndexedInstanced 直接绘制网格。 CPU： 无 Cull =
7.7 毫秒，Cull = 6.9 毫秒

CPU 上的所有缓冲区写入都是使用单个线程执行的。 使用多个线程写入可执行缓冲区时，将具有性能优势。 如果每个状态设置和绘制的大小保持不变，则多个线程可以轻松地写入 CPU 上的缓冲区。 NO_OP数据包可用于填充缓冲区中未使用的空间。 通过 GPU 写入数据包数据可减少 CPU 时间，写出数据包数据的成本不高。 此方法是示例中测试的最快方法。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* __运行示例__，*在 GDK 文档中。*

# 使用示例

## 屏幕截图

![](./media/image3.png)

| 操作 | 游戏板 |
|---|---|
| 更改绘图技术 | A 或 B 按钮 |
| 剔除网格 | X button |
| 显示/隐藏有关所选技术的更多说明 | Y button |
| 退出 | &ldquo;视图&rdquo;按钮 |

# 实现说明

WriteOwnBundlesHelper.h 中提供了用于写出捆绑包的示例中使用的所有代码。 将数据包数据写入缓冲区基于头文件 d3d12_x.h 中的代码。

例如，DrawIndexedInstanced 定义为：

```cpp
D3DINLINE void D3DAPI DrawIndexedInstanced(
    _In_ UINT IndexCountPerInstance,
    _In_ UINT InstanceCount,
    _In_ UINT StartIndexLocation,
    _In_ INT BaseVertexLocation,
    _In_ UINT StartInstanceLocation)
{
    D3D12XBOX_PPUT pPut = m_Putter.m_pCurrent;
    if (pPut < m_Putter.m_pLimit_Draw)
    {
        m_Putter.PutD(pPut, D3D12XBOX_PACKET_DRAW_INDEXED_INSTANCED);
        m_Putter.PutD(pPut, InstanceCount);
        m_Putter.PutD(pPut, StartIndexLocation);
        m_Putter.PutD(pPut, IndexCountPerInstance);
        m_Putter.PutD(pPut, BaseVertexLocation);
        m_Putter.PutD(pPut, StartInstanceLocation);
        m_Putter.m_pCurrent = pPut;
    }
    else
    {
        CommandListFunction()-\>DrawIndexedInstanced(this,
        IndexCountPerInstance,
        InstanceCount,
        StartIndexLocation,
        BaseVertexLocation,
        StartInstanceLocation);
    }
}
```


写出捆绑包时，只需将其更改为：

```cpp
void DrawIndexedInstancedBYOB(
    _Inout_ UINT32** writeAddress,
    _In_ UINT IndexCountPerInstance,
    _In_ UINT InstanceCount,
    _In_ UINT StartIndexLocation,
    _In_ INT BaseVertexLocation,
    _In_ UINT StartInstanceLocation)
{
    **writeAddress = D3D12XBOX_PACKET_DRAW_INDEXED_INSTANCED;
    *(*writeAddress + 1) = InstanceCount;
    *(*writeAddress + 2) = StartIndexLocation;
    *(*writeAddress + 3) = IndexCountPerInstance;
    *(*writeAddress + 4) = BaseVertexLocation;
    *(*writeAddress + 5) = StartInstanceLocation;
    *writeAddress += 6;
}
```


# 已知问题

使用 ExecuteIndirectBundleX 时，PredicationBuffer 当前不起作用。 Pass 0 for PredicationBufferOffset。

# 更新历史记录

- 2019 年 4 月：初始版本

- 2019 年 11 月：更新为支持项目 Xbox Series X|S


