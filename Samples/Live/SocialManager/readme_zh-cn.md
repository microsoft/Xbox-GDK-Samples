  ![](./media/image1.png)

#   社交示例

*此示例兼容于 Microsoft 游戏开发工具包(2020 年 6 月)*

# 

# 说明

此示例演示 Microsoft 游戏 SDK(GDK)提供的 社交管理器 C-API。

此示例包括以下场景:

-   添加用户并创建组

-   基于筛选器检索社交组

-   响应社交管理器事件

# 生成示例

-   如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为
    **Gaming.Xbox.XboxOne.x64**。

-   如果使用 Project Scarlett，请将活动解决方案平台设置为
    **Gaming.Xbox.Scarlett.x64**。

*有关详细信息，请参阅 GDK 文档中的*"运行示例"*。*

# 使用示例

该示例使用标准游戏板或键盘进行控制。屏幕底部显示包含所有可用操作的输入图例。

## 示例屏幕

![Text Description automatically generated](./media/image3.png)

| 操作                        |  游戏板                                 |
|-----------------------------|----------------------------------------|
| 登录用户                    |  菜单按钮/选项卡键                      |
| 刷新当前筛选器的 UI         |  按钮/F5 键                             |
| 已查看的更改社交组          |  LB 和 RB 肩部按钮/向左或向右箭头键     |
| 退出                        |  "视图"按钮/ESC 键                      |

# 实施说明

直接与社交管理器 API 接口的代码封装到 SocialManagerIntegration.cpp
文件中。

# 隐私声明

在编译和运行示例时，将向 Microsoft
发送示例可执行文件的文件名以帮助跟踪示例使用情况。若要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。

# 更新历史记录

**更新时间：***2021 年 7 月*

**初始版本：***2019 年 9 月*
