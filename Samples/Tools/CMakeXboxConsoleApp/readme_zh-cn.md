# Cmake Xbox 主机应用

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

这是使用 [CMake](https://cmake.org/) 跨平台生成系统生成&ldquo;Win32 控制台&rdquo;应用程序的示例，可使用 Microsoft GDK 在 Xbox 硬件上执行此应用程序。 它适用于使用&ldquo;printf&rdquo;样式输出的非图形开发人员单元测试。

![查看源图像](./media/image1.png)

*如果你在查找使用 CMake 构建标准 Microsoft GDK 应用程序的详细信息, 请参阅 **CMakeExample** 和 **CMakeGDKExample***。

# 生成示例 (Visual Studio)

使用 Visual Studio 2019 或 2022 从&ldquo;新建项目对话框&rdquo;或使用&ldquo;文件 -> 打开 -> 文件夹...&rdquo;菜单命令选择&ldquo;打开本地文件夹...&rdquo;，然后打开示例文件夹：

> 这要求你安装&ldquo;适用于 Windows 的 C++ CMake 工具&rdquo;组件(`Microsoft.VisualStudio.Component.VC.CMake.Project`)。

如果需要，请编辑 CMake **XdkEditionTarget** 变量（在 CMakePresets.json 或 CMakeList.txt 中）以确保引用了正确的 GDK 版本。

CMake 工具应在打开时自动生成缓存。 否则，请选择 CMakeList.txt 然后从右键菜单选择&ldquo;生成缓存&rdquo;。 然后使用&ldquo;生成 -> 全部重新生成&rdquo;菜单命令。 生成产品位于&ldquo;**Out**&rdquo;子文件夹中。

有关 Visual Studio 中的 CMake 的详细信息，请参阅 [Microsoft Docs](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio)。

默认设置包括定义为 CMake 预设的 **x64-Debug** 和 **x64-Release**，**x64-Clang-Debug** 和 **x64-Clang-Release** 配置。

> 这要求你安装&ldquo;适用于 Windows 的 C++ Clang 编译器&rdquo;(`Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Llvm.Clang`)组件。

*如果按 F5，则它会尝试在开发电脑而不是远程主机上运行，因此它可能会失败或可能不会失败。 你需要按照以下说明部署程序才能成功运行。*

# 生成示例（命令行）

还可以使用 *VS 2019 或 2022 开发人员命令提示* 从命令行生成和构建：

```
cd CMakeXboxConsoleApp
cmake -B out -DXdkEditionTarget=221000
cmake --build out
```


还提供了 CMake 预设

```
cmake --list-presets
cmake --preset=x64-Debug
cmake --build out\build\x64-Debug
```


# 使用示例

若要部署示例，请打开 *Xbox 游戏命令提示* 实例并更改为示例目录：

```
cd CMakeXboxConsoleApp\out\build\<config>
xbcp bin\Console\*.exe xd:\
xbcp bin\Console\*.dll xd:\
```


运行示例：

```
xbrun /O D:\CMakeXboxConsoleApp.exe
```


程序将在 System OS 的上下文中运行。

如果希望改为在 Game OS 的上下文中运行，可以使用类似的过程。 首先在目标主机上运行 Game OS 游戏。 一个很好的候选项是使用 Visual Studio 的新项目对话框并使用 Microsoft GDK 创建默认的&ldquo;Direct3D 12 Xbox 游戏项目&rdquo;。 生成并部署它，并使其保持运行状态。

然后使用：

```
xbcp /x/title bin\Console\*.exe xd:\
xbcp /x/title bin\Console\*.dll xd:\
```


运行示例：

```
xbrun /x/title /O D:\CMakeXboxConsoleApp.exe
```


请记住，此步骤可以通过将进程注入游戏 OS VM 来实现。 目前不支持多进程游戏，且多个组件（包括图形、音频、GameRuntime）未经过测试，也不支持在多个进程方案中使用。 让&ldquo;承载&rdquo;的游戏非常简单，并限制其对 CPU 资源的使用也是一个好主意。

# 实现详细信息

对于 PC 桌面设备，Win32 控制台 exe（即 /SUBSYSTEM：CONSOLE）的 **CMakeLists.txt** 如下所示：

```
cmake_minimum_required (VERSION 3.20)

project(CMakeExampleWindowsConsole LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} Main.cpp)

target_compile_definitions(${PROJECT_NAME} PRIVATE _CONSOLE _UNICODE UNICODE)

if(MVSC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /GR- /fp:fast)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /Zc:__cplusplus /Zc:inline)
endif()
```


对于 Xbox 硬件上的系统和游戏操作系统，我们必须使用一组不同的链接库，并确保我们不会选取任何不受支持的库。 我们还应启用适当的 API 分区以避免使用不受支持的 API，此示例可确保使用平台标头和库进行构建。

在 Xbox 硬件上运行的应用程序还需要提供所需的 Visual C++ 运行时 DLL，以及为调试生成的 ucrtbased.lib。

此示例中的 Xbox&ldquo;主机&rdquo;CMake 设置为将 EXE 生成到
由于无法将 Direct3D 用于主机应用，我们避免了平台中的主要 API 差异，并且可以合理地期望同一 EXE 在这两个平台上运行。 这也由特定的 XboxOne 设置，并且未在 CMakeLists.txt 中设置 Scarlett include/lib 路径。 如果需要，可以启用其他编译器 CPU 目标
| | |
|---|---|
|在 Xbox Series X|S 或 Xbox One 硬件上运行。 由于我们无法使用 |

生成选项 `OPTIMIZE_FOR_SCARLETT` 为打开。 生成的 EXE 将运行
为了演示这一点，该示例使用 **DirectXMath XMVerifyCPUSupport** 函数来执行相关的 CPUID 检查。
| | |
|---|---|
|专用于 Xbox Series X|S 硬件。 这是通过设置完成的|
|与之前在 Xbox Series X|S 上一样，但无法在Xbox One上运行。 收件人|

# 其他信息

有关本示例中使用的所有编译器和链接器开关的详细信息，请参阅 **CMakeExample**。

此示例中的 CMake 项目支持选择加入生成选项，以使用生成并/并不安装 (BWOI)。 如果启用，则需要 ExtractedFolder 环境变量，该变量指向由 *BWOIExample* 的 `extractgdk.cmd` 脚本创建的提取的 Microsoft GDK。 也可以选择使用提取的 Windows SDK。 CMake 项目不需要 vctargets.cmd 脚本的结果，因为它不使用 Gaming.*.x64 MSBuild 平台。

若要启用此生成选项，请使用 CMakeSettings.json 设置为 `BUILD_USING_BWOI` True。 或者，如果使用命令行生成，则添加 `-DBUILD_USING_BWOI=ON` 到生成步骤。

查看 **BWOIExample** 了解更多详情。

# 已知问题

如果使用 clang/LLVM 工具集，请确保使用的是 Windows 10 SDK （19041） 或更高版本。 在 DirectXMath 3.13 及更早版本中，XMVerifyCPUSupport 实现未为该工具集正确生成。 请参阅<https://walbourn.github.io/directxmath-3.14/>了解详细信息。

# 版本历史记录

| 日期 | 说明 |
|---|---|
| 2020 年 5 月 | 初始版本。 |
| 2020 年 6 月 | 已针对 2020 年 6 月 GDK FAL 版本进行了更新。 |
| 2020 年 11 月 | 清理了 CMake 文件，添加了_CONSOLE 定义。 |
| 2021 年 2 月 | 次要 CMake 清理。 |
| 2021 年 8 月 | 工具链文件的改进。 |
| 2021 年 10 月 | BWOI 的更新。 |
| 2022 年 1 月 | 添加了 VS 2022 支持。<br />CMake 清理并添加了 CMake 预设文件。 |
| 2022 年 10 月 | 删除了 VS 2017 支持。 |
| 2023 年 2 月 | 删除了自定义生成选项BUILD_FOR_LTCG，以支持 CMake 3.9 的标准CMAKE_INTERPROCEDURAL_OPTIMIZATION。<br />更新了新的 VS 2022 17.5 交换机。 |
| 2023 年 3 月 | 已更新为为 Playfab.Services.C 扩展库添加新目标。 |
| 2023 年 6 月 | Xbox One游戏需要与 VS 2022 或更高版本一起使用 `/d2vzeroupper-` ，因为默认行为已从 VS 2019 翻转 |
| 2023 年 10 月 | Microsoft GDK 需要 Windows 11 SDK (22000) 或更高版本。 |


