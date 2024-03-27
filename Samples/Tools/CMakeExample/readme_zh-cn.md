# CMake 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

这是使用 [CMake](https://cmake.org/) 跨平台生成系统通过 Ninja 生成器使用 Microsoft 游戏开发工具包生成可执行文件的示例。

![查看源图像](./media/image1.png)

*此样本的主要目的是清楚地记录为 Gaming.\*.x64 平台生成所需的所有路径和设置。 这将复制 GDK 所安装 MSBuild 规则中实现的大部分功能。 有关通过 Visual Studio 生成器利用 CMake 的替代方法，请参阅 **CMakeGDKExample***.

# 生成样本 (Visual Studio)

使用 Visual Studio 2019 (16.11) 或 Visual Studio 2022 选择&ldquo;打开本地文件夹...&rdquo;从&ldquo;新建项目&rdquo;对话框或&ldquo;文件 -\> 打开 -\> 文件夹...&rdquo;菜单命令并打开&ldquo;桌面&rdquo;、&ldquo;XboxOne&rdquo;或&ldquo;Scarlett&rdquo;文件夹。

> 这要求安装&ldquo;适用于 Windows 的 C++ CMake 工具&rdquo;组件 (`Microsoft.VisualStudio.Component.VC.CMake.Project`)。

如果需要，请编辑 **XdkEditionTarget** 变量（在 CMakePresets.json 或 CMakeList.txt 中），以确保引用了正确的 GDK 版本。

CMake 工具应在打开时自动生成缓存。 否则，请选择 CMakeList.txt 然后从右键菜单选择&ldquo;生成缓存&rdquo;。 然后使用&ldquo;生成 -> 全部重新生成&rdquo;菜单命令。 生成产品位于&ldquo;**Out**&rdquo;子文件夹中。

有关 Visual Studio 中的 CMake 的详细信息，请参阅 [Microsoft Docs](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio)。

默认设置包括 **x64-Debug**、**x64-Release**、**x64-Clang-Debug** 和 **x64-Clang-Release** 配置，以改用 clang/LLVM。

> 这需要安装&ldquo;适用于 Windows 的 C++ Clang 编译器&rdquo;() `Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Llvm.Clang`) 组件。

尝试在开发电脑而不是远程主机上运行，因此会失败。 需要按照以下说明部署程序才能成功运行。*
| | |
|---|---|
|*如果为 Xbox One 或 Xbox Series X|S 项目按 F5，则为|

# 生成样本（命令行）

还可以使用 *VS 2019 或 2022 开发人员命令提示符*从命令行生成和构建。 有关可用预设的完整列表，请使用：

```
cd CMakeExample\Desktop
cmake --list-presets

cd CMakeExample\Scarlett
cmake --list-presets

cd CMakeExample\XboxOne
cmake --list-presets
```


在每种情况下，请更改相应的目标平台并使用：

```
cmake --preset=x64-Debug
cmake --build out\build\x64-Debug
```


## 故障排除

*可能需要编辑 CMakePresets.json 以匹配 GDK 版本。*

*验证是否正在使用 CMake 3.20 或更高版本：*

```
cmake --version
```


# 使用示例

若要部署样本，请打开 *Xbox 游戏命令提示符*实例，并更改为目标的样本目录：

```
cd CMakeExample\Desktop\out\build\x64-Debug\bin

cd CMakeExample\Scarlett\out\build\x64-Debug\bin

cd CMakeExample\XboxOne\out\build\x64-Debug\bin
```


## 面向桌面

若要运行&ldquo;松散布局&rdquo;，请使用：

```
cd bin\x64
CMakeExampleDesktop.exe
```


## 面向 Xbox One 或 Xbox Series X|S

### 推送部署

若要推送部署，请使用&ldquo;松散&rdquo;布局：

```
xbapp deploy Gaming.Xbox.Scarlett.x64
```


-或-

```
xbapp deploy Gaming.Xbox.XboxOne.x64
```


### 从个人电脑运行

若要从个人电脑运行&ldquo;松散&rdquo;布局，请执行以下操作：

