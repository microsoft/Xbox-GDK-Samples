# xbgamepad

*此工具与 Microsoft 游戏开发工具包预览版（2020 年 2 月）兼容*

# 说明

这是最小的 win32 控制台应用程序，从本地连接的 XINPUT 设备（例如 Xbox One
游戏手柄）获取输入，并使用 XDK 和 GDK 附带的
XTF（Xbox工具框架）库将这些输入发送到 Xbox One 或 Xbox Series X devkit。

# 运行

它可以在未安装 GDK 的计算机上运行，​​因为它将使用 xtfdlls
文件夹中的dll（如有）。此工具需要 Windows 8 或更高版本。

# 用法

*xbgamepad /x:\<devkit ipv4 地址 \> \[/r:\<更新率 hz - 默认值为 30\>\]*

需要通过 TCP 端口 4211 和 4212 访问 devkit。

# 构建必备组件

-   Visual Studio 2017

-   Windows 10 SDK

-   最近的 XTF 标头和库 GDK 安装（通过将环境变量 GameDK
    更改为包含和链接器输入中的 XDK 版本，可以将项目修改为使用 XDK）

# 分发

若要在未安装 GDK 的计算机上运行，需要从现有 GDK 安装复 xbtp.dll 和
xtfinput.dll 文件。 它们位于 %GameDK%\\bin 中。可以在未安装 GDK
的计算机上与 xbgamepad.exe 并行放置。
