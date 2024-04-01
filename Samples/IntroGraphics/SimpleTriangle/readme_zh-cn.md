# 简单的三角形示例

*此示例可用于 Microsoft 游戏开发工具包（2020 年 6 月）*

# 说明

此示例演示了如何创建静态 Direct3D 12 顶点缓冲区以在屏幕上渲染三角形。

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* *GDK 文档*中的__运行示例__。&nbsp;

# 使用示例

该示例除了退出之外，没有其他控制。

# 实现说明

此示例的主要目的是使读者熟悉 ATG 示例模板结构，并简单演示如何使用 Direct3D 12 API。

> **CreateDeviceDependentResources**：在这里将加载编译顶点
> 和像素着色器 Blob 以及创建各种 Direct3D 渲染
> 资源。 *着色器由 Visual Studio 编译。*
>
> **Render**：在这里将渲染三角形并将其呈现到
> 屏幕。

有关设备创建和呈现处理的详细信息，请参阅 [DeviceResources](https://github.com/Microsoft/DirectXTK12/wiki/DeviceResources)。

有关使用循环计时器的详细信息，请参阅 [StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


