![](./media/image1.png)

# SimpleWinHttp 桌面示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

此示例演示如何使用 WinHTTP 发出 HTTP 请求，包括为经过身份验证的 Xbox Live 调用将用户令牌和签名添加到标头，以及通过 WebSocket 连接、发送和接收来自主机的消息。

# 生成示例

此示例不应要求对生成进行任何特定更改，如果使用 XDKS.1 沙盒，则应在不进行任何修改的情况下运行。

*有关详细信息，请参阅* *GDK 文档*中的__运行示例__。&nbsp;

# 使用示例

运行示例时，可以打开与将重复发送消息的服务的 WebSocket 连接，也可以通过 HTTPS 将请求发送到需要使用 XSTS 令牌进行身份验证的服务。

对于 WebSocket，请先单击&ldquo;连接&rdquo;按钮以建立 WebSocket 连接。 默认情况下，示例连接 `wss://echo.websocket.org` 的打开回升服务器。连接后，选择&ldquo;发送消息&rdquo;将打开虚拟键盘，以便你可以向终结点发送自定义消息。

对于 HTTPS 调用，可以选择调用标准 Xbox Live 终结点，以使用 XBL 服务 HTTP 请求按钮获取有关当前用户 (Profile Service) 的信息。 这会连接到服务，正确添加 XSTS 令牌作为授权标头，并添加 Signature 标头。

若要模拟对自定义游戏服务的调用，可以使用游戏服务 HTTP 请求，该请求还会追加游戏服务所需的 XSTS 令牌身份验证。 默认情况下，这会调用正在运行的示例版本的游戏服务示例，并将使用用户的 X 令牌中用于对服务进行身份验证的所有声明进行回复。 其他服务功能（包括 b2b 商务 URI）也可以与此示例一起使用，方法是使用代码中注释掉的其他选项替代按钮的目标 URL。 有关配置自己的自定义游戏服务的详细信息，请参阅游戏服务示例和配置指南。

## 主屏幕

![](./media/image3.png)

# 实现说明

WinHttp 用法全部可在 WinHttpManager.h/.cpp 中找到。 在这里，你将找到以下内容的演示：

- 等待网络可用性并设置 WinHTTP 会话

- 从 Web 服务器创建 HTTPS&ldquo;GET&rdquo;请求

- 升级与 WebSocket 的连接

- 向回显服务器发送消息

- 接收响应

- 干净地关闭 WebSocket

- 进行常规 HTTP 查询

有关详细的 API 说明和用法，请参阅 [WinHTTP 文档](https://docs.microsoft.com/en-us/windows/desktop/api/_http/) 。

# 更新历史记录

2020 年 1 月 - 初始版本

2022 年 6 月 -- 针对 2022 年 3 月 GDK（及更高版本）兼容性进行了更新

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


