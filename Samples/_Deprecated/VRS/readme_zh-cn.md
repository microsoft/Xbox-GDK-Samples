  ![](./media/image1.png)

#   可变速率着色示例

*此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容*

# 

# 说明

Both Anaconda (Xbox Series X) 和 Lockhart 均支持 tier2.x
可变速率着色。这是 tier2 超集，如下所述：
<https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html>

该技术通过每像素调用像素着色器少于一次并将结果广播到多个像素，来减少像素着色器负载。这对于正向渲染（包括透明度）特别有吸引力，也有利于延迟渲染。

该示例每帧两次渲染完全相同的场景，同时对两者进行计时，并报告由于 VRS
而产生的节时。 使用 VRS 渲染的场景从使用 VRS
渲染的前一帧得出着色率，不是从实测中得出。调试模式可用于查看实测情况，使用
VRS 渲染的场景，绝对差和使用的着色率。

Raymarched 阴影和 AO 用于为像素着色器提供一些有用的操作。

# ![](./media/image3.png)

#  构建示例

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

此示例不支持 Xbox One。

*有关详细信息，请参阅 GDK 文档中的"*运行示例*"。*

# 使用示例

| 操作                             |  游戏手柄                          |
|----------------------------------|-----------------------------------|
| 更改调试模式                     |  D-Pad 左/右                       |
| 控制太阳                         |  Y 按钮和肩部按钮                  |
| 更改着色率公差                   |  触发器按钮                        |
| 照相机                           |  摇杆和 DPad 向上/向下             |
| 更改可视化                       |  X 按钮                            |
| 循环着色率生成着色器             |  B 按钮                            |
| 退出                             |  "视图"按钮                        |

# 

# 实现说明

VRS 的 API 图面非常小。主要关注项是：

地形::RenderVRS -- 其将调用 RSSetShadingRate 和 RSSetShadingRateImage

地形::CalculateShadingRat --
这会生成计算着色器，该着色器根据上一帧生成着色率

另外在 HLSL 中， 函数 BuiildShadingRate 位于 GenerateShadingRate.hlsl 内

对于 wave32 和 wave64，存在四个变体
BuildShadingRate，采用全分辨率或半分辨率数据工作。根据我们的实验，wave64速度更快，建议使用一半的分辨率。

# 已知问题

示例尚未在 Lockhart 硬件上进行测试。

# 更新历史记录

2020 年 1 月初始发布

# 隐私声明

在编译和运行示例时，示例可执行文件的文件名将发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私政策的详细信息，请参阅 [Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。
