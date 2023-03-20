  ![](./media/image1.png)

#   DumpTool 示例

此示例与 Microsoft 游戏开发工具包预览版（2019 年 11 月）兼容

# 

# 说明

DumpTool 在与 Xbox One Title
相同的操作系统分区中运行，并为你按名称指定为工具参数的另一个进程生成故障转储。你可以编译该工具以便立即使用，也可以从源代码中浏览，以将故障转储功能添加到自己的工具或标题。

# 构建示例

如果使用 Project Scarlett，你需要将 Gaming.Xbox.Scarlett.x64
平台配置添加到项目中。可以通过 Configuration Manager
执行此操作：选择"活动解决方案平台"下的"Configuration
Manager"选项，然后选择"新建..."。将"键入或选择新平台"设置为
Gaming.Xbox.Scarlett.x64，将"从此处复制设置"设置为
Gaming.Xbox.XboxOne.x64。然后选择"确定"。

有关详细信息，请参阅 GDK 文档中的"运行示例"。

# 使用示例

DumpTool 编译为标题模式控制台应用程序（另请参阅《[MSDN
白皮书](https://developer.xboxlive.com/en-us/platform/development/education/Documents/Title%20Mode%20Console%20Applications.aspx)》。）
使用 Visual Studio 将 .exe
部署到控制台将关闭所有正在运行的应用程序，因此，需要先构建
.exe，将其复制到控制台，然后再运行，这需要执行以下多个步骤：

1.  在 Visual Studio 中构建工具以生成 DumpTool.exe

2.  启动标题（例如，SimpleTriangle 示例）

3.  将 DumpTool.exe 复制到游戏操作系统分区

\> xbcp /x/title Gaming.Xbox.x64\\Layout\\Image\\Loose\\\*.exe
xd:\\DumpTool\\

\> xbcp /x/title Gaming.Xbox.x64\\Layout\\Image\\Loose\\\*.dll
xd:\\DumpTool\\

4.  运行此工具，收集 SimpleTriangle.exe 的会审转储

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:triage
SimpleTriangle.exe

5.  将 .dmp 文件复制回开发电脑进行调试

\> xbcp /x/title xd:\\SimpleTriangle.dmp

DumpTool 项目包含一个简单的批处理文件
runCommand.bat，此文件可自动执行前四个步骤，使测试代码更改变得容易。

## DumpTool 命令行

DumpTool 还支持一组丰富的命令行选项：

用法：DumpTool \[-mdt:\<小型转储类型\> \...\] \[-pdt:\<预定义类型\>\]
\<可执行文件名称\>

\<小型转储类型\>：Normal WithDataSegs WithFullMemory WithHandleData

FilterMemory ScanMemory WithUnloadedModules

WithIndirectlyReferencedMemory FilterModulePaths

WithProcessThreadData WithPrivateReadWriteMemory

WithoutOptionalData WithFullMemoryInfo WithThreadInfo

WithCodeSegs WithoutAuxiliaryState

WithFullAuxiliaryState WithPrivateWriteCopyMemory

IgnoreInaccessibleMemory WithTokenInformation

WithModuleHeaders FilterTriage

\<预定义类型\>：heap mini micro triage native

\<小型转储类型\> 与 MINIDUMP_TYPE 枚举的值相对应，而此枚举在
[GDNP](https://developer.xboxlive.com/en-us/platform/development/documentation/software/Pages/MINIDUMP_TYPE_typedef___dbghelp_Xbox_Microsoft_T_may17.aspx)
和
[MSDN](https://msdn.microsoft.com/en-us/library/windows/desktop/ms680519(v=vs.85).aspx)
上有记录。通过在命令行上指定 --mdt: 的多个实例，组合 MINIDUMP_TYPE
的不同值。请注意，有很多种可能！为了简化操作，该工具还提供了 --pdt
选项。

存在"预定义类型"(-pdt) 选项，用于简化通常需要使用 -mdt 选项单独提供的
MINIDUMP_TYPE 标志。预定义类型与 xbWatson.exe 支持的故障转储类型相对应：

![](./media/image3.png)

示例：

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:triage
SimpleTriangle.exe

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:Mini
SimpleTriangle.exe

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:Heap
SimpleTriangle.exe

请注意，该工具还提供"微转储"和"本机转储"。请查看源代码中与这些值对应的标志的确切组合。如果你不熟悉
MiniDumpWriteDump()，则从预定义的转储标志开始，然后根据需要试验其他标志。该工具应该方便地促进该实验，因为它可以同时允许
--pdt: 和 --mdt，还可以组合标志：

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe --pdt:micro
--mdt:WithHandleData

--mdt:WithUnloadedModules SimpleTriangle.exe

## 部署工具

如果计划在标题中使用
DumpTool（或某些变体），请考虑将该工具添加到游戏部署中，从而无需将其复制到游戏操作系统。此工具提供了一种便捷方式来生成故障转储，不会以任何其他方式破坏正在运行的标题。

# 实现说明

-   此外，还可以直接从可执行文件的代码中调用
    MiniDumpWriteDump()。例如，许多开发人员将此功能添加到其未处理的异常筛选器。下面是对
    MiniDumpWriteDump 的一个非常简单的示例调用。

> MiniDumpWriteDump(
>
> GetCurrentProcess(),
>
> GetProcessId(GetCurrentProcess()), hDumpFile, mdt, nullptr, nullptr,
> nullptr);

-   GSDK 还附带了轻量工具，名为
    [xbWatson](https://developer.xboxlive.com/en-us/platform/development/documentation/software/Pages/xbwatson_may17.aspx)，可用于捕获故障转储。DumpTool
    中的功能等同于 xbWatson
    中的"故障转储"功能。请注意，无需执行任何其他部署步骤即可使用
    xbWatson。

-   还可以使用 Visual Studio
    捕获故障转储。在"调试"菜单中查找"将转储另存为..."选项。请注意，此选项会在你附加到进程后出现，并且在你暂停（"全部中断"）时变为活动状态。

# 已知问题

在调用 MiniDumpWriteDump 之前，请确保同时使用 GENERIC_WRITE 和
GENERIC_READ 打开文件，否则生成的 .dmp 文件可能是损坏的。

# 更新历史记录

初始发布：2019 年 4 月。
