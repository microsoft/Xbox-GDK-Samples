![](./media/image1.png)

# TextureCompression 示例

*此示例可用于 Microsoft 游戏开发工具包（2021 年 6 月）*

# 说明

此示例演示了如何使用 Scarlett 平台提供的新硬件解压缩单元来处理纹理数据。

# 生成示例

如果使用 Project Scarlett，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

此示例不支持 Xbox One。

*有关更多信息，请参阅*&nbsp;__运行示例__（位于 *GDK&nbsp;文档）中。*

# 使用示例

![徽标特写 描述自动生成](./media/image3.png)

该示例附带了几个示例纹理，用于说明不同的 BCn 编码可以使用哪种类型的压缩比率进行存档。 可以使用控制器浏览纹理和 mip 级别：

| 操作 | 游戏板 |
|---|---|
| 下一个纹理 | 向上方向键 |
| 上一个纹理 | 向下方向键 |
| 提高 mip 级别 | 向右方向键 |
| 降低 mip 级别 | 向左方向键 |
| 退出 | &ldquo;视图&rdquo;按钮 |

# 实现说明

硬件解压缩单元使用 Direct Storage API 与 NVMe 控制器建立管道连接。 通过对请求启用解压缩，从 NVMe 驱动器流式传输的数据会先通过管道输送至解压缩单元，再写入内存。 无需中间缓冲区或在程序中同步即可实现此目的。 有关详细信息，请参阅 GDK 文档中的 Direct Storage 概述。

示例中存在的 .xbtc 纹理已使用 GDK 中的新 XBTC 压缩工具脱机压缩。 此工具可读取 DDS 文件，并使用 Deflate（即 Zlib）算法和专为 BCn 编码纹理数据设计的专有 BCPack 算法进行进一步压缩。 有关详细信息，请参阅 GDK 文档中的 Xbox 纹理压缩器概述。

# 已知问题

目前不支持 xbtc 文件可能输出的 PRT 磁贴格式（请查看 XBTC.exe 工具中的 --tilemode 选项）。

由于 XG 库中的 bug，示例 xbtc 文件已使用标准磁贴模式 (\_S) 进行压缩。 该示例的更高版本将启用对本机 (\_D) 磁贴模式的支持（请查看 XBTC.exe 工具中的 --tilemode）。

所有 XBTC 压缩目前都是非破坏性的。 如 XBTC 路线图文档中所述，我们将在 GDK 的未来版本中引入以质量换大小的选项。

# 更新历史记录

- 2020 年 2 月：预览版

- 2021 年 6 月：更新了示例资产以应对 XBTC 格式的更改。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


