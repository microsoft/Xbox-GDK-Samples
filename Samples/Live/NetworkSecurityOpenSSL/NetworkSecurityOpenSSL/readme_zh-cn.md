![](./media/image1.png)

# 网络安全 OpenSSL 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# **注意：这些示例仅作为引用参考。 我们现在或将来均不保证它们足以实现你的网络安全目标。 请务必独立评估你的使用。**

# 说明

此示例演示使用 OpenSSL 的 DTLS 实现帮助保护 UDP 流量的最佳实践。

![自动生成图形用户界面、文本、网站描述](./media/image3.png)

# 生成示例

此示例支持 Xbox One、Xbox Series 主机和 GDK 桌面。 在下拉列表中选择要生成的配置。

*有关详细信息，请参阅*__运行示例__，详见*GDK 文档。*

此示例不附带预编译版本的 OpenSSL。 将需要从以下源获得最新版本，并将其添加到项目中：

对于个人电脑，你可以从以下网站下载最新的二进制文件：

- <https://www.openssl.org/>

- <https://github.com/openssl/openssl>

另一种方法是使用程序包管理器，如 [vcpkg](aka.ms/vcpkg) 或 nuget。

对于 Xbox 主机，需要从 github 获取数据源，并使用以下配置构建一个自定义版本：

```perl
perl&nbsp;Configure&nbsp;VC-WIN64A-masm&nbsp;no-ui-console&nbsp;no-dso&nbsp;no-stdio -D"WINAPI_FAMILY=WINAPI_FAMILY_GAMES"&nbsp;-D"_WIN32_WINNT=0x0A00" -DOPENSSL_SYS_WIN_CORE
```


# 运行示例

该示例可以作为客户端或主机运行，并使用预共享密钥或证书。 该示例是为了在客户端和主机之间一次只演示一个连接而建立的。 若要以主机身份启动，请按&ldquo;接受连接&rdquo;按钮。 使用预共享密钥或证书时，该示例将打印出证书的密钥或指纹。 单击&ldquo;接受&rdquo;后将看到一个窗口弹出，询问你期望连接的客户端的 IP 地址。 在主机上，这需要允许来自客户端的数据包通过控制台的防火墙。

![Graphical user interface Description automatically generated](./media/image4.png)

若要连接到主机，请单击&ldquo;打开连接&rdquo;按钮。 第一个弹出的窗口将是主机的 IP 地址和端口。 第二个将是针对主机正在使用的指纹或预共享密钥。

一旦连接完成，按&ldquo;发送消息&rdquo;将允许你输入一个字符串，该字符串将在 2 个示例实例之间发送。 如果流量是在 Wireshark 等工具中查看的，则该流量将加密且不可读取。

# 实现说明

该示例使用 OpenSSL 的内存 BIO，而不是 DGRAM BIO。 这样做是为了允许对多个连接重复使用套接字（未在本示例中演示）。 若要利用这一点，数据必须在套接字和 BIO 之间移动，而不是像 DGRAM BIO 那样自动完成。

基础 UDP 套接字使用重叠 I/O 和 Windows 线程池来最大程度地减少操作系统接收到数据和数据递送到游戏之间的延迟。

最后，在生产代码中，应使用密钥服务或大厅服务（如 MPSD）来帮助在游戏实例之间安全地共享密钥材料。

# 已知问题

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。

# 更新历史记录

**初始版本**：2021 年 8 月

2022 年 6 月 -- 针对 2022 年 3 月 GDK（及更高版本）兼容性进行了更新


