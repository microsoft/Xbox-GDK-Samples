![](./media/image1.png)

# LampArray 示例

*此示例与 Microsoft 游戏开发工具包（2023 年 3 月 QFE1）映像兼容*

# 说明

此示例演示如何使用 LampArray API 在 RGB 设备（如键盘和鼠标）中操作灯。

> **请注意：**自 2023 年 3 月 QFE1 版本起，GDK LampArray API 仅支持主机上的以下设备。 将来的恢复版本会添加对其他设备的支持。
> - 适用于 Xbox One 的 Razer Turret（键盘和鼠标）
> - Razer BlackWidow Tournament Edition Chroma V2

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 Gaming.Xbox.XboxOne.x64。

| | |
|---|---|
| 如果使用 Xbox Series X | S 开发工具包，请将活动解决方案平台设置为 Gaming.Xbox.Scarlett.x64。 |

*有关详细信息，请参阅 *GDK 文档中的&ldquo;&nbsp;__运行示例__*&rdquo;。*

# 使用示例

确保已连接兼容的设备。  使用键盘箭头键或游戏手柄的方向键在示例效果之间移动。

按 Esc 键或&ldquo;视图&rdquo;按钮退出。

# 实现说明

在 `LightingEffects.cpp` 文件中找到效果实现。  回调和其他功能位于 `Lighting.cpp` 文件中。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


