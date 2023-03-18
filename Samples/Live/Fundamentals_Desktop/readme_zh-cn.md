  ![](./media/image1.png)

#   电脑基础知识示例

此示例与 Microsoft 游戏开发工具包（2019 年 11 月）兼容

# 

# 说明

本示例演示如何登录 Xbox Live
并进行许可证检查，以确保游戏由当前已登录用户所有。另外，它还会执行更新检查来确保应用是应用商店中发布的最新版本。

![](./media/image3.png)

# 必备组件

-   Windows SDK 10.0.18362.1

-   Microsoft GRDK 190500（2019 年 5 月）10.1.18362.1021

-   Visual Studio 2017

-   首先登录到 Xbox 应用，然后在沙盒 XDKS.1 中登录到应用商店应用的 Xbox
    测试帐户

> 使用测试帐户获取应用商店中示例的许可证（运行\
> `ms-windows-store://pdp/?productid=9NRL15W975GM`）

# 构建示例

该示例配置为使用 VS2017
进行构建。此示例仅适用于电脑，因此仅提供平台"Gaming.Desktop.x64"。

# 运行示例

使用 XStore API
的一个关键特点是，它们需要有效的许可证才能正常工作。在启动时，会通过调用许可服务对此进行验证。通常，如果没有有效的许可证，API
将返回 0x803f6107，表示找不到有效的许可证。

若要为测试帐户获取有效的许可证，请运行以下命令，直接访问示例产品的应用商店页面：

`ms-windows-store://pdp/?productid=9NRL15W975GM`

请注意，你需要位于 XDKS.1 沙盒中，然后使用你的测试帐户登录到 Xbox
应用\*，再使用同一测试帐户登录到 Windows 应用商店。

从应用商店安装的示例将获得适当的许可并正常运行，但可能代表示例的旧版本。若要让
Visual Studio 中构建的示例正常运行，需要进行一些额外设置。如果是 2019 年
11 月版 GDK，使用 F5
运行示例将无法正确注册你的调试版本，也无法链接到相应的许可证信息。此外，部署松散的版本也无法正确执行更新下载和安装方案。

## 启动示例

若要启用本地构建的松散版本，需要在下面的步骤中运行 add-appxpackage
命令。这将使用随附的 MicrosoftGame.config
注册已构建示例，它与从应用商店下载的已绑定许可证的包具有相同的名称和标识。

最后，对于 2019 年 11 月版
GDK，必须从"开始"菜单（如果已固定任务栏，也可以使用任务栏）启动该应用，许可证才能正常工作。不能使用
F5 来运行，也不能直接执行 .exe，这将导致出现 0x803f6107 错误。

若要使本地构建的示例版本准备好运行，请执行以下操作：

1.  [将沙盒切换为
    XDKS.1](https://docs.microsoft.com/en-us/gaming/xbox-live/xbox-live-sandboxes)

2.  使用测试帐户登录到 Xbox 应用\*（任何测试帐户都能在此沙盒中正常运行）

3.  使用相同的测试帐户登录到 Windows 应用商店应用

4.  构建示例

5.  对于 2019 年 11 月版 GDK 之后的版本：使用 F5

6.  对于 2019 年 11 月版 GDK：

    a.  打开 2017 Visual Studio 命令提示符

    b.  运行以下命令以按照上述说明注册应用\
        wdapp register \[Gaming.Desktop.x64\\Debug 文件夹的绝对路径\]

    c.  从"开始"菜单启动应用（如果按 F5 或直接运行 .exe，将导致检查
        Store API 的结果时出现错误 0x803f6107）

    d.  按需附加调试器

\* 对于 Xbox 应用，可以是 Xbox
主机小帮手应用，也可以是显示商店界面并提供 Game Pass 信息的新 Xbox
应用。对于后者，当你切换到与应用商店应用中的帐户不匹配的新帐户时，它将提示你匹配帐户。

## 测试更新

只有当内容 ID 与从应用商店中获得的已发布包的 ID 匹配时
(1062A2A1-C314-4DDC-94A2-424693687D97)，松散部署的版本（相对于打包版本）才能检查更新可用性（即，`XStoreQueryGameAndDlcPackageUpdatesAsync`）。可在以下注册表项中对此进行验证：

`HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Store\\ContentId\\41336MicrosoftATG.ATGSimpleLiveSample_dspnxghe87tn0`

如果示例应用已完全安装在应用商店中，应能相应地正确设置；否则，如果第一个应用安装实例使用的是松散版本，则可能需要手动设置。

若要实际测试下载并应用更新，必须使用打包版本：

1.  使用 createmsixvc.cmd 创建 v1 包

2.  wdapp install \<v1 msixvc\>

3.  编辑 microsoftgame.config 以递增版本

4.  在 Visual Studio 中部署应用，以确保将更改复制到
    Gaming.Desktop.x64/Debug

5.  使用 createmsixvc.cmd 创建包含新版本的 v2 包

6.  wdapp update \<v2 msixvc\> /m

此时暂存 v2 作为可用的强制更新。现在，启动（在步骤 2 中安装的）v1
后，将检测到可用更新，并且将显示"下载和安装"按钮。单击此按钮将终止应用，并模拟更新游戏的过程。

完成更新后，启动标题将运行 v2。可以在 Powershell
中运行已安装的版本来验证它：

`get-appxpackage 41336MicrosoftATG.ATGSimpleLiveSample`

这是唯一可测试的更新流程。由于签名存在差别，即使它具有更高的版本，也无法将任何已发布的应用商店包检测为更新。应用商店包将只会更新为更高版本的应用商店包。

# 实现说明

请注意，如果有多个用户登录，则会将 StoreContext
分配给用户更改回调中的最新帐户，该帐户有可能会与示例中显示的帐户不匹配。在多用户应用场景中，应用商店操作确实不太好用，因此将
StoreContext 分配给按了 A 按钮的帐户通常是合适的。使用 Xbox (beta)
应用登录将是检测并确保帐户一致性的最佳方式。

# 隐私声明

在编译和运行示例时，会将示例可执行文件的文件名发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。

# 更新历史记录

初始发布：2019 年 4 月

更新时间： 2020 年 1 月
