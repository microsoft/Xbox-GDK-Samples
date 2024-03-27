# 简单纹理示例

*此示例可用于 Microsoft 游戏开发工具包（2020 年 6 月）*

# 说明

此示例演示了如何使用 Direct3D 12 渲染简单的纹理化四边形。

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* *GDK 文档*中的__运行示例__。&nbsp;

# 使用示例

该示例除了退出之外，没有其他控制。

# 实现说明

此处的纹理使用一个简单的帮助程序进行加载，帮助程序使用 Windows 图像处理组件 (WIC)，旨在简化学习。 对于生产用途，应查看 DirectX 工具包的
[DDSTextureLoader](https://github.com/Microsoft/DirectXTK12/wiki/DDSTextureLoader)
和 [WICTextureLoader](https://github.com/Microsoft/DirectXTK12/wiki/WICTextureLoader)。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


