# FilePerfTestCombo 示例

*此示例可用于 Microsoft 游戏开发工具包（2022 年 3 月）*

# 说明

本示例适用于文档中的 [在 Xbox One 上最大限度地提高文件性能](https://developer.microsoft.com/games/xbox/docs/gdk/maximizing_file_performance_win32_xbox_one)和[最大化 Project Scarlett 性能](https://developer.microsoft.com/games/xbox/docs/gdk/maximizing_file_performance_win32_scarlett)。 文档中显示的所有数字都是从此示例生成。 使用此示例，可以检查基准方法，并进行更改以模拟其他配置。

# 使用示例

可以在 FilePerfTestCombo.cpp 和 Sample::ParseCommandLine 函数中找到所有命令行选项。

第一步是生成测试配置所需的数据文件。

1) 请确保数据文件的驱动器上至少有 30 G 的可用空间。

2) 选择 Gaming.Desktop.x64 平台的发布配置。

3) 设置命令行。

```
performfullsetup createpackedfile numiterations 5
```



4) 生成并运行。

a.  根据硬件情况，这可能需要几分钟时间。

如果需要的数据文件可用于使用 zlib 的解压缩测试配置。 将命令行更改为

```
performzipsetup ratio \[compression ratio\] createpackedfile numiterations
```


\[压缩比率\] 可以是介于 0 和 100 之间的任意数字。 默认值为 50。 这表示压缩比率为 50%，压缩的文件将是原始文件大小的 50%。

> 注意：根据硬件，这可能需要超过 1 小时和多于 200 G 的磁盘空间。

各种测试配置主要通过命令行进行控制，这些选项不区分大小写。

- 测试要运行的类型 -- 这些都可以在同一命令行上用于特定的多个测试配置。

   - DoSync

      - 使用 Win32 和同步操作。

   - DoASync

      - 使用 Win32 和重叠操作。

   - DoSyncDStorage

      - 以同步方式使用 DirectStorage。

   - DoASyncDStorage

      - 以异步方式使用 DirectStorage。

   - DoZip

      - 使用 DirectStorage 和解压缩硬件。

      - 需要使用解压缩数据。

- NumIterations \[计数\]

   - 要为每个测试配置执行的迭代数。

- 加载 \[第一个顺序\] \[最后一个顺序\]

   - 用于测试配置的第一个和最后一个顺序读取集。

   - 按顺序排列的有效选项。

      - True_Sequential

      - 随机

      - Random_Sequential

      - 向后

      - 冗余

- 大小 \[第一个大小\] \[最后一个大小\]

   - 用于测试配置的第一个和最后一个读取大小。

   - 按顺序排列的有效选项。

      - size_8k

      - size_12k

      - size_16k

      - size_32k

      - size_64k

      - size_128k

      - size_192k

      - size_256k

      - size_512k

      - size_1024k

      - size_2048k

      - size_4096k

      - size_8192k

      - size_16384k

      - size_32768k

- 深度 \[第一个深度\] \[最后深度\]

   - 用于 Win32 异步测试配置的第一个和最后一个队列深度。

   - 按顺序排列的有效选项。

      - depth_1

      - depth_2

      - depth_4

      - depth_8

      - depth_12

      - depth_16

      - depth_24

      - depth_32

      - depth_64

      - depth_128

      - depth_256

      - depth_512

      - depth_768

      - depth_1024

      - depth_2048

      - depth_4096

- UsePackedFile

   - 是使用单个文件还是多个较小的文件。

- DoRealtime

   - DirectStorage 测试的修饰符，用于使用实时优先级队列。

部分示例命令行

```
DoASync NumIterations 5 load True_Sequential Random Depth Depth_4 Depth_64 Size Size_8k Size_32768k
```


使用 4 到 64 之间的队列深度以及 8 KB 到 32 MB 之间的读取大小，对真实顺序位置和随机位置的所有组合执行 Win32 异步测试。

```
DoASyncDStorage NumIterations 5 load True_Sequential Random Size Size_8k Size_32768k
```


执行 DirectStorage 异步测试，其中包含真实顺序位置和随机位置的所有组合，读取大小介于 8 KB 和 32MB 之间。

```
DoASync DoASyncDStorage NumIterations 5 load True_Sequential Random Depth Depth_4 Depth_64 Size Size_8k Size_32768k
```


使用 4 到 64 之间的队列深度以及 8 KB 到 32 MB 之间的读取大小，对真实顺序位置和随机位置的所有组合执行 Win32 和 DirectStorage 异步测试。

> 注意：队列深度仅应用于 Win32 测试，因为这是唯一使用此选项的测试类型。

# 生成示例

如果使用 Xbox One 开发工具包，请将可用解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Scarlett 开发工具包，请将可用解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

如果使用台式电脑，请将可用解决方案平台设置为 `Gaming.Desktop.x64`。

*有关详细信息，请参阅* *GDK 文档中的*__运行示例__。&nbsp;

# 更新历史记录

2020 年 11 月初始发布

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


