  ![](./media/image1.png)

#   SimplePLM 示例

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

# 说明

本示例展示 PLM 事件和 PLM
相关事件的行为。示例将打印到屏幕上，并调试输出时间戳、线程
ID、函数名称以及与 PLM
相关事件的该函数相关的任何其他数据。使用本示例可以了解 PLM 事件的行为。

示例还可以执行导致 PLM
相关转换的操作，以演示受影响的事件和状态。其中包括启动到全屏幕 SystemOS
体验（设置）和"显示 AccountPicker TCUI"。

# 使用示例

##  主屏幕

![Sample Screenshot](./media/image3.png)

| 操作                                  |  Xbox One 控制器              |
|---------------------------------------|------------------------------|
| 启动设置应用                          |  A                            |
| 显示 AccountPicker                    |  X                            |
| 退出                                  |  视图                         |

在 Visual Studio
中运行以测试挂起和恢复操作时，用户可以使用"生命周期事件"菜单使应用挂起，然后恢复，如下所示。

![](./media/image4.png)

未在 Visual Studio
调试器下运行时，用户可通过使应用不可见（启动设置将执行此操作）导致应用程序挂起，该应用将在
10
分钟后挂起。若要恢复已挂起的应用程序，用户只需使应用程序再次显示即可。或者，用户可以使用
Xbox One 管理器执行挂起和恢复操作。

另一种选择是使用通过 XDK 工具安装的 Xbapp.exe
工具，该工具可通过以下命令暂停和恢复应用：

Xbapp.exe suspend SimplePLM_1.0.0.0_x64\_\_zjr0dfhgjwvde

Xbapp.exe resume SimplePLM_1.0.0.0_x64\_\_zjr0dfhgjwvde

# 实现说明

在关联的函数和事件处理程序（而不是 SimplePLM.cpp）中的 Main.cpp
中会提供事件日志记录。另外，在选择不同操作时也会提供日志，还会提供用于提醒用户注意控件的初始日志。

# 隐私声明

在编译和运行示例时，示例可执行文件的文件名将发送给
Microsoft，用于帮助跟踪示例使用情况。要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅《[Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)》。
