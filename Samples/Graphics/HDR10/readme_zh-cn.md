# HDR10 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

此示例将 UHD 电视切换为 HDR 模式，并渲染值高于 1.0f 的 HDR 场景，该场景在 UHD 电视上显示比白色更亮的效果。 该示例的目标是显示要使用的 API、创建 HDR 交换链的方式，以及大于 1.0f 的不同值在 UHD 电视上的效果。

![A picture containing timeline Description automatically generated](./media/image1.png)

![Text Description automatically generated](./media/image2.png)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关更多信息，请参阅*&nbsp;__运行示例__（位于 *GDK&nbsp;文档）中。*

# 使用示例

该示例使用以下控制。

| 操作 | 游戏板 |
|---|---|
| 切换显示 ST.2084 曲线 | A button |
| 仅显示纸质白块的切换开关 | B button |
| 调整白纸亮度 | D-pad |
| 调整值 | 左/右拇指摇杆 |
| 退出 | View button |

# 实现说明

此示例使用 API 确定附加的显示器是否支持 HDR。 如果是这样，它会将显示器切换到 HDR 模式。 一个非常简单的 HDR 场景，其值高于 1.0f，呈现到 FP16 后台缓冲区，并输出到两个不同的交换链，一个用于 HDR，另一个用于 SDR。 即使使用者使用 HDR 显示器，GameDVR 和屏幕截图仍需要 SDR 信号。

此示例具有
[DeviceResources](https://github.com/Microsoft/DirectXTK12/wiki/DeviceResources)
支持 HDR 和 SDR 交换链的类。

请参阅白皮书&ldquo;[Xbox One 上的 HDR](http://aka.ms/hdr-on-xbox-one)&rdquo;。

到目前为止，游戏正在使用 Rec.709 颜色主控件和 Rec.709 伽玛曲线输出和 SDR 信号。 UHD 显示器的一项新功能是更宽的颜色范围 （WCG）。 若要使用此功能，我们需要使用新的颜色空间 Rec.2020 颜色主控件。 UHD 显示器的另一个新功能是高动态范围 （HDR）。 若要使用此项，我们需要使用不同的曲线，即 ST.2084 曲线。 因此，若要输出 HDR 信号，需要将 Rec.2020 颜色主控件与 ST.2084 曲线配合使用。

若要显示 SDR 信号，只需在 HDR 场景中剪裁 1.0f 以上的所有值，然后使用 Rec.709 颜色质调输出 8 位值，即可应用简单的音调映射着色器。 请参阅
[PostProcess](https://github.com/Microsoft/DirectXTK12/wiki/PostProcess)
适用于 *DirectX 12 的 DirectX 工具包中*的类，用于其他音调映射运算符。

为了显示 HDR 信号，着色器用于将 Rec.709 颜色主控件旋转到 Rec.2020 颜色主控件，然后将 ST.2084 曲线应用于 HDR 显示可以正确显示的输出 10 位值。 HDR 显示器上输出的白色和亮度将由用于定义&ldquo;纸白色&rdquo;的所选 nits 值决定。 SDR 规范将&ldquo;纸白色&rdquo;定义为 80 个字节，但这适用于具有深色环境的影院。 如今，消费者习惯于更亮的白色，例如智能手机的 \~550 nits（以便可以在阳光下查看）、电脑显示器 200-300 Nits、SDR 电视 120-150 nits 等。 可以在示例中使用 DPad 向上/向下调整&ldquo;纸白&rdquo;的 nits。 显示白色旁边的亮值可能会欺骗眼睛，因此，如果只想看到&ldquo;纸白色&rdquo;块，则可以使用&ldquo;A&rdquo;按钮进行切换。

该示例有两种模式：

- 场景中具有特定值的呈现块

- 呈现具有特定亮度值 （nits） 的 ST.2084 曲线

# 已知问题

请注意，该实现有效，但你也可以使用 **HDRAutoToneMapping** 和/或 **SimpleHDR** 中的技术来允许游戏仅显示 HDR 映像，并且系统将完全处理 SDR 输出和 GameDVR。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


