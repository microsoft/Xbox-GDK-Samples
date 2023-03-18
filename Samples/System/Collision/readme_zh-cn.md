  ![](./media/image1.png)

#   冲突示例

*此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容*

# 

# 说明

此示例展现了 Xbox One 应用中针对简单边界体积测试的 DirectXMath
碰撞类型。

![C:\\temp\\xbox_screenshot.png](./media/image3.png)

# 构建示例

如果使用 Project Scarlett，你需要将 Gaming.Xbox.Scarlett.x64
平台配置添加到项目中。可以通过 *Configuration Manager*
执行此操作：选择"活动解决方案平台"下的"Configuration
Manager"选项，然后选择"新建..."。将"键入或选择新平台"设置为
Gaming.Xbox.Scarlett.x64，将"从此处复制设置"设置为
Gaming.Xbox.XboxOne.x64。然后选择"确定"。

*有关详细信息，请参阅 GDK 文档中的"*运行示例*"。*

# 使用示例

示例显示了 4 个不同的"碰撞"组：

1.  一个静态边界**平截头体**，与动画球体、轴对齐框、方向框和三角形相碰撞。

2.  一个静态**轴对齐框**，与动画球体、轴对齐框、方向框和三角形相碰撞。

3.  一个静态边界**方向框**，与动画球体、轴对齐框、方向框和三角形相碰撞。

4.  一条动画**光线**，与动画球体、轴对齐框、方向框和三角形相碰撞。如果有光线击中，标记框则会被放置在目标物体上的交叉点处。

| 操作                         |  游戏手柄                              |
|------------------------------|---------------------------------------|
| 围绕组沿轨道移动镜头 X/Y     |  右操纵杆                              |
| 重置视图                     |  右操纵杆按钮                          |
| 将焦点置于"平截头体"组       |  向上方向键                            |
| 将焦点置于"轴对齐框"组       |  向右方向键                            |
| 将焦点置于"方向框"组         |  向下方向键                            |
| 将焦点置于"光线"测试组       |  向左方向键                            |
| 切换帮助                     |  "菜单"按钮                            |
| 退出                         |  "视图"按钮                            |

# 实现说明

有关 DirectXMath 的边界体积类型的更多信息，请参阅针对以下内容的
[Microsoft
Docs](https://docs.microsoft.com/en-us/windows/desktop/dxmath/directxmath-portal)：

-   **BoundingBox** 类

-   **BoundingFrustum** 类

-   **BoundingOrientedBox** 类

-   **BoundingSphere** 类

-   **TriangleTests** 命名空间

可以在 [GitHub](https://github.com/Microsoft/DirectXMath)
上获取最新版本的 DirectXMath。

# 已知问题

DirectXMath 的 **BoundingFrustum** 类仅适用于左手视图系统。

# 更新历史记录

此示例的 Xbox One XDK 版本最初发布于 2016 年 5 月。可以在
[GitHub](https://github.com/walbourn/directx-sdk-samples/tree/master/Collision)上找到此示例的最新旧式
DirectX SDK 版本。

# 隐私声明

在编译和运行示例时，示例可执行文件的文件名将发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私政策的详细信息，请参阅 [Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。
