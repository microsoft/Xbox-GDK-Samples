# 简单三角形示例

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

# 说明

本示例演示如何创建静态 Direct3D 12 顶点缓冲区以在屏幕上呈现三角形。

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

# 构建示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

有关详细信息，请参阅 GDK 文档中的"运行示例"。

# 使用示例

本示例只使用退出控件，不涉及其他控件。

# 实现说明

本示例的主要目的是使读者熟悉 ATG 样本模板结构，并提供使用 Direct3D 12
API 的简单演示。

> CreateDeviceDependentResources：用于加载已编译的顶点和像素着色器
> blob，并创建各种 Direct3D 呈现资源。着色器是由 Visual Studio 编译的。
>
> Render： 用于呈现三角形并显示到屏幕。

有关创建设备和处理演示文稿的详细信息，请参阅《[DeviceResources](https://github.com/Microsoft/DirectXTK12/wiki/DeviceResources)》。

有关如何使用循环计时器的详细信息，请参阅《[StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)》。

# 隐私声明

在编译和运行示例时，示例可执行文件的文件名将发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。
