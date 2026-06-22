![](./media/image1.png)

# 库轮换示例

*此示例可用于 Microsoft 游戏开发工具包（2022 年 3 月）*

# 说明

*库轮换*是一种技术，有助于加快在同一个 draw 调用中对多个呈现目标进行写入的过程。 这对向多个 GBuffer 进行写入时的延迟传递过程非常有用。 只有当目标位于 DRAM 中时斜坡旋转才有用，因为它不会对 ESRAM 中的目标产生影响。

Xbox One 和 Xbox One S 有 8 个库，Xbox One X 有 16 个库和 Xbox
X，因为所有 GBuffers 都将位于 DRAM 中。 **观察到的结果：**
| | |
|---|---|
|系列 X|S 有 2 个库。 此功能在 Xbox One 上特别有用|


使用 4 个 GBuffer 时，示例在 Xbox One 上显示大约 **10%-14%** 的增益，每个 GBuffer 分配给不同的库，使用 Xbox One X 时增益约 **20%**。 如果传递中有更多 GBuffer，则增益会增加。

# ![](./media/image3.png)生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* __运行示例__，*在 GDK 文档中。*

# 使用示例

| 操作 | 游戏板 |
|---|---|
| 更改资源分配技术 | A button |
| 退出 | &ldquo;视图&rdquo;按钮 |

# 实现说明

库轮换支持仅在使用 DirectX 12 API 时才可用。
非常相似，请参阅示例源代码。 *提交的资源：*
| | |
|---|---|
|下面是 Xbox One 的代码示例。 Xbox Series X|S 的代码为|


```cpp
ResDesc.Layout = D3D12XBOX_BANK_ROTATED_TILE_MODE(D3D12_TEXTURE_LAYOUT, bankRotationIndex);
```

然后，在创建资源时，可以将此新布局与 CreateCommittedResource 一起使用。

*已放置或组件放置的资源：*

```cpp
D3D12_GPU_VIRTUAL_ADDRESS rotatedAddress;

assert(XGComputeBankRotationAddress((XG_GPU_VIRTUAL_ADDRESS)gpuAddress, 
                                    &xgResLayout[gbufferIndex], 
                                    0, 
                                    0, 
                                    bankRotationIndex, 
                                    &rotatedAddress));
```


这个新的 rotatedAddress 可用于放置资源。

# 已知问题

无

# 更新历史记录

初始版本 2017 年 5 月

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


