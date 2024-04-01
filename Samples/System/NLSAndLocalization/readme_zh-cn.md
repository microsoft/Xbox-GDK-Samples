![](./media/image1.png)

# NLS 和本地化示例

*此示例可用于 Microsoft 游戏开发工具包（2020 年 6 月）*

# 说明

此示例演示如何本地化 MicrosoftGame.Config 中引用的字符串和资产，以及如何正确访问本地化的游戏内资源。 还演示了一些常见的 NLS（国家/地区语言支持）API。

## 主屏幕

![](./media/image3.png)

*在此图像中，控制台用户的语言设置为 es-AR*

| 操作 | 游戏板 | 键盘 |
|---|---|---|
| 选择要运行的按钮 | 方向键上/下 | 方向键/鼠标 |
| 按下按钮 | A 按钮 | Enter/左键单击 |
| 退出 | &ldquo;视图&rdquo;按钮 | Esc |

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

如果使用 Windows 10，请将活动解决方案平台设置为 `Gaming.Desktop.x64`。

*有关详细信息，请参阅* __运行示例__，*在 GDK 文档中。*

# 实现说明

# 此示例演示本地化游戏的基础知识。 示例中使用的资源分析器非常基本，不提供任何错误检查。 如果游戏打算遵循此设置，则不应直接复制此资源分析器，而应将其视为一个示例，说明功能更全面的资源分析器如何在此上下文中运行。 此示例的目标是帮助开发人员熟悉本地化资源的过程。

**枚举当前本地化设置**

# 启动应用程序时，它将显示某些常见 NLS API 的输出。 可以通过按&ldquo;枚举当前本地化设置&rdquo;按钮再次手动显示它们。

GetUserDefaultLocaleName API 检索在其中设置主机的区域设置，GetUserDefaultGeoName API 检索在其中设置主机的位置。 XPackageGetUserLocale API 检索与包区域设置最匹配的用户区域设置。 结果可以转换为 LCID。

**系统设置驱动的语言选择**

此示例将根据其应用程序语言设置更改其游戏内图像和文本。 示例的默认语言由 XPackageGetUserLocale API 确定。 XPackageGetUserLocale 应该是游戏选择区域设置的真实来源，因为它将确定与用户系统上的游戏最匹配的区域设置。 区域设置基于所有可用数据，包括主机和用户设置以及游戏支持的内容。 结果将始终为在游戏配置中声明的语言，如果未声明任何语言，则为用户语言。 然后，示例在运行时使用此区域设置来选择要显示的相应本地化图像和文本。

在示例中，MicrosoftGame.config 中定义了 7 个区域设置：

- 

- en-US

- en-GB

- zh-Hans-CN

- zh-Hant

- ja-JP

- es

- fr

选择这些区域设置以演示常见的回退方案。 当主机语言和位置设置与游戏支持的语言不完全对应，并且需要从可用选项中进行选择时，将发生回退。

例如，主机语言为&ldquo;en-CA&rdquo;时，将回退到&ldquo;en-GB&rdquo;。 另一种情况是使用&ldquo;fr&rdquo;，在此示例中，仅定义了不带区域的&ldquo;fr&rdquo;。 在这种情况下，当主机语言为法语时，无论主机语言区域是什么，它都将回退到 &ldquo;fr&rdquo;。

注意，主机设置将本地化设置划分为&ldquo;语言&rdquo;、&ldquo;语言区域&rdquo;和&ldquo;位置&rdquo;。 在 Xbox One 管理器上，首选语言合并前两个，地理区域对应于位置。 对于此示例，仅语言设置会影响语言选择;位置/地理区域仅影响 GetUserDefaultGeoName。

**MicrosoftGame.Config 字符串本地化**

示例的显示名称和说明也基于主机语言进行本地化。 如果要支持本地化，MicrosoftGame.Config 中的这些字段设置为 ms-resource 引用，例如：

OverrideDisplayName=\"ms-resource:ApplicationDisplayName\"

然后，根据 Resources.resw 文件中位于其各自区域设置下的值填充这些值。 例如，如果主机语言设置为日语 (ja)， 区域设置为日本 (JP)，则显示名称将从项目的 ja-JP 文件夹中的 Resources.resw 文件中拉取，对于此示例，该文件将是字符串 &ldquo;NLS 和本地化 (ja-JP)&rdquo;。 默认情况下，Resources.resw 文件和每个语言文件夹应位于根项目文件夹中。 如果 Resources.resw 文件位于其他位置，例如此示例使用的&ldquo;字符串&rdquo;目录（例如 ProjectFolder\\Strings\\Resources.resw，ProjectFolder\\Strings\\ja-JP\\Resources.resw 等。 ），请确保在项目属性页的&ldquo;包本地化目录&rdquo;属性中指定文件夹。

**MicrosoftGame.Config 包映像本地化**

示例的包映像也可以基于主机语言进行本地化。 首先，默认映像的路径在 MicrosoftGame.config 中照常指定。 以下代码段指示默认图像在示例中&ldquo;图像&rdquo;目录下的位置：

StoreLogo=\"Images\\StoreLogo.png\"

Square480x480Logo=\"Images\\Square480x480Logo.png\"

Square150x150Logo=\"Images\\Square150x150Logo.png\"

Square44x44Logo=\"Images\\Square44x44Logo.png\"

SplashScreenImage=\"Images\\SplashScreen.png\"

然后，在默认映像所在的同一目录中，MicrosoftGame.config 中定义的每种语言的子目录将包含本地化变体（例如 ProjectFolder\\Images\\ja-JP\\StoreLogo.png，ProjectFolder\\Images\\ja-JP\\Square480x480Logo.png 等）。 无论使用哪种语言，都会先检查该特定语言文件夹中是否有图像文件；如果有，就使用该文件。 另外，如果语言文件夹中没有映像，或者该语言不受不支持（即语言文件夹缺失），则使用默认资源。 请确保在生成示例时将每个徽标正确复制到&ldquo;Loose&rdquo;文件夹。

请注意，如果与 MicrosoftGame.config 设置对应的产品实际已发布，并且该沙盒（或零售）中的帐户已登录，则字符串和映像可以由在合作伙伴中心产品页面中配置的等效字段替代。 合作伙伴中心中也不需要游戏和应用商店一览支持的语言的 1 对 1 对应关系，但这会很有帮助。

# 更新历史记录

2020 年 4 月初始版本

2021 年 5 月更新示例以支持其他 NLS 功能，包括 UX 刷新

2021 年 7 月 MicrosoftGame.config 已更新

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


