# 简单 MSAA

*此示例兼容于 Microsoft 游戏开发工具包（2020 年 6 月）*

![](./media/image1.png)

# 说明

此示例使用 DirectX 12 为 3D 场景实现了 MSAA 渲染目标和深度/模板缓冲区。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* *GDK 文档*中的__运行示例__。&nbsp;

# 使用示例

| 操作 | 游戏板 |
|---|---|
| 切换 MSAA 与单一样本 | A button |
| 退出 | &ldquo;视图&rdquo;按钮 |

# 实现说明

UI 是在不使用 MSAA 的情况下绘制的，它使用显式解析，而不是依赖于 MSAA 交换链的隐式解析。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


