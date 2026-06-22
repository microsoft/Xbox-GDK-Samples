![](./media/image1.png)

# DevkitTooling 示例

*此示例可用于 Microsoft 游戏开发工具包（2022 年 3 月）*

# 说明

Xbox 开发工具包计算机是功能强大的硬件，可用于实现除运行和测试游戏之外的目的。 Xbox Game Core 应用程序解锁了开发工具包的完整处理资源，并可以运行将其 API 使用限制为 WINAPI_FAMILY_GAMES 子集的 Win32 应用程序。

DevkitTooling 示例演示如何启动 CPU 和 GPU 子处理，以便它们可作为工具在 Xbox Devkits 上运行。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Project Scarlett，请将可用解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅* *GDK 文档中的*__运行示例__。&nbsp;

# 使用示例

DevkitTooling 示例显示单个屏幕，其中包含以子处理方式运行的工具的日志记录输出。

![Text Description automatically generated](./media/image3.png)

如果要启动示例 CPU 工具子处理，请在游戏板上按 \[X\]。 如果要启动示例 GPU 工具子进程，请在游戏板上按 \[Y\]。 自动退出之前，示例子进程将总共运行 5 秒。 日志输出将显示在屏幕上。

运行 GPU 工具子进程时，屏幕将更改为显示工具（简单三角形）而不是示例的呈现。 GPU 工具完成后，示例将再次开始呈现。 为了支持这一点，使用 *SuspendX* 暂停渲染，并使用 *ResumeX* 恢复渲染。 有关详细信息，请参阅实施说明。

![自动生成的形状描述](./media/image4.png)

# 实现说明

此示例使用了一些 Xbox 游戏中不常见的功能。 此外，还必须遵守一些特殊要求，从而阻止开发工具包遇到意外错误。 下面的每个节介绍了在开发工具包上运行工具时的不同功能、要求和其他注意事项。

## Xbox 游戏分区

要使在 Xbox 开发工具包上运行的应用程序获取对硬件资源的完全访问权限，它必须在&ldquo;游戏分区&rdquo;中运行。 &ldquo;游戏分区&rdquo;是 Xbox 主机上专门用于此目的的虚拟机。

通常，Xbox 的游戏会部署到开发工具包或作为包安装，从而向系统注册。 然后，Xbox 系统可以在&ldquo;游戏分区&rdquo;上正确启动这些游戏。

如果有未向系统注册的工具或其他独立进程，仍然可以在&ldquo;游戏分区&rdquo;上运行这些过程。 但是，&ldquo;游戏分区&rdquo;必须首先处于活动状态。 DevkitTooling 应用程序负责处理此问题，因为系统将确保在启动示例时标题分区处于活动状态。 然后，DevkitTooling 使用
[CreateProcess 启动工具或进程](https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa)
它位于&ldquo;游戏分区&rdquo;内，并且具有完全的硬件资源访问权限。

注意：*xbrun.exe* 工具还可以启动&ldquo;游戏分区&rdquo;中的应用程序。 但是，*xbrun.exe* 不会确保&ldquo;游戏分区&rdquo;首先处于活动状态。 如果游戏当前正在 Xbox 开发工具包上运行，则 *xbrun.ext* 的&ldquo;*/x/title*&rdquo;参数会指定应在游戏分区中运行可执行文件。 请参阅 GDK 文档，从而了解有关 *xbrun.exe* 的详细信息。

## 端口与 Xbox 防火墙

默认情况下，Xbox 主机和 Xbox 开发工具包具有防火墙，可阻止 TCP 和 UDP 的大多数未经请求的入站连接请求。 对于零售游戏，始终阻止未经请求的入站 TCP，没有解决办法。 但是，允许 UDP 使用首选的本地 UDP 多玩家端口，可以使用 *XNetworkingQueryPreferredLocalUDPMultiplayerPort\[Async\]* 查询该端口。

Xbox 开发工具包可以通过 MicrosoftGame.config 文件中的条目配置为允许未经请求的入站 TCP 和 UDP 数据包:

```
<?xml version="1.0" encoding="utf-8"?>
<Game configVersion="0">
...
  <DevelopmentOnly>
    <DebugNetworkPortList>
      <DebugNetworkPort>4600</DebugNetworkPort>
      <DebugNetworkPort>4601<DebugNetworkPort>
    </DebugNetworkPortList>
  </DevelopmentOnly>
</Game>
```


对于每个将允许未经请求的入站连接的端口，应将不同的 *DebugNetworkPort* 条目添加到 *DebugNetworkPortList* 中。 对于直接通过 TCP 相互通信的 P2P 工具等情况，此为必需操作。

更新 MicrosoftGame.config 文件后，应重新部署游戏以拾取更新。

## Xbox 开发工具包上的 Win32 应用程序

&ldquo;游戏分区&rdquo;可以直接运行尚未进行任何特定 Xbox 集成的 Win32 主机应用程序。 但是，Win32 API 可用性为通常适用于 Windows 电脑的精简子集。

要将 Win32 应用程序限制为仅可用的 API，应在包含 Windows 标头之前将 *WINAPI_FAMILY* 定义为 *WINAPI_FAMILY_GAMES*。

```
#define WINAPI_FAMILY WINAPI_FAMILY_GAMES
#include <Windows.h>
```


CPUTool 项目就是一个非常简单的示例。 它会生成限制为 *WINAPI_FAMILY_GAMES* 的主机 Win32 应用程序。 因此，CPUTool.exe 可以在 Windows 电脑和 Xbox 主机上成功运行。

## Xbox 开发工具包上的 GPU 进程

对于在&ldquo;游戏分区&rdquo;中运行的进程，使用 D3D GPU 有更严格的限制:

* 在&ldquo;游戏分区&rdquo;中，一次只能有 1 个进程使用 Direct3D。

* 可以使用特定于 Xbox 的 D3D12 设备方法 *SuspendX/ResumeX* 管理多个利用 D3D 的进程。 两个进程不得同时主动利用 D3D。

* 必须使用特定于 Xbox 的 D3D 标头和库。

错误处理这些注意事项可能会导致应用程序崩溃、呈现损坏，甚至导致主机崩溃。 在创建利用 GPU 的子进程之前，DevkitTooling 示例会使用 *SuspendX* 处理这些情况。 子进程完全退出后，会使用 *ResumeX* 重新启用示例呈现。

GPUTool 项目是一个简单的进程，可在屏幕上渲染三角形。 其用于展示在遵守 GPU 注意事项时如何在&ldquo;游戏分区&rdquo;中使用 GPU 子进程。

## 具有多个进程的 GameInput

GameInput 库当前(在创建此示例时)不支持在创建此示例时一次用于多个进程。 此外，无法在进程内或通过终止和启动进程对其进行清理和重新初始化。 因此，初始化 GameInput 的第一个进程是唯一可以使用它的进程。 任何未来尝试初始化的进程都将崩溃。

# 更新历史记录

2021 年 6 月初始版本。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


