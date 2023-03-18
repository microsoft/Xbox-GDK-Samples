  ![](./media/image1.png)

#   FrontPanelLogo 示例

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

# 

# 说明

这个示例提供了一些起始代码，帮助你使用标准的图像格式呈现 Xbox One X
Devkit 和 Project Scarlett devkit
前面板显示的图像。例如，如果你要在展会或会议上展示你的游戏，你可能希望在前面板上有一些与你的游戏艺术风格和样式一致的图形。该示例还在主显示屏上显示图像，因此也可以在
Xbox One S 或 Xbox One
开发工具包上运行，但以这种方式使用该示例的实用程序非常有限。

# 构建示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
Gaming.Xbox.XboxOne.x64。

如果使用 Project Scarlett，请将活动解决方案平台设置为
Gaming.Xbox.Scarlett.x64。

有关详细信息，请参阅 GDK 文档中的"运行示例"。

默认情况下，该示例在执行过程中使用两个图像在前面板和主显示屏上显示。FrontPanelLogo.png
显示在前面板上，而 FullScreenLogo.png
显示在主显示屏上。为了进行快速、简单的自定义，只需将 FrontPanelLogo.png
和 FullScreenLogo.png 替换成你自己的作品，然后重新生成。

# 使用示例

## 主显示屏徽标

![](./media/image3.png)

## 前面板显示屏徽标

![](./media/image4.png)

| 操作                                   |  游戏手柄                    |
|----------------------------------------|-----------------------------|
| 退出                                   |  "视图"按钮                  |

# 

# 

# 实现说明

该示例使用一个帮助程序类
FrontPanelDisplay，该类管理前面板的缓冲区并提供简化显示屏操作的方法。具体而言，该示例使用
FrontPanelDisplay::LoadWICFromFile 加载标准的 .png
图像文件。该方法支持多种标准图像格式，包括 PNG、JPG 和
BMP。该方法还将自动缩放图像并将其转换为前面板所需的尺寸和像素格式。如果担心前面板的图像质量，则可以通过预先编辑图像以使其最好地适应面板的尺寸和像素格式来获得最佳效果。（面板
256x64 像素，每个像素 16 灰度。）

要将自定义前面板图像快速添加到自己的游戏中，请将 FrontPanelDisplay
类（和支持代码）添加到代码库中，然后仅复制示例中的几行代码以初始化显示屏并加载图像：

if (XFrontPanelIsAvailable())

{

// 初始化 FrontPanelDisplay 对象

m_frontPanelDisplay = std::make_unique\<FrontPanelDisplay\>();

// 加载徽标图像

m_frontPanelDisplay-\>LoadWICFromFile(L\"Assets\\\\FrontPanelLogo.png\");

}

在初始化/更新代码路径中的某个位置，你将至少需要调用
FrontPanelDisplay::Present()：

if (XFrontPanelIsAvailable())

{

// wait a few frames and then this only needs to be called once

if (m_timer.GetFrameCount() == 10)

{

m_frontPanelDisplay-\>Present();

}

}

# 更新历史记录

2019 年 4 月第一次发布。

2019 年 11 月支持 Project Scarlett Devkit。

# 隐私声明

在编译和运行示例时，示例可执行文件的文件名将发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私政策的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。
