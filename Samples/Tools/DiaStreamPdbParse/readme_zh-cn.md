![](./media/image1.png)

# DiaStreamPdbParse 示例

*此示例可用于 Microsoft 游戏开发工具包（2022 年 3 月）*

# 说明

此示例演示如何使用 DIA（调试接口访问）SDK 解析游戏中的符号，尤其是从异常期间生成的调用堆栈解析符号。 它还演示如何使用 IStream 接口，控制从磁盘加载的 PDB 数据。 最后，它提供名为 msdia140-xbox.dll 的 DIA DLL 的 Xbox 版本。

Xbox 版本的 DIA 使用 HeapAlloc 创建返回给调用方的 BSTR 对象。 它们需要由调用方通过 HeapFree 释放，它是名为 DIAString 的帮助程序类，在 symbolLookup.cpp 中提供，负责在桌面和主机中执行正确操作。 使用 DIA API 时，DIAString 类可用作 BSTR 的直接替代。

Xbox 上对 BSTR 对象使用 HeapAlloc/HeapFree 是因为通过 SysAllocString/SysFreeString 的标准 BSTR 内存管理方法对游戏不可用。 DIA 查询函数将返回通过 SysAllocString 分配的 BSTR，但游戏将无法释放该内存。

Xbox 版本的 DIA 也不包含对主机上不存在的 Shlwapi.dll 的引用。

注意：你可以在零售和开发游戏环境中免费使用此示例附带的 DIA DLL 版本 (msdia140-xbox.dll)。 但是，Visual Studio 团队目前不支持 Xbox 版本。 如果遇到问题，请通过 Xbox 论坛联系以获取帮助。

# 生成示例

此示例支持以下平台

| 平台 | 说明 |
|---|---|
| Gaming.Desktop.x64 | 使用作为 Visual Studio 的一部分安装的 DIA 版本。 |
| Gaming.Scarlett.xbox.x64<br />Gaming.XboxOne.xbox.x64 | 在主机上使用 msdia140-xbox.dll。 它作为生成过程的一部分自动复制到主机。 |

*有关详细信息，请参阅* *GDK 文档中的*__运行示例__。&nbsp;

# 使用示例

该示例将自动捕获当前调用堆栈，解析每个函数的符号，并在屏幕上显示结果。

# 实现说明

symbolLookup.cpp 中的 DumpCallstack 函数是此示例的主要入口点。 它可能需要特定线程或对 `EXCEPTION_POINTERS` 对象的引用，以便调用堆栈进行分析。

在 symbolLookup.cpp 的顶部有几个定义，用于控制要收集的符号信息的数量和类型。 某些符号信息的收集成本可能很高，尤其是在 Xbox One 系列主机中的旋转驱动器上收集的信息。

PdbMemoryStream.cpp/h 包含 IPdbMemoryStream，它派生自 IStream。 此类提供给 DIA 以替换其默认 PDB 加载方法。 使用此类可以更精细地控制所使用的内存量以及读取模式，以最大程度地减少对游戏的影响。

# 更新历史记录

2022 年 3 月初始版本

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


