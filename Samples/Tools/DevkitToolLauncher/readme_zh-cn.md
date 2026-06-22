![](./media/image1.png)

# DevkitToolLauncher 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

Xbox 开发工具包计算机是功能强大的硬件，可用于实现除运行和测试游戏之外的目的。 Xbox Game Core 应用程序解锁了开发工具包的完整处理资源，并可以运行将其 API 使用限制为 WINAPI_FAMILY_GAMES 子集的 Win32 应用程序。

DevkitToolLauncher 示例提供了 Xbox 应用程序，该应用程序有助于在开发工具包的游戏分区中运行进程，从而实现工具目的。 此外，还提供了两个示例进程以展示如何使用 Win32 主机仅 CPU 进程和 Xbox D3D12 GPU 进程。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Project Scarlett，请将可用解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

*有关详细信息，请参阅 **GDK 文档中*的&ldquo;&nbsp;__运行示例__&rdquo;。

还提供了多个脚本，从而推动示例的命令行使用，例如生成、部署和运行。 要使用脚本，请打开 "*Xbox \[One/Scarlett\] VS \[2017/2019\] Gaming Command Prompt*"，然后导航到示例目录。 下表介绍了这些脚本:

| 脚本 | 用法 |
|---|---|
| Build.bat | Build.bat \[Configuration\] \[Platform\]<br /><br />为指定的配置(调试、发布)和平台(XboxOne、Scarlett)生成 DevkitToolLauncher.exe、CPUTool.exe 和 GPUTool.exe。 始终使用 x64 配置将 CPUTool.exe 生成为简单的主机 Win32 应用程序。 |
| Deploy.bat | Deploy.bat \[Platform\]<br /><br />部署之前生成到默认开发工具包的 DevkitToolLauncher.exe 的松散内部版本。 |
| DeployExampleTools.bat | DeployExampleTools.bat \[Configuration\]\[Platform\]<br /><br />将 CPUTool.exe 和 GPUTool.exe 复制到默认 devkit 上的 SystemScratch 驱动器。 这些应用程序部署到 "d:\\DevkitToolLauncherExampleTools\\\[CPU/GPU\]Tool"文件夹。 |
| BuildAndDeploy.bat | BuildAndDeploy.bat \[Configuration\] \[Platform\]<br /><br />将 Build.bat、Deploy.bat 和DeployExampleTools.bat 合并到一个脚本中。 |
| Run.bat | Run.bat \[CommandLine\]<br /><br />启动之前部署的 DevkitToolLauncher.exe，并将所有脚本参数传递到 DevkitToolLauncher.exe 的命令行中。 请参阅以下 [命令行使用](#Command_Line_Usage)节，从而了解有关使用命令行参数的详细信息。 |

# 使用示例

此示例提供 3 个进程：DevkitToolLauncher.exe、CPUTool.exe 和 GPUTool.exe。 DevkitToolLauncher.exe 为主要的示例进程，另外两个进程作为仅 CPU 和 GPU 利用进程的示例提供，可由 DevkitToolLauncher.exe 运行。

## 示例功能

DevkitToolLauncher 进程拆分为 3 个屏幕:&ldquo;工具浏览器&rdquo;、&ldquo;启动设置&rdquo;和&ldquo;运行时日志&rdquo;。 不同的屏幕允许查找要运行的进程、设置该进程的启动信息，以及查看正在运行的进程的主机输出。

### 工具浏览器

![文本说明已自动生成](./media/image3.png)

&ldquo;工具浏览器&rdquo;通过开发工具包的系统暂存驱动器(d:\\)提供文件浏览器界面。 使用此浏览器，可以导航到可执行文件以运行并选择该文件。 选择该可执行文件后，示例会转到&ldquo;*启动设置*&rdquo;屏幕。

### 启动设置

![自动生成的图形用户界面、文本描述](./media/image4.png)

&ldquo;*启动设置*&rdquo;屏幕允许设置选定工具可执行文件的启动行为。 使用此屏幕，可以设置命令行参数、工作目录，以及是否应在有/无 GPU 支持的情况下启动示例。

选择任意文本输入将显示屏幕键盘以输入参数。 commandline 参数和工作目录保存在系统暂存驱动器根路径下名为 "DevkitToolLauncherParameterCache.json" 的缓存文件中。 可以删除此缓存以重置保存的参数。

为了支持具有 GPU 功能的工具，&ldquo;*启动(CPU 和 GPU)&rdquo;*按钮将在启动工具之前挂起示例上的渲染。 工具可执行文件退出或终止后，渲染将自动重启。 请参阅 [Xbox 开发工具包上的 GPU 进程](#_Hlk70930525) 节，从而了解有关此示例如何支持 GPU 进程的详细信息。

选择*&ldquo;启动(仅 CPU)&rdquo;*将进入&ldquo;*运行时日志*&rdquo;屏幕，不会挂起渲染。

如果选定工具可执行文件具有 GPU 功能，且已选择&ldquo;*启动(仅 CPU)*&rdquo;，则主机将因两个进程尝试使用 GPU 而遇到未定义的行为问题。 例如，应用程序可能会崩溃屏幕、可能会看到项目，或者主机可能会关闭。 请参阅 [Xbox 开发工具包上的 GPU 进程](#_Hlk70930525)，从而了解详细信息。

如果正在运行的工具可执行文件需要允许未经请求的入站连接，则必须通过 MicrosoftGame.config 文件打开这些端口。 请参阅 [端口和 Xbox 防火墙](#Ports_Firewall)，从而了解详细信息。

### 运行时日志

![文本说明已自动生成](./media/image5.png)

选择&ldquo;*启动设置*&rdquo;屏幕上的其中一个启动选项时，&ldquo;运行时日志&rdquo;屏幕是显示的最后一个屏幕。

此屏幕将启动选定的可执行文件作为子进程，并通过管道将其 stdout 和 stderr 写入传输到 DevkitToolLauncher 进程中。 屏幕上会显示这些通过管道传输的输出。 可以使用手柄导航屏幕日志。

如果工具子进程使用 GPU，且通过&ldquo;*启动设置*&rdquo;中的&ldquo;*启动(CPU 和 GPU)*&rdquo;按钮挂起渲染，则不会显示此屏幕。 但是，示例仍然在通过管道传输输出并管理正在运行的进程。 当工具子进程退出时，此屏幕将自动显示，并随附发生的所有日志记录。

无论是否挂起渲染，始终都可以按住手柄上的 \[B\] 以终止工具子进程。 工具子进程不再运行后，可以按 [B] 以返回&ldquo;*工具浏览器*&rdquo;屏幕并重新开始。

## 命令行用法

无需使用提供的脚本或命令行即可使用示例。 我们提供这些工具是为了展示如何使用 DevkitToolLauncher 示例通过命令行启动工具。

要开始，请打开其中一个 Xbox 游戏命令提示符。 然后，使用 "*cd /D \[SamplePath\]*" 命令导航到示例目录。

### 生成

可以使用示例目录中提供的 Build.bat 脚本完成生成。 命令行参数为:

```
Build.bat [Configuration] [Platform]
```


例如，要为 Scarlett 生成内容，请使用"*build.bat release scarlett*"命令。

### 部署

为了进行部署，我们提供了另一个名为 Deploy.bat 的脚本。 部署 DevkitToolLauncher 示例将在内部使用 "*xbapp deploy*" 命令，从而将松散的内部版本成复制到开发工具包中。

```
Deploy.bat [Platform]
```


要部署 GPUTool 和 CPUTool，请使用 DeployExampleTools.bat 脚本。 此脚本会将这些示例工具复制到名为 "DevkitToolLauncherExampleTools" 的文件夹的 SystemScratch 驱动器中。 这些工具会部署到 SystemScratch 驱动器，从而允许示例的*工具浏览器 *屏幕发现它们，而不是位于&ldquo;示例&rdquo;的主安装文件夹中。

```
DeployExampleTools.bat [Configuration] [Platform]
```


### 一步生成与部署

为了简化操作，BuildAndDeploy.bat 脚本将上述三个脚本合并为了一个脚本。

BuildAndDeploy.bat \[Configuration\] \[Platform\]

### 正在运行

DevkitToolLauncher.exe 包含命令行设置，从而允许跳过&ldquo;*工具浏览器* &rdquo;和&ldquo;*启动设置*&rdquo;屏幕、启动工具进程，以及直接导航到&ldquo;*运行时日志*&rdquo;屏幕。 命令行设置如下所示:

DevkitToolLauncher.exe \[-gpu/processUsesGpu\] \[-workingDir \"path\"\] \[\-- \"process\" \[commandline\]\]

此命令行允许设置可在&ldquo;*启动设置*&rdquo;屏幕上设置的相同信息:

| 参数 | 说明 |
|---|---|
| -gpu 或 - processUsesGpu | 可选。 如果指定，则 DevkitToolLauncher.exe 示例将在子进程的持续时间内挂起渲染。 |
| -workingDir | 如果指定要运行的进程，则此为必需项。 指定要为子进程设置的工作目录。 |
| \-- | 可选。 如果 "\--\" 标记出现在命令行上，则示例会将以下参数解释为要运行的子进程。 子进程必须为进程的完整路径。 将进程后的每个参数作为该子进程的命令行进行传递。 |

例如，要运行使用 DeployExampleToole.bat 复制到系统暂存驱动器的示例 CPUTool 5 秒，可以使用以下命令:

```
Run.bat -workingDir "d:\DevkitToolLauncherExampleTools\CPUTool" --"d:\DevkitToolLauncherExampleTools\CPUTool\CPUTool.exe" 5
```


要运行具有渲染的示例 GPUTool，可以使用以下命令:

```
Run.bat -gpu -workingDir "d:\DevkitToolLauncherExampleTools\GPUTool"
-- "d:\DevkitToolLauncherExampleTools\GPUTool\GPUTool.exe"
```


注意：GPUTool.exe 进程会在屏幕上渲染三角形，且不会自动退出。 可以按住手柄上的 \[B\] 以终止 GPUTool.exe (由 DevkitToolLauncher.exe 启动时)。

### 正在端接

Run.bat 命令行使用 *xbapp* 启动 DevkitToolLauncher 示例并传递命令行。 如果要从命令行终止示例，则可以运行 "*xbapp terminate*"。

## 自己的工具

DevkitToolLauncher 示例旨在成为完整的托管产品/服务以帮助你运行自己的工具，并旨在成为教育资源以说明如何将开发工具包用于此类情况。 如果你有自己的工具或计划编写一些工具，则可以执行上述 Run.bat 步骤托管它们。

请参阅 [Xbox 开发工具包上的 Win32 应用程序](#Win32_Info)，从而了解有关可作为工具运行的主机应用程序的详细信息。 请参阅 [Xbox 开发工具包上的 GPU 进程](#_Hlk70930525)，从而了解有关可利用开发工具包的 GPU 的工具的详细信息。

需要将工具置于开发工具包的系统暂存驱动器，以便由 DevkitToolLauncher 托管。 为此，可以使用 "*xbcp.exe*" 工具:

```
xbcp [ToolLocationPathOnLocalPC] xd:\[Folder]
```


# 实现说明

此示例使用了一些 Xbox 游戏中不常见的功能。 此外，还必须遵守一些特殊要求，从而阻止开发工具包遇到意外错误。 下面的每个节介绍了在开发工具包上运行工具时的不同功能、要求和其他注意事项。

## Xbox 游戏分区

要使在 Xbox 开发工具包上运行的应用程序获取对硬件资源的完全访问权限，它必须在&ldquo;游戏分区&rdquo;中运行。 &ldquo;游戏分区&rdquo;是 Xbox 主机上专门用于此目的的虚拟机。

通常，Xbox 的游戏会部署到开发工具包或作为包安装，从而向系统注册。 然后，Xbox 系统可以在&ldquo;游戏分区&rdquo;上正确启动这些游戏。

如果有未向系统注册的工具或其他独立进程，仍然可以在&ldquo;游戏分区&rdquo;上运行这些过程。 但是，&ldquo;游戏分区&rdquo;必须首先处于活动状态。 由于系统将确保启动 DevkitToolLauncher.exe 时&ldquo;游戏分区&rdquo;处于活动状态，因此 DevkitToolLauncher 应用程序会处理此问题。 随后，DevkitToolLauncher 启动工具或进程，使用
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
      <DebugNetworkPort>4601</DebugNetworkPort>
    </DebugNetworkPortList>
  </DevelopmentOnly>
</Game>
```


对于每个将允许未经请求的入站连接的端口，应将不同的 *DebugNetworkPort* 条目添加到 *DebugNetworkPortList* 中。 对于直接通过 TCP 相互通信的 P2P 工具等情况，此为必需操作。

更新 MicrosoftGame.config 文件后，应重新部署游戏以拾取更新。 为 DevkitToolLauncher 示例执行此操作的一种简单方法是，再次运行 *BuildAndDeploy.bat* 脚本。

## Xbox 开发工具包上的 Win32 应用程序

&ldquo;游戏分区&rdquo;可以直接运行尚未进行任何特定 Xbox 集成的 Win32 主机应用程序。 但是，Win32 API 可用性为通常适用于 Windows 电脑的精简子集。

要将 Win32 应用程序限制为仅可用的 API，应在包含 Windows 标头之前将 *WINAPI_FAMILY* 定义为 *WINAPI_FAMILY_GAMES*。

```
#define WINAPI_FAMILY WINAPI_FAMILY_GAMES
#include <Wwindows.h>
```


CPUTool 项目就是一个非常简单的示例。 它会生成限制为 *WINAPI_FAMILY_GAMES* 的主机 Win32 应用程序。 因此，CPUTool.exe 可以在 Windows 电脑和 Xbox 主机上成功运行。

注意：这些应用程序应仅为主机应用程序，且不使用渲染库。 对于渲染，可以使用 Xbox D3D12，下一节对此进行了说明。

## Xbox 开发工具包上的 GPU 进程

对于在&ldquo;游戏分区&rdquo;中运行的进程，使用 D3D GPU 有更严格的限制:

* 在&ldquo;游戏分区&rdquo;中，一次只能有 1 个进程使用 Direct3D。
* 可以使用特定于 Xbox 的 D3D12 设备方法 *SuspendX/ResumeX* 管理多个利用 D3D 的进程。 两个进程不得同时主动利用 D3D。
* 必须使用特定于 Xbox 的 D3D 标头和库。

错误处理这些注意事项可能会导致应用程序崩溃、呈现损坏，甚至导致主机崩溃。 在创建利用 GPU 的子进程之前，DevkitToolLauncher 示例会使用 *SuspendX*处理这些情况。 子进程完全退出后，会使用 *ResumeX* 重新启用示例呈现。

GPUTool 项目是一个简单的进程，可在屏幕上渲染三角形。 其用于展示在遵守 GPU 注意事项时如何在&ldquo;游戏分区&rdquo;中使用 GPU 子进程。

## 具有多个进程的 GameInput

GameInput 库当前(在创建此示例时)不支持在创建此示例时一次用于多个进程。 此外，无法在进程内或通过终止和启动进程对其进行清理和重新初始化。 因此，初始化 GameInput 的第一个进程是唯一可以使用它的进程。 任何未来尝试初始化的进程都将崩溃。

DevkitToolLauncher 示例使用 GameInput 提供输入处理，因此，工具子进程当前无法利用 GameInput。

## 示例 CPU 使用开销

通常，DevkitToolLauncher 示例的 CPU 开销非常低。 它通过 [睡眠](https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleep) 将帧速率限制为 30FPS，从而确保保持低使用率。 非睡眠时，大多数的示例功能都非常快。 但是，如果工具子进程需要获取尽可能多的所有 CPU 核心的处理时间，则&ldquo;调试&rdquo;配置中的 UITK 渲染可能会出现问题。

要在&ldquo;调试&rdquo;配置中获得最佳工具子进程性能，可以将 &ldquo;*启动(CPU 和 GPU)*&rdquo;(或使用命令行时的 -gpu)与任意进程(包括非 gpu 进程)结合使用。 由于这会挂起示例渲染，因此免去了 DevkitToolLauncher 示例的大部分开销。

使用&ldquo;发布&rdquo;配置生成内容时，也有效消除了 UITK 的性能影响。

# 更新历史记录

2021 年 6 月初始版本。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


