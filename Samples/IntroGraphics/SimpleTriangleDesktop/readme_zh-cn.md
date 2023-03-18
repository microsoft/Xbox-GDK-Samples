# 简单三角形示例（电脑）

此示例与 Microsoft 游戏开发工具包（2019 年 11 月）兼容

# 说明

本示例演示如何创建静态 Direct3D 12 顶点缓冲区以在屏幕上呈现三角形。

![](./media/image1.png)

# 使用示例

本示例只使用退出控件，不涉及其他控件。

本示例将在任何配备有支持 DirectX 12 的视频卡的 Windows 10
系统上运行。在"调试"配置中，如果未找到支持 DirectX 12 的视频卡，则将使用
WARP12（如果可用）（需要图形工具这一可选的 Windows 组件）。

# 实现说明

本示例的主要目的是使读者熟悉 ATG 样本模板结构，并提供使用 Direct3D 12
API 的简单演示。

> CreateDeviceDependentResources：用于加载已编译的顶点和像素着色器
> blob，并创建各种 Direct3D 呈现资源。着色器是由 Visual Studio 编译的。
>
> Render： 用于呈现三角形并显示到屏幕。

有关创建设备和处理演示文稿的详细信息，请参阅《[DeviceResources](https://github.com/Microsoft/DirectXTK12/wiki/DeviceResources)》。

有关如何使用循环计时器的详细信息，请参阅《[StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)》。
