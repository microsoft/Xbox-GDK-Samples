![](./media/image1.png)

# HlslCompile 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

示例以多种不同的方式编译同一像素着色器，以演示适用于电脑端资源版本的不同选项。 着色器编译器正在积极开发中，我们会在功能更改时更新示例。

示例使用两种不同的编译器接口创建着色器：

- `Dxc.exe` - 新着色器编译器前端的命令行接口

- `DxCompiler_x[s].dll` 新着色器编译器前端的可调用接口

分别有两个 exe 和 dll 文件的副本，属于 Xbox One 和
`DxCompiler_xs.dll`。）着色器符号提供 PIX 的重要信息的方式与 C++ 符号为 Visual Studio 和其他工具提供上下文的方式相同。 着色器编译器接口支持多个符号存储选项：
| | | |
|---|---|---|
|属于 Xbox Series X|S。 （dll 的 Xbox Series X|S 副本为 |


- 嵌入在二进制文件中 - 此方法最简单，但在运行时内存使用方面通常成本太高。 新着色器编译器前端的嵌入符号已弃用。

- 去除为手动选择的文件名 - 例如，调用方可以选择该名称作为源文件名的可识别变体。

- 去除为自动选择的文件名 - 将根据已编译着色器的哈希选择名称。 建议使用此方法，因为 PIX 无需提示即可计算相同的着色器哈希。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* *GDK 文档*中的__运行示例__。&nbsp;

生成并运行解决方案时，Visual Studio 将

- 生成 MyHlslCompiler 项目
- 生成 HlslCompile 项目
   - 作为该生成的一部分，使用 MyHlslCompiler.exe 编译某些着色器
- 使用编译结果在控制台上运行 HlslCompile 可执行文件

# 使用示例

该示例是非交互式的。 下面屏幕图像中的每一行都包含一个三角形。 使用同一像素着色器的副本呈现每个三角形，每个副本都以不同的方式进行编译。 每个像素着色器二进制文件的大小以青色形式列出（屏幕截图中的数字可能已过期）。 每行文本的其余部分介绍了如何编译着色器以及如何存储符号。

若要验证是否已正确生成符号，建议对示例进行 PIX GPU 捕获，并尝试检索 PIX 中每个三角形的像素着色器的符号。 在某些情况下，PIX 将自动检索正确的符号，而在其他情况下，需要对用户的部件执行手动操作。

![](./media/image3.png)

若要在电脑上调试 MyHlslCompiler 项目，需要将 Path 环境变量指向 GDK 二进制文件，如下所示：

![MyHlslCompiler 设置](./media/MyHlslCompiler-settings.png)

# 已知问题

无。

# 更新历史记录

2019 年 4 月初始版本

Microsoft GDK 2019 年 11 月更新

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


