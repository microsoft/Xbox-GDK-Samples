# 简单动态资源示例

*此示例与 Microsoft 游戏开发工具包（2021 年 4 月）兼容*

# 说明

此示例演示如何在 HLSL 着色器模型 6.6 中使用 HLSL 动态资源。它在功能上与
SimpleTexture 相同，但使用 HLSL 中的 ResourceDescriptorHeap\[\] 和
SamplerDescriptorHeap\[\] 直接通过堆访问资源。

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Xbox Series X|S，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

由于使用 HLSL SM 6.6 功能，为电脑构建 (Gaming.Desktop.x64) 需要 [DirectX
Agility
SDK](https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/)。Direct
Agility SDK 以 NuGet 程序包的形式包含在示例中。它还利用
Microsoft.Windows.SDK.CPP NuGet 程序包获取 DXC.exe 编译器的最新 Windows
SDK (22000) 版本。开发人员还可以直接从
[Github](https://github.com/microsoft/DirectXShaderCompiler/releases)
使用最新的 DXC。

*有关详细信息，请参阅 GDK 文档中的*"运行示例"*。*

# 使用示例

该示例除了退出之外，没有其他控制。

# 实现说明

此示例几乎借用了 SimpleTexture 的所有代码。唯一区别在于资源的访问。

此示例从根签名中删除绑定的资源，将其替换为 HLSL 着色器代码中的
ResourceDescriptorHeap\[\] 和 SamplerDescriptorHeap\[\]
访问。这需要确保在 SetGraphicsRootSignature() 之前调用
SetDescriptorHeaps()，并将标志 CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED 和
SAMPLER_HEAP_DIRECTLY_INDEXED 添加到根签名。

有关 HLSL 6.6 动态资源的详细信息，请参阅 [HLSL SM 6.6
动态资源](https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html)。

有关动态资源的更高级用法，请参阅 Graphics\\VisibilityBuffer 示例。

# 隐私声明

在编译和运行示例时，将向 Microsoft
发送示例可执行文件的文件名以帮助跟踪示例使用情况。若要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。
