![](./media/image1.png)

# Franchise Game Hub sample

此示例与 Microsoft GDK （2023 年 10 月）兼容

# 说明

专享中心是一种让发布者以使用户在游戏之间共享体验的方式提供相关游戏的特选组织。 它充当获取、安装和启动相关游戏的中心点，并托管相关游戏可以使用的持久性本地存储空间。

此示例演示如何实现用户体验所需的许多操作。 它包含三个项目：
- 一个游戏中心产品 （GameHub），用于显示与游戏相关的游戏以及可以为每个游戏执行的操作
- 中心感知产品 （RequiredGame），需要游戏中心启动并能够读取中心写入的共享数据
- 不需要游戏中心启动的中心感知型产品 (RelatedGame)；这表示仍希望与游戏中心关联的较旧游戏，但无法利用任何特定于游戏中心的字段和 API，因为它已在较旧的 GDK 上提供

![](./media/image3.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Windows 电脑，请将活动解决方案平台设置为 `Gaming.Desktop.x64`。请注意，虽然代码应编译，但&ldquo;游戏中心&rdquo;主要是主机功能，某些功能可能不起作用。

*有关详细信息，请参阅* *GDK 文档中*的&nbsp;__运行示例__&rdquo;。

# 运行示例

此示例配置为在 XDKS.1 沙盒中工作。 购买和安装操作需要测试帐户。 任何 @xboxtest.com 测试帐户都应在 XDKS.1 沙盒中工作。

![](./media/screenshot1.jpg)

第一行显示与游戏中心相关的游戏，在本例中，有两个游戏。 选择每个游戏后，下方会列出其每个关联的加载项。 可以购买每个产品，在安装了程序包的游戏或 DLC 的情况下。

安装后，可以启动每个游戏。 中心感知游戏将能够显示写入共享 PLS 的文件的内容，以及转换回中心。

![](./media/screenshot2.jpg)


无法启动不了解中心的游戏 -否则，它不知道它来自中心。

所有这些都可以通过通过Visual Studio调试运行的游戏中心来完成，但脚本可用于将每个项目放入包中：
- `makepcpkg.cmd`
- `makescarlett.cmd`
- `makexboxone.cmd`

生成包后，使用 `xbapp install` 安装每个包。这允许与本地生成的包进行交互，而无需从 Store 获取和安装。

当尝试测试更新时，这一点尤为重要，因为本地生成（已部署或打包）无法更新到 Store 内部版本。 为此，UpdateTesting 目录中提供了一组单独的脚本：
- `buildpackages.cmd`：为关联的游戏生成 v100 和 v200 版本的包，并为每个游戏生成 DLC
- `installandstageupdates.cmd`： `xbapp install` 每个包的 v100 和 `xbapp update /a` v200，模拟更新的可用性。

其结果应该是游戏应反映每个更新都可用，并启用更新流。

![](./media/screenshot3.jpg)


# 实现说明

`XPackageEnumeratePackages` 似乎可以同时 `ThisAndRelated` 返回和 `ThisPublisher` 作用域的相同结果。 若要查看差异，请安装 XDKS.1 沙盒中可用的其他示例之一，例如 InGameStore，DownloadableContent。

RelatedGame（中心感知）如何与 GameHub 使用 `RelatedProduct` GameHub 的 microsoftgame.config 中的节点相关。

如何通过在各自的 microsoftgame.config 中将 GameHub 与 RequiredGame `AssociatedFranchiseGameHubId` 进行匹配，使 RequiredGame（中心感知）与 GameHub `FranchiseGameHubId` 相关。

主要区别在于，无需重新发布不区分中心的游戏即可作为 GameHub 中引用的游戏进行添加，但中心感知游戏是从一开始就使用&ldquo;专享游戏中心&rdquo;方案创建的。 这也是中心感知游戏可以返回 GameHub 的原因，因为它知道其 titleId 能够 `XLaunchUri` 返回到。

2023 年 10 月恢复中将不提供 UI 更改，进一步说明游戏中心的工作方式，即中心感知游戏不会显示在&ldquo;我的游戏&rdquo;中，并且只能与游戏中心一起安装和启动。

不应有任何要求，要求使用相同的 GDK 生成游戏中心和关联的游戏。 RelatedGame 可以设置为使用早于 2023 年 10 月的 GDK 进行生成。 RequiredGame 和 GameHub 不能，因为它们依赖于 2023 年 10 月新增的 API 和 microsoftgame.config 字段。

发生这种情况时 `XStoreEnumerateProductsQuery` ，是有意 `XStoreProductsQueryHasMorePages` 而非调用的。 首先，此示例涉及的产品很少。 其次，在 2023 年 10 月，只需展开传入 `XStoreQueryAssociatedProducts` 的 maxItems 或 `XStoreQueryAssociatedProductsForStoreId` 包括游戏的预期产品数，所有产品都将返回到 Result 函数的查询句柄中。 仅当枚举了所有产品时，才会命中回调，当然，对于具有许多产品的游戏，这可能需要一段时间。

# 已知问题

请考虑将任何电脑功能视为原型;此示例适用于主机。

当中心处于脱机状态（`XStoreQueryAssociatedProductsForStoreId` 不脱机工作）时，无法区分 DLC。

`XStoreQueryGameAndDlcPackageUpdates` 或 `XStoreQueryPackageUpdates` 使用多个 ID 传入在使用暂存时 `xbapp update` 不一致地返回可用更新。

通过 `XStoreDownloadAndInstallPackageUpdates` 安装更新并使用 `XPackageCreateInstallationMonitor` 监视更新时，`XPackageGetInstallationProgress` 将提前返回 `completed` = true。

# 更新历史记录

**初始版本：** 2023 年 10 月

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