```
xbapp launch Gaming.Xbox.Scarlett.x64\CMakeExampleScarlett.exe
```


-或-

```
xbapp launch Gaming.Xbox.XboxOne.x64\CMakeExampleXboxOne.exe
```


## 打包部署

如果要创建包，请执行以下操作：

```
makepkg genmap /f chunks.xml /d x64
makepkg pack /f chunks.xml /lt /d x64 /pd . /pc
```


-或-

```
makepkg genmap /f chunks.xml /d Gaming.Xbox.Scarlett.x64
makepkg pack /f chunks.xml /lt /d Gaming.Xbox.Scarlett.x64 /pd .
```


-或-

```
makepkg genmap /f chunks.xml /d Gaming.Xbox.XboxOne.x64
makepkg pack /f chunks.xml /lt /d Gaming.Xbox.XboxOne.x64 /pd .
```


然后将生成的包安装到主机（确切的 .xvc 文件名将会有所不同）：

```
xbapp install CMakeExampleXboxOne_1.0.0.0_neutral__zjr0dfhgjwvde.xvc
```


对于桌面，扩展名为&ldquo;.msixvc&rdquo;（确切的文件名会有所不同）：

```
xbapp install CMakeExampleDesktop_1.0.0.0_x64__8wekyb3d8bbwe.msixvc
```


运行时的样本创建设备和交换链，并绘制彩色三角形。 它没有控制或其他行为。

![C:\\temp\\xbox_screenshot.png](./media/image2.png)

*如果要打包其他版本，请参阅每个 CMakeLIst.txt 末尾的注释，了解要使用的特定命令行选项。*

# 实现详细信息

有关各种 Visual C++ 开关的详细信息，请参阅以下链接：

| /GR | <https://docs.microsoft.com/en-us/cpp/build/reference/gr-enable-run-time-type-information> |
|---|---|
| /GS<br /> /RTC<br /> /sdl<br /> /DYNAMICBASE<br /> /NXCOMPAT | <https://aka.ms/msvcsecurity> |
| /DEBUG:fastlink | <https://devblogs.microsoft.com/cppblog/faster-c-build-cycle-in-vs-15-with-debugfastlink/> |
| /EHsc | <https://devblogs.microsoft.com/cppblog/making-cpp-exception-handling-smaller-x64/> |
| /fp | <https://docs.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior><br /> <https://devblogs.microsoft.com/cppblog/game-performance-improvements-in-visual-studio-2019-version-16-2/> |
| /FS | <https://docs.microsoft.com/en-us/cpp/build/reference/fs-force-synchronous-pdb-writes> |
| /GL<br /> /Gw<br /> /LTCG | <https://devblogs.microsoft.com/cppblog/tag/link-time-code-generation/><br /> <https://devblogs.microsoft.com/cppblog/introducing-gw-compiler-switch/> |
| /Gy | <https://docs.microsoft.com/en-us/cpp/build/reference/gy-enable-function-level-linking> |
| /JMC | <https://devblogs.microsoft.com/cppblog/announcing-jmc-stepping-in-visual-studio/> |
| / permissive- | <https://devblogs.microsoft.com/cppblog/permissive-switch/> |
| /std:c++14 | <https://devblogs.microsoft.com/cppblog/standards-version-switches-in-the-compiler/> |
| /Yc<br /> /Yu<br /> /Fp<br /> /FI | <https://docs.microsoft.com/en-us/cpp/build/creating-precompiled-header-files> <https://devblogs.microsoft.com/cppblog/shared-pch-usage-sample-in-visual-studio/> |
| /Zc：\ _\_cplusplus | <https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/> |
| /Zc: 预处理器 | <https://devblogs.microsoft.com/cppblog/announcing-full-support-for-a-c-c-conformant-preprocessor-in-msvc/> |
| /Z7<br /> /Zi<br /> /ZI | <https://docs.microsoft.com/en-us/cpp/build/reference/z7-zi-zi-debug-information-format> |
| /ZH:SHA_256 | <https://learn.microsoft.com/en-us/cpp/build/reference/zh> |
| /guard:cf<br /> /guard:ehcont<br /> /CETCOMPAT | <https://learn.microsoft.com/en-us/cpp/build/reference/guard-enable-control-flow-guard><br /> <https://learn.microsoft.com/en-us/cpp/build/reference/guard-enable-eh-continuation-metadata><br /> <https://learn.microsoft.com/en-us/cpp/build/reference/cetcompat> |

