  ![](./media/image1.png)

#   SimpleHttp 示例

*此示例与 Microsoft GDK（桌面）和 GDKX (Xbox)（2021 年 4 月）兼容*

# 说明

此示例演示如何使用 XCurl 发出 HTTP
请求，包括将用户令牌和签名添加到标头以通过身份验证调用 Xbox Live。

# 生成示例

此示例不应要求对生成进行任何特定更改，如果使用 XDKS.1
沙盒，则应在不进行任何修改的情况下运行。

*有关详细信息，请参阅 GDK 文档中的*"运行示例"*。*

# 使用示例

运行示例时，可以通过 HTTPS 将请求发送到需要使用 XSTS
令牌进行身份验证的服务。

对于 HTTPS 调用，可以选择调用标准 Xbox Live 终结点，以使用 XBL 服务 HTTP
请求按钮获取有关当前用户 (Profile Service) 的信息。
这会连接到服务，正确添加 XSTS 令牌作为授权标头，并添加 Signature 标头。

若要模拟对自定义游戏服务的调用，可以使用游戏服务 HTTP
请求，该请求还会追加游戏服务所需的 XSTS 令牌身份验证。
默认情况下，这会调用正在运行的示例版本的游戏服务示例，并将使用用户的 X
令牌中用于对服务进行身份验证的所有声明进行回复。 其他服务功能（包括 b2b
商务
URI）也可以与此示例一起使用，方法是使用代码中注释掉的其他选项替代按钮的目标
URL。
有关配置自己的自定义游戏服务的详细信息，请参阅游戏服务示例和配置指南。

## 主屏幕

![Graphical user interface, text, website Description automatically generated](./media/image3.png)

# 实现说明

XCurl 用法全部可在 HttpManager.h/.cpp 中找到。
在这里，你将找到以下内容的演示：

-   等待网络可用性

-   从 Web 服务器创建 HTTPS"GET"请求

-   进行常规 HTTP 查询

请参阅 XCurl 文档，了解详细的 API 说明和用法。

# 更新历史记录

2021 年 4 月初始版本

# 隐私声明

在编译和运行示例时，将向 Microsoft
发送示例可执行文件的文件名以帮助跟踪示例使用情况。若要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。
