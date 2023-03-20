  ![](./media/image1.png)

#   FrontPanelGame 示例

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

# 

# 说明

FrontPanelGame 是完全在 Xbox One X Devkit 和 Project Scarlett Devkit
前面板上实现的经典"贪吃蛇游戏"。此示例主要是为了娱乐而提供，但也确实展示了前面板控件的一些重要方面：

-   前面板从标题代码开始完全可编程

-   按钮和方向键 API 均类似于熟悉的游戏手柄 API

-   此示例展示了一些实用程序代码，可以在自己的前面板项目中使用它们

    -   使用 RasterFont 呈现文本（另请参阅：FrontPanelText 示例）

    -   FrontPanelInput 提供了一个可重用的输入类，类似于
        DirectXTK::GamePad

    -   FrontPanelDisplay 提供了一个简单的类来管理前面板缓冲区

    -   绘制基本形状（如线条和矩形）的代码

![](./media/image3.png)

# 构建示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

有关详细信息，请参阅 GDK 文档中的"运行示例"。

# 使用示例

| 操作                                      |  前面板                   |
|-------------------------------------------|--------------------------|
| 开始新游戏                                |  按钮 1                   |
| 移动贪吃蛇                                |  方向键（左、右、上、下） |
| 获取屏幕截图                              |  方向键选择               |
| 在游戏模式和系统模式之间切换              |  按住"方向键选择"         |

# 更新历史记录

2019 年 4 月，首次发布此示例。

2019 年 11 月，支持 Project Scarlett Devkit。

# 隐私声明

在编译和运行示例时，示例可执行文件的文件名将发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私政策的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。
