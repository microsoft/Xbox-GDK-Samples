![](./media/image1.png)

# CMaskDecode 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

# 此示例演示了如何从 cmask 图面提取快速清除值。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关更多信息，请参阅*&nbsp;__运行示例__（位于 *GDK&nbsp;文档）中。*

# 使用示例

![](./media/image2.jpeg)

| 操作 | 游戏板 |
|---|---|
| 切换 MSAA | A button |
| 切换子磁贴显示 | B button |
| 旋转视图 | 左控制杆 |
| 重置视图 | 左控制杆（单击） |
| 退出 | &ldquo;视图&rdquo;按钮 |

# 实现说明

cmask 图面是与呈现器目标关联的元数据图面。 呈现目标的每个 8x8 磁贴都有一个关联的 4 位 cmask 条目。 cmask 条目包含以下信息：

• 如果未启用 MSAA，则每个 cmask 位表示一个 16 像素子磁贴的&ldquo;清除&rdquo;状态（子磁贴的形状取决于数据格式，即微平铺模式
cMask [解码](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/cmask-decoding)） • 如果启用了 MSAA，则两个 cmask 的高顺序位分别表示一个 32 像素子磁贴的&ldquo;清除&rdquo;状态（对于确切的子磁贴形状，请参阅 [CMask 解码](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/cmask-decoding) 文档）。 两个低顺序位包含 fmask 元数据。
| | |
|---|---|
|Xbox One 和 Xbox Series X|S 上的重排模式，请参阅文档中的更多详细信息|


此示例演示如何将 cmask 解码到纹理上，以及如何解释快速清除位。 此信息可用于加速基础图面的清晰区域的检测。 一个应用程序（此处未演示）可能跳过最为空纹理的快速清除消除。

该示例将解码的快速明文信息显示为主场景上的红色/绿色覆盖。 绿色像素属于完全清晰的磁贴（或子磁贴）。 红色像素属于部分写入的磁贴（或子磁贴）。

此示例使用 DirectX 12.X 实现。 如果删除根签名，则可以将相同的着色器 (ColorDecompressUtility.hlsli / CMaskDecodeCS.hlsl) 用于 DirectX 11.X 实现。

# 已知问题

此示例取决于某些驱动程序的选择（这些选择将来可能发生变化）：

• cmask 图面是以平铺图面还是线性图面创建

| | |
|---|---|
| • 平铺模式（在 Xbox One 上）或重排模式（在 Xbox Series X | S 上）为 cmask 图面选择 |

# 更新历史记录

示例的原始版本是使用基于 XSF 的框架编写的。 2 月，示例被改写为使用 ATG 示例模板。
2020 年 5 月。
| | |
|---|---|
|2018 年支持 Xbox One X。 添加对 Xbox Series X|S 的支持|

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


