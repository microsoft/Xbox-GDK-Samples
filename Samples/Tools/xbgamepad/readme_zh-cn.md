# xbgamepad

*此示例可用于 Microsoft 游戏开发工具包（2022 年 3 月）*

# 说明

这是一款最小 win32 主机应用，能够从本地连接的 XINPUT 设备（例如 Xbox 游戏手柄） 提取输入并将这些输入发送到
XDK 和 GDK 附带的库。
| | |
|---|---|
|使用 XTF（Xbox 工具框架）的 Xbox One 或 Xbox Series X|

# 正在运行

它可以在没有安装 GDK 的计算机上运行，因为它将使用 xtfdlls 文件夹中的 dll（如果存在）。 这款工具需要 Windows 8 或更高版本。

# 用法

```
xbgamepad /x:<devkit ipv4 address> [/r:<update rate in hz - default is 30>]
```


需要通过 TCP 端口 4211 和 4212 访问开发工具包。

# 版本先决条件

- Visual Studio 2019 (16.11) 或 Visual Studio 2022

- Windows 10 SDK

- XTF 标头和库的最新 GDK 安装（可通过更改为包含和链接器输入中的环境变量 GameDK 来修改项目使用 XDK）

# 分发

若要在未安装 GDK 的计算机上运行，需要从现有 GDK 安装复制 xbtp.dll 和 xtfinput.dll 文件。 这两个文件位于 `%GameDK%\bin` 中。可以将它们与 xbgamepad.exe 并行放置在计算机，而无需安装 GDK。


