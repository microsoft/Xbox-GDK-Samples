  ![](./media/image1.png)

#   MP4Encoder 示例

*此示例兼容于 Microsoft 游戏开发工具包（2020 年 8 月）*

# 

# 说明

此示例演示了如何使用媒体基础 API 将游戏的后台缓冲区编码为使用 H264
编解码器的 MP4 视频文件。

![A picture containing chart Description automatically generated](./media/image3.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

*有关详细信息，请参阅 GDK 文档中的*"运行示例"*。*

# 使用示例

此示例使用以下控件。

| 操作                                         |  游戏手柄              |
|----------------------------------------------|-----------------------|
| 停止编码                                     |  A                     |
| 退出                                         |  视图按钮              |

# 使用说明

此示例使用计算着色器将其后台缓冲区转换为可用于视频编码的 NV12 纹理。NV12
纹理包含两个部分：具有亮度数据 (Y) 的完全分辨率平面 0 和具有色度数据
(UV) 的半分辨率平面 1。计算着色器执行简单的双线性缩小采样来计算色度。

媒体基础 API 可用于利用 Xbox 硬件编码器将 NV12 纹理编码为带有 H264
视频流的 MP4 视频文件。该 API
将两个文件写入开发工具包系统的暂存驱动器，一个文件包含原始 H264
视频流，另一个 MP4 文件包含 H264
视频流。可以使用以下方法找到这些文件："xbdir
xd:\\\\MP4EncoderSampleOutput.mp4"

游戏可以使用 Xbox 硬件编码器以最大 1080p @ 30Hz
进行编码，因为该编码器也以系统级别用于游戏 DVR。尽管它只能以 30Hz
进行编码，但此示例是以 60Hz
进行渲染。因此，此示例为编码创建了一个单独的线程，以避免停止渲染。

若要轻松查看 MP4 文件，可以使用 xbcp 复制文件，也可以使用 Xbox
管理器工具中的文件资源管理器功能。

![Graphical user interface, application Description automatically generated](./media/image4.png)

![Graphical user interface Description automatically generated](./media/image5.png)

![Graphical user interface, text, application Description automatically generated](./media/image6.png)

# 已知问题

无

# 隐私声明

在编译和运行示例时，将向 Microsoft
发送示例可执行文件的文件名以帮助跟踪示例使用情况。若要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。
