# 简单播放 3D 声音示例

*此示例兼容于 Microsoft 游戏开发工具包(2020 年 6 月)*

# 说明

此示例演示如何使用 XAudio2 和 X3DAudio 在 Xbox One 上播放位置音频。 侦听器是静态的(由白色三角形表示)，并且发射器(由黑色三角形表示)可以在 3D 空间中移动，尽管视图是从上到下的。 发射器周围的圆表示衰减曲线的结束，其中的线条表示发射器锥形的内外边界。 有关这些术语的详细信息，请参阅 [常见音频概念](https://msdn.microsoft.com/en-us/library/windows/desktop/ee415692%28v=vs.85%29.aspx)

![](./media/image1.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Project Scarlett，请将可用解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* *GDK 文档中的*__运行示例__。&nbsp;

# 使用示例

| 操作 | 游戏板 |
|---|---|
| 移动发射器 | 左控制杆 |
| 旋转发射器 | 右控制杆 |
| 调整发射器高度 | 左/右肩按钮 |
| 重置发射器位置 | 左/右控制杆 |
| 更改混响类型 | 向上/向下方向键 |

# 实现说明

此示例演示如何将 XAudio2 与 X3DAudio 配合使用来播放位置声音。 初始化 XAudio2 后，将添加用于混响的子混合通道，并无限循环播放 wav 文件。 每次更新都会使用发射器的当前位置计算 X3DAudio DSP 设置，以确定位置和方向。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


