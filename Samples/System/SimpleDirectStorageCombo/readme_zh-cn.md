![](./media/image1.png)

# SimpleDirectStorageCombo 示例

*此示例可用于 Microsoft 游戏开发工具包（2020 年 6 月）*

# 说明

此示例演示了在主机和桌面上使用 DirectStorage 的几种不同方法。

- SimpleLoad -- 用于初始化 DirectStorage、打开文件、将请求排入队列并等待完成的最小界面。

- StatusBatch -- 演示如何使用状态数组创建一批请求以进行通知。

- StatusFence -- 演示如何使用 ID3DFence 创建一批请求以进行通知。

- MultipleQueues -- 演示如何使用不同的优先级创建多个队列。

- Cancellation -- 演示如何取消挂起的请求。

- RecommendedPattern -- 演示使用 DirectStorage 实现最高性能的建议模式。

- Xbox 硬件解压缩 -- 演示如何使用硬件
   | | |
   |---|---|
   |在 Xbox Series X|S 主机上运行时的 zlib 解压缩。|

- Xbox 内存中硬件解压缩 -- 演示如何使用
   解压缩内存中已有的数据。
   | | |
   |---|---|
   |Xbox Series X|S 主机上可用的硬件 zlib 解压缩|

- Xbox 软件解压缩 -- 演示如何在 Xbox One 系列主机上运行时使用软件 zlib 解压缩。

- 桌面 CPU 解压缩 -- 演示如何将游戏提供的 CPU 解压缩编解码器支持与桌面上的 DirectStorage 配合使用。

# 生成示例

此示例支持以下平台

- Gaming.Desktop.x64

   - 使用电脑 API 集上的 DirectStorage。

- Gaming.Scarlett.xbox.x64

   - 使用 Xbox 上提供的 Xbox DirectStorage 实现
      | | |
      |---|---|
      |Series X|S 主机|

- Gaming.XboxOne.xbox.x64

   - 使用提供的软件仿真层，该层提供 Xbox DirectStorage 实现的功能，但在内部使用 Win32 API 集。

*有关详细信息，请参阅* *GDK 文档中的*__运行示例__。&nbsp;

# 使用示例

该示例将自动创建一个数据文件，然后执行所述的每个子部分。

# 实现说明

所有实施都包含在 SampleImplementations 文件夹中。 其中详细介绍了每个步骤的详细信息。

若要了解如何使用 BCPack 压缩的示例，请参阅 TextureCompression 示例。

zlib 库（版本 1.2.11）受此许可证约束：<http://zlib.net/zlib_license.html>

# 更新历史记录

初始版本 2022 年 2 月

已于 2022 年 10 月更新，添加了桌面 CPU 解压缩

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


