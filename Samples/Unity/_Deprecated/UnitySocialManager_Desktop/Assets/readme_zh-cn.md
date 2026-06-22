![](./media/image1.png)

# 适用于桌面的 Unity 社交管理器

*此示例与以下内容兼容：*

- *具有 Xbox 扩展的 Microsoft 游戏开发工具包（2022 年 3 月）和更高版本*

- *Unity 编辑器 2021.3.4f1 及更高版本*

- *[GDK Unity 程序包](https://github.com/microsoft/gdk-unity-package)*

# 说明

适用于桌面的 Unity 社交管理器示例演示了如何使用 Unity 游戏引擎来使用 Xbox Live 社交管理器。 可以根据不同的状态和关系级别更改好友组，以从 Xbox 服务 API 中查找特定的用户组。

![自动生成的图形用户界面、应用程序描述](./media/image3.png)

# 可记笔记的代码文件

**XboxManager.cs**：包含 Xbox GDK 和 Xbox Live 服务 API 的初始化，以及登录用户和查询各种信息（如沙盒和游戏 ID）。

**XboxSocialManager.cs**：包含演示 Xbox Live 社交管理器使用情况的 API。

# 生成示例

**重要说明：** 该示例 **需要** [*GDK Unity 程序包*](https://github.com/microsoft/gdk-unity-package)。 为了成功生成示例，必须包括程序包中提供的&ldquo;*GDK-API&rdquo;*和&ldquo;*GDK-Tools&rdquo;*。 将包含的&ldquo;*GDK-Tools*&rdquo;合并到预配置&ldquo;*MicrosoftGame.config*&rdquo;文件的导入的&ldquo;*GDK-Tools&rdquo;*中。 &ldquo;*Project&rdquo;*文件夹应镜像下图。

![自动生成的图形用户界面、文本、应用程序描述](./media/image4.png)

为 *Unity GDK* 生成需要使用 *GDK 生成器* 而不是 Unity 生成菜单。 生成器通过 Unity 菜单栏中的 *GDK -\> PC -\>&ldquo;生成和运行&rdquo;*选项提供。

![自动生成的图形用户界面、文本、应用程序描述](./media/image5.png)

*有关详细信息，请参阅* *GDK 文档中的*[运行示例](https://docs.microsoft.com/en-us/gaming/gdk/_content/gc/get-started-with-pc-dev/get-started-with-unity-pc/gdk-unity-end-to-end-guide)。

# 运行示例

你需要登录 Xbox Live 的测试帐户来执行社交组更改和检索用户状态。

桌面的沙盒 **必须** 设置为 XDKS.1。

**重要提示：** 若要在&ldquo;找到的 Xbox 用户&rdquo;部分查看结果，必须至少有一位通过好友或收藏夹链接到登录 Xbox 用户的好友。 刷新会在某些 Xbox 服务事件中自动触发，例如状态更改或社交组更新。

*社交组命令：*

- &ldquo;*全部好友&rdquo;*

   - XblPresenceFilter = XblPresenceFilter.All

   - XblRelationshipFilter = XblRelationshipFilter.Friends

- &ldquo;所有收藏夹&rdquo;

   - XblPresenceFilter = XblPresenceFilter.All

   - XblRelationshipFilter = XblRelationshipFilter.Favorite

- *&ldquo;所有在线好友&rdquo;*

   - XblPresenceFilter = XblPresenceFilter.AllOnline

   - XblRelationshipFilter = XblRelationshipFilter.Friends

- *&ldquo;在线好友&rdquo;*

   - XblPresenceFilter = XblPresenceFilter.TitleOnline

   - XblRelationshipFilter = XblRelationshipFilter.Friends

**重要提示：** 它们使用 Xbox 服务状态和关系筛选器的预定义组合来确定与调用 Xbox 用户相关的已检索 Xbox 用户。

*找到 Xbox 用户命令：*

- &ldquo;*玩家标记...状态&rdquo;* - 显示游戏玩家标记和找到的前五个用户的连接状态，这些用户符合当前社交组设置的社交条件。 选择后，它还会通过 Xbox UI 显示用户的 Xbox 档案。

**重要提示：** 仅当用户满足预定义筛选器中的给定条件时，才会显示这些内容。 否则，它们将为空白。

*其他命令：*

- &ldquo;*登录&rdquo;* - 允许新用户使用 Xbox 用户选择 UI 登录。

- &ldquo;*刷新&rdquo;* - 手动刷新 Xbox 用户的用户列表。

- &ldquo;*清除日志&rdquo;* - 清除所有现有日志的主机。

- &ldquo;*关闭&rdquo; - * 关闭示例。

# 已知问题

此示例是针对本文档中的程序包和版本开发和测试。 使用任何较新版本的 GameCore 或 Unity 编辑器可能会导致生成失败和不兼容。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。

# 更新历史记录
2023/5/18 - 添加了重要的代码文件


