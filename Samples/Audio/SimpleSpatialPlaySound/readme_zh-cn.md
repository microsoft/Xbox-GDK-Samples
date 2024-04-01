# 简单空间声音示例

*此示例可用于 Microsoft 游戏开发工具包（2020 年 6 月）*

# 说明

此示例演示了如何利用 Windows Sonic 技术，使用 ISpatialAudioClient 播放具有高度通道的静态音频。 可通过两个文件选项进行播放，并使用键盘控件开始/停止、暂停/播放和选择文件。

![](./media/image1.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Project Scarlett，请将可用解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* *GDK 文档中的*__运行示例__。&nbsp;

# 使用示例

| 操作 | 控制器 |
|---|---|
| 启动/停止 | A button |
| 循环下一个文件 | B button |
| 退出 | View button |

# 实现说明

此示例演示如何使用 ISpatialAudioClient 使用空间技术播放静态床声音。 初始化并启动 ISpatialAudioClient 后，它将使用回调来请求缓冲区帧。

# 更新历史记录

2019 年 3 月初始版本


