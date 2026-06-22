![](./media/image1.png)

# Xbox 代理服务器示例

## 说明

此示例演示从开发工具包代理接收检测信号请求的 Web 服务。 该示例旨在简化其功能，并显示所选控制台的检测信号内容。 可以使用提供的 UI 将作业请求发送到目标开发工具包，并在处理作业时查看检测信号请求更新。

多个开发工具包可以以示例服务器为目标，允许一次向每个开发工具包颁发一个作业。

有关开发工具包代理功能的详细信息，请参阅 [开发工具包代理概述](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/devkitagent-overview)。


## 生成和配置示例

若要生成示例，请在 Visual Studio 2022 中加载解决方案，然后修改 XboxAgentServerSample\Properties\launchSettings.json 和 XboxAgentServerSample\appsettings.json，如下所示：

- 将 `[server computer name]` 占位符替换为运行示例的电脑的本地网络名称。

- 在 HttpsInlineCertStore 中，根据 SSL 证书配置 URL 和证书设置。

> [！注意]
> 支持自签名证书，在此示例中，我们使用 CurrentUser/My 存储来存储证书。

## 所需证书

运行示例需要以下证书：

- 在服务器上安装有公钥和私钥的 SSL 证书，以使用开发工具包启用 HTTPS 流量。  这可以是自签名证书，但证书的名称和使用者必须是计算机的网络名称，否则它将不受 SSL 流量信任。
- 要安装在开发工具包上的 SSL 证书公钥证书，以启用对服务器的 HTTPS 调用。 复制到 xs:\Microsoft\Cert
- 在服务器上安装有公钥和私钥的信赖方证书，以便从信赖方的开发工具包解密 XSTS 令牌。

> [！注意]
> 如果服务器使用的是受信任的根 SSL 证书，则无需将 SSL 公共证书安装到开发工具包。

在服务器上，SSL 和信赖方公钥证书和私钥证书都应安装到 `Certificates - Current User\Personal` 证书存储中。 这允许服务器在 Visual Studio 2022 和 Kestrel 中运行时访问它们。

