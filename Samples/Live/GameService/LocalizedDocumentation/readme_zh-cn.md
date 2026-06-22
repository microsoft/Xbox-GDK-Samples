  ![](./media/image1.png)

#   XSTS 2018 服务器样本

# 描述

要将自定义 Web 服务与支持 Xbox Live 的游戏集成，您需要使用 XSTS
令牌进行身份验证和用户身份识别。 使用 GetTokenAndSignatureAsync()
从客户端获取令牌非常简单，但是，对于某些开发人员来说，与 XSTS
令牌集成并对其进行操作可能具有挑战性。 此样本是一个功能正常的 Web
服务器，其示例源代码可用于解密和验证 XSTS
令牌，以及与所需服务进行通信以代表用户进行经身份验证的调用（后端到后端或
b2b）。

如果使用其他语言编写自己的令牌处理，则可将其用作教育示例，也可将其用作构建
Web 服务的基础框架。

# 构建示例

此样本是完备的样本，但依赖于 NuGet 程序包（Newtonsoft.Json、Jose.JWT
以及与 Azure Key Vault
连接所需的程序包），当您首次打开并构建解决方案时，应下载这些程序包。
要编译此样本，只需打开 Xsts2018.sln，然后编译即可。

您可启用以下功能，只需将其添加至项目的条件编译符号文本框（在"项目属性"-\>"构建"-\>"所有配置"下找到）。

-   LINUX - 在构建中支持 FWD
    标头，这些标头在流量来自反向代理并添加这些标头的 Linux 上常用

-   XSTS_LEGACY - 支持处理旧版 XSTS 令牌（带有信赖方证书的 Asymmetric
    Draft 7 JWE），而不是 XSTS 2017 格式和对称共享密钥信赖方

# 使用样本

有关如何配置 Azure
服务并部署与其一起运行的样本的说明，请参阅此样本随附的"XSTS 2018
样本快速入门指南"。

# 实施说明

此样本具有以下主要功能和设计：

-   通过原始源代码或使用公开提供的开源软件解决方案（Newtonsoft.Json、Jose.JWT、.NET
    Core 2.1）对所有令牌处理类和函数进行源代码访问

-   可用作创建 Web 服务的起始基础的样本

-   能够使用 C# ASP.NET Core 2.1 在多个服务器操作系统上运行

    -   通过 Windows Server 2016 验证

    -   通过 Linux (Ubuntu 16.04) 验证

-   旧版 XSTS 令牌（XDP 配置的 JWT Draft 7 Asymmetric）和新版 2017 XSTS
    令牌（UDC 配置的 JWT RFC 对称共享密钥）的令牌处理和验证

-   检索和管理 Xbox Live 服务器到服务器请求所需的服务令牌

-   检索和使用代表用户的 Xbox Live 服务器到服务器请求所需的委托身份验证
    XSTS 令牌

-   开发中心中信赖方、业务伙伴证书和 NSAL 设置的逐步配置说明。

# 已知的问题

-   无

# 更新历史记录

2018 年 10 月 26 日 -- v1.0

# 

# 隐私声明

有关 Microsoft 隐私政策的更多信息，请参阅 [Microsoft
隐私声明](https://privacy.microsoft.com/zh-cn/privacystatement/)。
