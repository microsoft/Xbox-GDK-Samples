# RemoteConsoleView 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

此示例工具演示了如何使用 WPF 应用中的 VideoStreamingControl，以类似于 Xbox One Manager 的方式在电脑上远程显示主机内容。

运行示例时，输入主机名称（或 IP 地址），点击&ldquo;连接&rdquo;按钮，主机显示应呈现。

首次运行应用时，可能会收到防火墙提示：应用需要网络访问才能访问远程主机。

如果要在自己的 WPF 项目中使用此控件，请执行以下操作：

- 确保项目生成 64 位（该控件仅在 64 位版本中可用）

- 添加对`C:\Program Files (x86)\Microsoft GDK\bin\Microsoft.Xbox.Tools.RemoteVideo.dll`的引用（GDK 安装随附）

- 将此文件添加到项目：`C:\Program Files (x86)\Microsoft GDK\bin\xtfremotevideo.dll`并确保&ldquo;*复制到输出目录*&rdquo;设置为&ldquo;*复制（如果较新）*&rdquo;。 此文件必须位于 exe 旁边，该控件才能正常工作。

- 添加对该控件的 XAML 引用，有关详细信息，请参阅示例。

## VideoStreamingControl

该控件使用非常简单。 将*Source*属性设置为主机名称（或 IP 地址），它将在连接后立即启动（*自动播放* 默认为 True）。 它具有*Stop*方法，可以通过*Volume*和*IsMuted*属性控制卷。 如果发生错误，可以绑定到*Status*和*StatusMessage*属性。 请注意，此控件使用*HWndHost*进行呈现，这意味着无法在该控件顶部呈现其他 WPF 控件（由于空域问题）。

# 更新历史记录

初始版本 2020 年 3 月。