如果尚未在合作伙伴中心中配置信赖方，请参阅 [本文](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/live-web-services.html#corepa)。

### 创建自签名 SSL 证书

在提升的 PowerShell 命令提示符中，运行以下命令。 将占位符文本替换为托管示例的计算机名称。

> ``` New-SelfSignedCertificate -CertStoreLocation Cert:\CurrentUser\My -DnsName "[server computer name]" -FriendlyName "[server computer name]" -NotAfter (Get-Date).AddYears(10) ```

<a id="configureFirewall"></a>
### 在电脑上配置防火墙以允许发往示例的流量

使用管理员访问权限打开 Powershell 命令提示符并运行以下命令。 将占位符文本替换为所使用的 SSL 证书的指纹。

> ```netsh advfirewall firewall add rule name="XboxAgentServerSample" dir=in protocol=tcp localport=8733 action=allow```

> ```netsh http add urlacl url=https://+:8733/ user=Everyone```

> ```netsh http add sslcert ipport=0.0.0.0:8733 certhash=[ssl certificate thumbprint] ```

<a id="configureAgent"></a>
### 将开发工具包配置为与示例通信

以下命令与 VS 游戏命令提示符窗口一起使用，使开发工具包以检测信号示例为目标。 将占位符文本替换为托管示例的计算机名称，以及开发工具包应使用该示例授权的信赖方名称（例如：rp://relyingparty.contoso.com/）。

> ``` xbconfig DevkitAgentServiceUri=https://[server computer name]:8733/api ```
> ``` xbconfig DevkitAgentRelyingParty=[your relying party name] ```

若要在开发工具包上安装 SSL 公钥证书，需要在 VS 游戏命令提示符窗口中使用以下命令复制该证书：

> ``` xbcp [path to .cer file] xs:\Microsoft\Cert ```


## 使用示例

正确配置示例后，配置为向服务器发送检测信号的开发工具包将显示在 Xbox 开发工具包标头下。

单击每个开发工具包的名称，查看有关最新的 HeartbeatRequest 的详细信息，或使用右侧面板中的选项和按钮向开发工具包发出作业。

> [！注意]
> 从开发工具包发送检测信号时，页面不会自动刷新。  若要刷新，请单击左上角的刷新按钮。


向控制台发出作业时，这些是控制台命令作业，如[基于控制台的命令行工具中所述](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/consolecommandlinetools)。 一些简单示例包括 [wdapp 列表](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/wdapp)、[wdapp 启动](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/wdapp)和 [wdconfig sandb 权限](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/wdconfig)。

有关可以与开发工具包代理一起使用的详细信息和可能的操作，请参阅 [开发工具包代理概述](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/devkitagent-overview)。

## 故障排除

解决问题的最佳方法之一是将开发工具包代理的日志记录级别设置为详细 （xbconfig DevkitAgentDesiredLogLevel=verbose），并使用 xbWatson 查看来自开发工具包代理的流量，以获取 xsts 令牌、发送检测信号以及 SSL 身份验证流中可能出现的证书问题。

### 验证 Web 服务是否正在运行，并且可以从电脑接收 HTTPS 流量

确保服务器能够在 Visual Studio 2022 中编译和运行。 服务器运行后，打开 Web 浏览器，并使用电脑名称（而不是 localhost）转到示例主页：
> ``` https://[server computer name]:8733 ```

如果服务器的主页未显示，则可能是 Kestrel，防火墙配置不正确。 尝试从 [配置开发工具包代理运行配置命令以与示例通信](#configureFirewall)。

你还需要尝试使用同一网络上不同电脑的 Web 浏览器访问该示例。

### 验证从开发工具包发送到服务器的传出流量

在开发工具包上启用 Fiddler 跟踪，如 [Xbox 开发工具包上的 Fiddler](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/fiddler-setup-networking) 中所述。 在 Fiddler 运行时，检查从开发工具包发往服务器的检测信号流量。 如果 Fiddler 中没有显示运行示例的电脑 URL 的调用，请尝试在[配置开发工具包代理下再次运行命令以与示例通信](#configureAgent)。

### 验证开发工具包是否能够完成对示例的 HTTPS 调用

如果开发工具包调用示例服务器，但未能完成 SSL 握手，但在通过 Fiddler 运行开发工具包流量时确实有效，则可能出现以下问题：

- SSL 证书 `subject` 或 `issued to` 值与开发工具包用于发送检测信号的 URL 中电脑的主机名不匹配。
- 服务器使用的 SSL 证书与开发工具包上安装的公共证书不匹配，该证书在配置开发工具包代理以[与示例通信时进行配置](#configureAgent)

首先，通过打开 Web 浏览器来验证对服务器进行 HTTPS 调用时使用的证书，然后使用电脑的名称（而不是 localhost）转到示例的主页：
> ``` https://[server computer name]:8733 ```

当主页显示时，请检查浏览器地址栏或 `View site information` 上的锁图标。不同的 Web 浏览器具有不同的方法来获取此数据，请参阅如何查找你正在使用的浏览器。 如果能够拉取服务器提供的 SSL 证书信息，请检查名称和指纹。 确保指纹与要复制到开发工具包的 .cer 文件中的指纹匹配。 此外，请确保证书的&ldquo;颁发者&rdquo;值与开发工具包代理使用的 URL 中的主机名匹配。

示例：示例服务器的 URL https://mylocalPC:8733。  对于 https 流量，开发工具包将仅接受颁发给&ldquo;mylocalPC&rdquo;的证书。

### 验证服务器是否能够解密检测信号中发送的 XSTS 令牌

从开发工具包到导致 HTTP 403 错误的示例的检测信号指示服务器无法验证正在发送的 XSTS 令牌。

首先，在[运行开发工具包代理配置命令](#configureAgent)时验证是否提供了正确的信赖方名称。 还可在重新启动后查看来自控制台的 fiddler 调用，以验证在请求正文中使用信赖方名称对 XSTS 的调用是否返回了令牌。

其次，验证示例是否能够访问电脑上信赖方的私有证书密钥。 查看在 Visual Studio 中运行的服务器的调试输出，应会看到指示发生情况的日志或警告。 或者，可以将断点放在 ValidateAuthorizationHeaderWithCache API 中，并单步执行该断点以查看错误发生的位置。 如果示例确信在 XSTS 令牌标头中找不到与指纹匹配的证书，请确保以下内容：

- 你的电脑上安装了与 XSTS 令牌的指纹匹配的证书私钥
- 证书和私钥安装到 `Certificates - Current User\Personal` 证书存储。

## 实现说明

此示例设计为可移植，以 ASP.NET 编写，并使用跨平台 [Kestrel Web 服务器](https://learn.microsoft.com/en-us/aspnet/core/fundamentals/servers/kestrel?view=aspnetcore-7.0)。

## 已知问题

主页不会自动刷新，请使用&ldquo;刷新页面&rdquo;按钮查看最新的检测信号请求。

## 更新历史记录

| **日期** | **Version** | **Description** |
|---|---|---|
| 2023 年 10 月 18 日 | 1.0 | 初始版本 |

## 隐私声明

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。

