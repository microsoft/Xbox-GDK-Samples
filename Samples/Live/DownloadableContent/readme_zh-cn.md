  ![](./media/image1.png)

#   可下载内容(DLC)示例

*Windows 电脑:此示例与 Microsoft GDK (2020 年 6 月)兼容*

*Xbox One/Xbox Series X|S:此示例与 Microsoft GDKX (2020 年 6 月)兼容*

# 

# 说明

此示例演示了如何通过 XPackage 和 XStore API
实现可下载内容的购买、下载、枚举和加载。

![ビデオゲームの画面のスクリーンショット 低い精度で自動的に生成された説明](./media/image3.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Xbox Series X|S 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

如果使用 Windows 电脑，请将活动解决方案平台设置为 Gaming.Desktop.x64

*有关详细信息，请参阅 GDK 文档中的"*运行示例"*。*

# 运行示例

此示例配置为在 XDKS.1
沙盒中运行，但并不严格要求在许可模式下在此沙盒中运行此示例。

屏幕左侧将显示已安装的包。可以"装载"/"卸载"包，这涉及到检查许可证。如果可以成功装载包，则示例将显示来自包的图像。无需获得许可即可使用此功能，但需要在本地安装以下所述的
DLC 包。

如果示例使用有权使用 Microsoft Store 产品的测试帐户在 XDKS.1
中运行(见下文)，则右侧将显示可用的
"持久"加载项列表。如果该项不归该帐户所有，则选择该项将显示购买
UI；如果归该帐户所有，则选择该项会下载包。完成后，包应显示在左侧列表中。这最准确地表示了从
Microsoft Store 购买 DLC 并从 CDN 安装包的实际零售流。

| 操作                       |  键盘               |  手柄               |
|----------------------------|--------------------|--------------------|
| 选择包                     |  箭头键向上和向下   |  方向键向上和向下   |
| 在本地包或 Microsoft Store 包之间切换 |  箭头键向左和向右  |  方向键向左和向右 |
| 装载或卸载包(左列) 购买或下载包(右列) |  Enter  |  A 按钮 |
| 切换 XPackageEnumeratePackages 种类和范围 |  向上/向下翻页  |  左缓冲键/右缓冲键 |
| 刷新枚举包                 |  Y                  |  Y 按钮             |
| 退出                       |  Esc                |  视图按钮           |

XStore API
需要有效的许可证才可正常运行，并需要应用特定的配置操作。请参阅标题为**"启用
XStore 开发和测试"**的 GDK 文档节，从而了解更多的详细信息。

如果错误执行此操作，则 XStore API 将返回 0x803f6107
(IAP_E\_UNEXPECTED)，指示找不到有效许可证。

# 如何设置产品

此产品的 Store ID 为 9NQWJKKNHF1L。

要访问其 Microsoft Store 页面，请从"游戏"命令提示符使用

`xbapp 启动 ms-windows-store://pdp/?productid=9NQWJKKNHF1L`

或仅在 Windows 上使用 `ms-windows-store://pdp/?productid=9NQWJKKNHF1L`。

![ロゴ が含まれている画像 自動的に生成された説明](./media/image4.jpeg)

在撰写本文时，9NQWJKKNHF1L 包含三个加载项，表示可用平台的包的常见组合:

-   9P96RFVJQ562 包含 Xbox Series、Xbox One GDK 和电脑的包

-   9PPJJCWPCWW4 包含 Xbox One ERA 包

-   9PGJRLSPSN3V 包含 Xbox One GDK 包和电脑

在 Scarlett 开发工具包上运行的示例应能够访问 Scarlett DLC
(9P96RFVJQ562)包，且从 Microsoft Store 安装的包应具有 \_xs 后缀。在 Xbox
One 开发工具包上运行的示例应能够访问所有三个包，且对于
9P96RFVJQ562，包会改为具有 \_x 后缀。在电脑上运行的示例应只能访问
9P96RFVJQ562 和 9PGJRLSPSN3V 包。

从 Microsoft Store
安装的示例将获得适当许可并正常运行，但可能表示示例的旧版本。

# 使用本地包运行

虽然此示例可以使用从 Microsoft Store 下载并安装的 DLC
包运行，但典型的开发将涉及 DLC
内容本地迭代。可通过多种方式实现此目的。有关详细信息，请参阅名为**"管理并许可可下载内容"**的
GDK 文档。

其中同样包含多个脚本文件，它们用于生成示例和 DLC
的打包版本。对于示例(即基础游戏)，makepcpkg、makexboxonepkg、makecarlettpkg
将创建各自的包。这些脚本将使用正确的 contentID (与在合作伙伴中心上为
9NQWJKKNHF1L 提交的包关联)生成包。

对于旁加载游戏包，必须替换 EKBID。

`xbapp setekbid 41336MicrosoftATG.ATGDownloadableContent_2022.3.8.0_neutral\_\_dspnxghe87tn0 {00000000-0000-0000-0000-000000000001}`

注意，对于 Xbox One 和 Scarlett，必须将正确的 **TargetDeviceFamily**
节点插入 Gaming.Xbox.\*.x64\\Layout\\Image\\Loose\\MicrosoftGame.config
中，否则 makepkg 将显示错误:

```xml
<ExecutableList>
    <Executable Name="DownloadableContent.exe"
        Id="Game"
        TargetDeviceFamily="Scarlett"/\>
</ExecutableList>
```

对于 DLC，DLCPackage 目录包含针对以下内容的所有必需文件

1.  Scarlett GDK DLC (\_xs.xvc)

2.  Xbox One GDK DLC (\_x.xvc)

3.  Xbox One ERA DLC (无扩展)

适用于 Xbox；DLCPackagePC 将包含电脑 .msixvc 所需的文件。

每个都包含 makedlcpkg 命令，这些命令将生成每个平台的 DLC 包。

借助这些功能，可以为游戏和 DLC 生成打包版本。要进行安装，请使用 Xbox
上的 **xbapp 安装**或适用于电脑的 **wdapp
安装**或可用的等效工具。在此配置中，任何已安装的 DLC
都应显示在左侧并可装载，即使示例本身未在许可模式下运行也是如此。

也可以使用松散文件完全运行。要实现此目的，请在 Xbox 上使用 **xbapp
部署**或在电脑上使用 **wdapp 注册**，并传入 MicrosoftGame.config
所在的目录，例如

`xbapp 部署 .\\DLCPackage\\Package_Scarlett`

`wdapp 注册 .\\DLCPackagePC\\Package`

应可以混合并匹配: 打包的基础游戏 + 松散 DLC；松散的基础游戏 + 打包的
DLC、松散的基础游戏 + Microsoft Store DLC
等。任何有关某些组合的问题，请参阅"已知问题"节。

# 已知问题

# 更新历史记录

**初始版本：** 2019 年 4 月

**更新:**2022 年 3 月

添加了 DLCPackagePC 文件夹，用于演示如何在电脑上创建 DLC。

修复了许可证丢失时的崩溃问题。

# 隐私声明

在编译和运行示例时，示例可执行文件的文件名将发送给
Microsoft，以帮助跟踪示例使用情况。若要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。
