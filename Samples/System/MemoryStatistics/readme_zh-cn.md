![](./media/image1.png)

# 内存统计信息示例

# 说明

此示例演示如何在 Xbox 游戏的运行时查询内存使用情况。 使用的主要 API 是 XMemGetWorkingSetStatistics，它用于获取游戏使用的资源的快照。 此示例演示如何获取当前内存使用情况的快照，或比较单个函数的快照前后的情况，以确定这些组件使用的内存。

# 生成示例

此示例是一个 Visual Studio 2017 项目，但也可以在较新版本的开发环境中加载和使用。

如果使用 Xbox One devkit，则应使用 *Gaming.Xbox.XboxOne.x64* 配置。

如果使用 Project Scarlett 开发工具包，则应使用 *Gaming.Xbox.Scarlett.x64* 配置以本机模式运行示例。 或者，可以使用 *Gaming.Xbox.XboxOne.x64* 配置在向后兼容模式下运行示例。

# 使用示例

此示例显示有关示例中内存使用情况的信息。 它最初在 3D 空间中显示单个模型。 可以在运行时创建更多模型，以了解它们如何影响资源使用情况。

![](./media/image3.png)

| 操作 | 游戏板 | 键盘 |
|---|---|---|
| 增加 teapot 的数目 | DPAD_RIGHT | 向右键 |
| 减少 teapot 的数目 | DPAD_LEFT | 向左键 |
| 显示有关在运行时的任何时间点的资源使用情况与初始化后的资源使用情况的比较的情况。 | Y button | P |
| 退出 | &ldquo;视图&rdquo;按钮 | Esc |

如果在玩游戏期间 Y 按钮/P 键显示的资源使用量持续增加，这可能表示内存泄漏或资源管理效率低下。

# 实现说明

此示例演示 **XMemGetWorkingSetStatistics** API 用于跟踪内存信息的三种方式。

- 它在 **Initialize** 函数的末尾捕获资源使用情况，以便在运行时随时与资源使用情况进行比较。

- 它在创建新 teapot 之前和之后进行捕获，以确定任何单个操作所需的内存量。

- 它还跟踪要查看的总内存使用量。

# 更新历史记录

- 初始版本 -- 2020 年 8 月

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


