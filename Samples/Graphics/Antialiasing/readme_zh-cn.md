  ![](./media/image1.png)

#   抗锯齿示例

*此示例与 Microsoft 游戏开发工具包（2020 年 6 月）兼容*

# 说明

# 此示例展示了不同的抗锯齿方法（SMAA、SMAA2x 和 FXAA）。

![](./media/image3.jpeg)

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Xbox Series X|S 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

*有关详细信息，请参阅 GDK 文档中的*运行示例*。*

# 使用示例

| 操作                                  |  游戏手柄                     |
|---------------------------------------|------------------------------|
| 循环 AA 技术                          |  A 按钮/ X 按钮               |
| 切换硬件 AA                           |  B 按钮                       |
| 循环 MSAA 计数                        |  Y 按钮                       |
| 选择 SMAA 边缘检测技术                |  方向键向左键、向下键、向右键 |
| 旋转视图                              |  左控制杆                     |
| 重置视图                              |  左控制杆（单击）             |
| 退出                                  |  视图按钮                     |

# 实现说明

此示例实现抗锯齿的后期处理着色器技术。

## SMAA

要了解 SMAA 算法的详细说明，可访问
<http://www.iryoku.com/smaa/>。场景将呈现并传递给通过 3
此传递来执行的算法：

-   边缘检测传递：这可以使用深度、luma
    或颜色值来完成。深度运行速度最快，并提供更好的结果。

-   混合权重传递

-   邻近区域混合传递

对于 SMAA 2x，首先使用 MSAA 2x
呈现场景，然后对从多示例生成的每个网格呈现器单独运行上述传递。

## FXAA

只需呈现场景并将其传递给着色器即可。要了解 FXAA 算法的详细说明，请访问
<http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf>。

*请参阅 MiniEngine 演示以了解如何使用 DirectCompute 实现 FXAA*。

***如果希望在游戏中实现这些技术，请务必阅读 ThirdPartyNotices.txt。***

请参阅 **SimpleMSAA**
示例，以查看使用内置多采样硬件的基础知识的演示，并参阅 **Multisampling**
以更详细地了解 MSAA。

# 更新历史记录

示例的原始版本是使用基于 XSF 的框架编写的。2020 年 6
月，示例被改写为使用 ATG 示例模板。

# 隐私声明

在编译和运行示例时，将向 Microsoft
发送示例可执行文件的文件名以帮助跟踪示例使用情况。若要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。
