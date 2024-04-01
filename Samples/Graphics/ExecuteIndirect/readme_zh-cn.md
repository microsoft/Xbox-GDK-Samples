![](./media/image1.png)

# ExecuteIndirect 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

*此示例演示如何使用 DirectX 12 的 ExecuteIndirect API 异步生成渲染命令。*

该示例创建了大量网格实例，这些实例随机分布在相机前。 在直接模式下，使用单独的绘制调用绘制每个网格实例。 在间接模式下，使用单个 ExecuteIndirect 调用绘制整个&ldquo;场景&rdquo;。

该示例可以选择在任一模式下执行 frustum 剔除。 在直接模式下，在 CPU 上一次剔除一个实例。 在间接模式下，使用 GPU 计算并行剔除实例。 ExecuteIndirect 调用只看到那些通过剔除的实例。 间接命令缓冲区中不存在其他实例。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* *GDK 文档*中的__运行示例__。&nbsp;

# 使用示例

此示例使用以下控件。

| 操作 | 游戏板 |
|---|---|
| 切换直接/间接绘制 | A 按钮 |
| 打开/关闭剔除 | B 按钮 |
| 退出 | &ldquo;视图&rdquo;按钮 |

![](./media/image3.png)

# 已知问题

无。

# 更新历史记录

XDK 2015 年 8 月初始版本

Microsoft GDK 2020 年 4 月更新

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


