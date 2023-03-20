# 简单 HDR 示例

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

# 说明

此示例将 UHD 电视切换到 HDR 模式，并呈现值高于 1.0f 的 HDR
场景，该场景将在 UHD
电视上显示为比白色更亮的颜色。此示例的目标是显示要使用的 API、应如何创建
HDR 交换链，以及在 UHD 电视上如何显示大于 1.0f 的不同值。

![A picture containing timeline Description automatically generated](./media/image1.png)

![Text Description automatically generated](./media/image2.png)

# 构建示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

有关详细信息，请参阅 GDK 文档中的"运行示例"。

# 使用示例

此示例使用以下控件。

| 操作                                           |  游戏手柄            |
|------------------------------------------------|---------------------|
| 切换显示 ST.2084 曲线                          |  A 按钮              |
| 切换显示仅纸白色块                             |  B 按钮              |
| 调整纸白色的亮度                               |  方向键              |
| 调整值                                         |  左/右控制杆         |
| 退出                                           |  "视图"按钮          |

# 实现说明

此示例使用 API 来确定附加的显示器是否支持 HDR。如果支持，则会显示切换到
HDR 模式。一个非常简单的 HDR 场景，其值高于 1.0f，将呈现给 FP16
后台缓冲区，并输出两个不同的交换链，一个用于 HDR，一个用于
SDR。即使使用者使用 HDR 显示器，GameDVR 和屏幕截图仍需要 SDR 信号。

此示例具有一个支持 HDR 和 SDR 交换链的
[DeviceResources](https://github.com/Microsoft/DirectXTK12/wiki/DeviceResources)
类的版本。

请参阅白皮书《[Xbox One 上的 HDR](http://aka.ms/hdr-on-xbox-one)》。

到目前为止，游戏输出和 SDR 信号均采用 Rec.709 色彩三原色和 Rec.709
伽玛曲线。UHD 显示器的一个新功能是更广泛的色域
(WCG)。要使用此功能，我们需要使用一个新的颜色空间，即 Rec.2020
色彩三原色。UHD 显示器的另一个新功能是高动态范围
(HDR)。若要使用此功能，我们需要使用不同的曲线，即 ST.2084
曲线。因此，若要输出 HDR 信号，则需要使用带 ST.2084 曲线的 Rec.2020
色彩三原色。

对于显示 SDR 信号，将应用简单的色调映射着色器，以便只需剪辑 HDR 场景中
1.0f 以上的所有值，并使用 Rec.709 色彩三原色输出 8
位值。有关其他色调映射操作符，请参阅适用于 DirectX 12 的 DirectX
工具包中的
[PostProcess](https://github.com/Microsoft/DirectXTK12/wiki/PostProcess)
类。

若要显示 HDR 信号，可使用着色器将 Rec.709 色彩三原色旋转到 Rec.2020
色彩三原色，然后应用 ST.2084 曲线以输出 HDR 显示器可以正确显示的 10
位值。HDR 显示器上输出的白度和亮度将由所选的定义"纸白色"的nit
值决定。SDR 规格将"纸白色"定义为
80nit，但这适用于黑暗环境下的影院。如今，使用者已习惯了亮得多的白色，例如智能手机的亮度约为
550 nit（以便在阳光下观看）、电脑显示器的亮度为 200-300 nit、SDR
电视的亮度为 120-150 nit 等。可在示例中使用向上/向下方向键调节"纸白色"的
nit。在白色旁边显示亮度值可能会欺骗眼睛，所以如果只想看到"纸白色"块，可以使用
A 按钮进行切换。

此示例有两种模式：

-   在场景中渲染具有特定值的块

-   使用特定亮度值 (nit) 渲染 ST.2084 曲线

# 已知问题

请注意，这里的 Project Scarlett
实现是有效的，但未来的更新将提供一种更简单的方法来使用自动色调映射。这将允许
Project Scarlett 标题仅显示 HDR 图像，SDR 输出和 GameDVR
将完全由系统处理。

# 隐私声明

在编译和运行示例时，示例可执行文件的文件名将发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。
