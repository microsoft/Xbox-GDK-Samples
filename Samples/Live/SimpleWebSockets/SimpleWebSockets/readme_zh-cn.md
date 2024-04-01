![](./media/image1.png)

# SimpleWebSockets 示例

*此示例与 Microsoft GDK（桌面）和 GDKX （Xbox）（2022 年 3 月）兼容*

# 说明

此示例演示如何使用 LibHttpClient 通过 Web 套接字连接、发送、接收来自主机的消息。

# 生成示例

此示例不应要求对生成进行任何特定更改，如果使用 XDKS.1 沙盒，则应在不进行任何修改的情况下运行。

*有关详细信息，请参阅* *GDK 文档*中的__运行示例__。&nbsp;

# 使用示例

运行示例时，可以打开与服务的 WebSocket 连接，该服务将重复发送的消息。

首先单击&ldquo;连接&rdquo;按钮以建立 WebSocket 连接。 默认情况下，示例连接 `wss://echo.websocket.org` 的打开回升服务器。连接后，选择&ldquo;发送消息&rdquo;将打开虚拟键盘，以便你可以向终结点发送自定义消息。 选择&ldquo;发送二进制消息&rdquo;将向终结点发送包含二进制有效负载的测试消息。

## 主屏幕

![文本说明已自动生成](./media/image3.png)

# 更新历史记录

2021 年 4 月 -- 初始版本

2022 年 6 月 -- 针对 2022 年 3 月 GDK（及更高版本）兼容性进行了更新

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