请注意：
[/Gm](https://docs.microsoft.com/en-us/cpp/build/reference/gm-enable-minimal-rebuild)
已弃用（最小重新生成），应从仍使用它的项目中将其删除。

# 其他信息

此样本中的 CMake 项目支持选择加入生成选项，以使用未安装的生成 (BWOI)。 如果启用，则需要 ExtractedFolder 环境变量，该变量指向 *由 BWOIExample* 的 extractgdk.cmd 脚本创建的已提取 Microsoft GDK。 还可以选择具有已提取的 Windows SDK。 CMake 项目不需要 vctargets.cmd 脚本的结果，因为它们不使用 Gaming.\*.x64 MSBuild 平台。

若要启用此生成选项，请将 `BUILD_USING_BWOI` 设置为 True。 或者，如果使用命令行生成，请将 `-DBUILD_USING_BWOI=ON` 添加到生成步骤。

有关更多详细信息，请参阅 **BWOIExample**。

# 版本历史记录

| 2023 年 2 月 | 已删除自定义生成选项，BUILD_FOR_LTCG 支持 CMake 3.9 的标准 `CMAKE_INTERPROCEDURAL_OPTIMIZATION`。<br /> 已针对新的 VS 2022 17.5 交换机进行更新。 |
| 日期 | 说明 |
|---|---|
| 2019 年 11 月 | 初始版本。 |
| 2020 年 2 月 | 已向样本添加 HLSL 着色器的使用。<br /> 已更新为选择性支持 BWOI。 |
| 2020 年 4 月 | 使用 CMake 3.16 或更高版本时已更新 pch 支持。 |
| 2020 年 5 月 | 已更新以支持 2020 年 5 月 GDK。 |
| 2020 年 6 月 | 已针对 2020 年 6 月 GDK FAL 版本进行更新。 |
| 2020 年 8 月 | 已更新并行详细信息。 |
| 2020 年 11 月 | 已向 Xbox 目标添加 xmem.lib 和 xg_*.lib。<br /> 已清理 CMake 文件。 |
| 2021 年 2 月 | CMake 注释的次要更新。 |
| 2021 年 4 月 | 添加 appnotify.lib 以解决桌面目标的连接问题。<br />添加 LargeLogo.png。 |
| 2021 年 6 月 | 常规代码清理。 |
| 2021 年 8 月 | 对工具链文件的改进。 |
| 2021 年 10 月 | BWOI 的更新。 |
| 2022 年 1 月 | 已添加 VS 2022 支持。<br /> CMake 清理并添加了预设文件。 |
| 2022 年 10 月 | 已删除 VS 2017 支持。<br />XSAPI 需要 XCurl;已添加对所有其他扩展库的注释外支持<br />，使桌面 Cmake 与 PC 版 GDK 兼容。 |
| 2022 年 11 月 | 需要 2022 年 3 月 GDK 或更高版本。<br /> 已更新为要求 CMake 3.20，现在 VS 2019 16.10 及更早版本已超过其支持生命周期。 |
| 2022 年 12 月 | 简化的桌面方案，使用&ldquo;x64&rdquo;平台样式，而不是自定义&ldquo;Gaming.Desktop.x64&rdquo;<br />将 .cmake 文件重新组织到其自己的子文件夹中。 |
| 2023 年 3 月 | 已更新为为 Playfab.Services.C 扩展库添加新目标。 |
| 2023 年 6 月 | Xbox One 游戏需要与 VS 2022 或更高版本一起使用 `/d2vzeroupper-`，因为默认行为已从 VS 2019 翻转。 |
| 2023 年 10 月 | Microsoft GDK 需要 Windows 11 SDK (22000) 或更高版本。 |


