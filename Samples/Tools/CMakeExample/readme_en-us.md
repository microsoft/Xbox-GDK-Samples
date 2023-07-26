# CMake Example

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This is an example of using the [CMake](https://cmake.org/)
cross-platform build system to build an executable with the Microsoft
Game Development Kit via the Ninja generator.

![See the source image](./media/image1.png)

*The primary purpose of this sample is to clearly document all the paths
and settings required to build for the Gaming.\*.x64 platforms. This
replicates much of the functionality that is implemented in the MSBuild
rules installed by the GDK. For an alternative method of utilizing CMake
via the Visual Studio generator, see **CMakeGDKExample***.

# Building the sample (Visual Studio)

Using Visual Studio 2019 (16.11) or Visual Studio 2022 select "Open a
local folder..." from the New Project Dialog or the "File -\> Open -\>
Folder..." menu command and open the Desktop, XboxOne, or Scarlett
folder.

> This requires that you have the "C++ CMake tools for Windows" component (`Microsoft.VisualStudio.Component.VC.CMake.Project`) installed.

If needed, edit the **XdkEditionTarget** variable (either in the
CMakePresets.json or CMakeList.txt) to ensure you have the correct GDK
edition referenced.

The CMake tool should generate the cache automatically upon opening.
Otherwise select the CMakeList.txt and select "Generate Cache" from the
right-button menu. Then use the "Build -\> Rebuild All" menu command.
The build products are in the "**out**" subfolder.

See [Microsoft
Docs](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio)
for more information on CMake in Visual Studio.

The default setup includes the **x64-Debug**, **x64-Release**,
**x64-Clang-Debug**, and **x64-Clang-Release** configurations to use
clang/LLVM instead.

> This requires that you have the "C++ Clang Compiler for Windows" ()`Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Llvm.Clang`) component installed.

*If you press F5 for the Xbox One or Xbox Series X|S projects, it is
attempting to run on the development PC and not the remote console so it
will fail. You need to deploy the program per the instructions below to
run successfully.*

# Building the sample (command-line)

You can also generate and build from the command line using a *VS 2019
or 2022 Developer Command Prompt*. For a complete list of available
presets, use:

```
cd CMakeExample\Desktop
cmake --list-presets

cd CMakeExample\Scarlett
cmake --list-presets

cd CMakeExample\XboxOne
cmake --list-presets
```

In each case change the appropriate target platform and use:

```
cmake --preset=x64-Debug
cmake --build out\build\x64-Debug
```

## Troubleshooting

*You may need to edit the CMakePresets.json to match your GDK edition.*

*Verify you are using CMake 3.20 or later with:*

```
cmake --version
```

# Using the sample

To deploy the sample, open an *Xbox Gaming Command Prompt* instance and
change to the sample directory for your target:

```
cd CMakeExample\Desktop\out\build\x64-Debug\bin

cd CMakeExample\Scarlett\out\build\x64-Debug\bin

cd CMakeExample\XboxOne\out\build\x64-Debug\bin
```

## Targeting Desktop

To run the 'loose layout', use:

```
cd bin\x64
CMakeExampleDesktop.exe
```

## Targeting Xbox One or Xbox Series X|S

### Push deploy

To do push deploy the 'loose' layout:

```
xbapp deploy Gaming.Xbox.Scarlett.x64
```

-or-

```
xbapp deploy Gaming.Xbox.XboxOne.x64
```

### Run-from-PC

To run the 'loose' layout from the PC:

```
xbapp launch Gaming.Xbox.Scarlett.x64\CMakeExampleScarlett.exe
```

-or-

```
xbapp launch Gaming.Xbox.XboxOne.x64\CMakeExampleXboxOne.exe
```

## Packaged deployment

To create a package:

```
makepkg genmap /f chunks.xml /d x64
makepkg pack /f chunks.xml /lt /d x64 /pd . /pc
```

-or-

```
makepkg genmap /f chunks.xml /d Gaming.Xbox.Scarlett.x64
makepkg pack /f chunks.xml /lt /d Gaming.Xbox.Scarlett.x64 /pd .
```

-or-

```
makepkg genmap /f chunks.xml /d Gaming.Xbox.XboxOne.x64
makepkg pack /f chunks.xml /lt /d Gaming.Xbox.XboxOne.x64 /pd .
```

Then install the resulting package to your console (the exact .xvc
filename will vary):

```
xbapp install CMakeExampleXboxOne_1.0.0.0_neutral__zjr0dfhgjwvde.xvc
```

For Desktop, the extension is ".msixvc" (the exact filename will vary):

```
xbapp install CMakeExampleDesktop_1.0.0.0_x64__8wekyb3d8bbwe.msixvc
```

The sample when run creates a device and swapchain, and draws a colored
triangle. It has no controls or other behavior.

![C:\\temp\\xbox_screenshot.png](./media/image2.png)

*For packaging the other versions, see comments at the end of each
CMakeLIst.txt for the specific command-line options to use.*

# Implementation Details

For more information on various Visual C++ switches, see the links
below:

| /GR | <https://docs.microsoft.com/en-us/cpp/build/reference/gr-enable-run-time-type-information> |
|-------------|--------------------------------------------------------|
| /GS<br /> /RTC<br /> /sdl<br /> /DYNAMICBASE<br /> /NXCOMPAT | <https://aka.ms/msvcsecurity> |
| /DEBUG:fastlink |  <https://devblogs.microsoft.com/cppblog/faster-c-build-cycle-in-vs-15-with-debugfastlink/> |
| /EHsc  | <https://devblogs.microsoft.com/cppblog/making-cpp-exception-handling-smaller-x64/> |
| /fp  | <https://docs.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior><br /> <https://devblogs.microsoft.com/cppblog/game-performance-improvements-in-visual-studio-2019-version-16-2/> |
| /FS  |  <https://docs.microsoft.com/en-us/cpp/build/reference/fs-force-synchronous-pdb-writes> |
| /GL<br /> /Gw<br /> /LTCG | <https://devblogs.microsoft.com/cppblog/tag/link-time-code-generation/><br /> <https://devblogs.microsoft.com/cppblog/introducing-gw-compiler-switch/> |
| /Gy  |  <https://docs.microsoft.com/en-us/cpp/build/reference/gy-enable-function-level-linking> |
| /JMC  | <https://devblogs.microsoft.com/cppblog/announcing-jmc-stepping-in-visual-studio/> |
| / permissive- | <https://devblogs.microsoft.com/cppblog/permissive-switch/> |
| /std:c++14  | <https://devblogs.microsoft.com/cppblog/standards-version-switches-in-the-compiler/> |
| /Yc<br /> /Yu<br /> /Fp<br /> /FI |  <https://docs.microsoft.com/en-us/cpp/build/creating-precompiled-header-files> <https://devblogs.microsoft.com/cppblog/shared-pch-usage-sample-in-visual-studio/> |
| /Zc:\ _\_cplusplus |  <https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/> |
| /Zc: preprocessor  | <https://devblogs.microsoft.com/cppblog/announcing-full-support-for-a-c-c-conformant-preprocessor-in-msvc/> |
| /Z7<br /> /Zi<br /> /ZI | <https://docs.microsoft.com/en-us/cpp/build/reference/z7-zi-zi-debug-information-format> |
| /ZH:SHA_256  | <https://learn.microsoft.com/en-us/cpp/build/reference/zh> |
| /guard:cf<br /> /guard:ehcont<br /> /CETCOMPAT  | <https://learn.microsoft.com/en-us/cpp/build/reference/guard-enable-control-flow-guard><br /> <https://learn.microsoft.com/en-us/cpp/build/reference/guard-enable-eh-continuation-metadata><br /> <https://learn.microsoft.com/en-us/cpp/build/reference/cetcompat> |

Note that
[/Gm](https://docs.microsoft.com/en-us/cpp/build/reference/gm-enable-minimal-rebuild)
(Minimal Rebuild) is deprecated and should be removed from projects that
still use it.

# Additional Information

The CMake projects in this sample support an opt-in build option to use
Build With/Out Installing (BWOI). If enabled, it requires an
ExtractedFolder environment variable which points to the extracted
Microsoft GDK created by the *BWOIExample*'s extractgdk.cmd script. It
can optionally also have an extracted Windows 10 SDK (19041) for the May
2020 GDK or later. The CMake projects do not need the results of the
vctargets.cmd script because they don't use the Gaming.\*.x64 MSBuild
platforms.

To enable this build option, set `BUILD_USING_BWOI` to True. Alternately,
if building with the command line, add `-DBUILD_USING_BWOI=ON` to the
generation step.

See the **BWOIExample** for more details.

# Version History

|Date|Notes|
|---|---|
|November 2019|Initial version.|
|February 2020|Added use of HLSL shaders to the example.<br /> Updated to optionally support BWOI.|
|April 2020|Updated with pch support when using CMake 3.16 or later.|
|May 2020|Updated to support the May 2020 GDK.|
|June 2020|Updated for the June 2020 GDK FAL release.|
|August 2020|Updated with side-by-side details.|
|November 2020|Added xmem.lib and xg_*.lib for Xbox targets.<br /> Cleaned up CMake files.|
|February 2021|Minor updates for CMake comments.|
|April 2021|Add appnotify.lib to resolve link issues with Desktop target.<br />Add LargeLogo.png.|
|June 2021|General code cleanup.|
|August 2021|Improvements for toolchain files.|
|October 2021|Updates for BWOI.|
|January 2022|Added VS 2022 support.<br /> CMake cleanup and added Presets file.|
|October 2022|Removed VS 2017 support.<br />XSAPI requires XCurl; Added commented out support for all other Extension Libraries<br />Made Desktop Cmake compatible with GDK for PC.|
|November 2022|Requires March 2022 GDK or later.<br /> Updated to require CMake 3.20 now that VS 2019 16.10 and earlier are out of their support lifecycle.|
|December 2022|Simplified Desktop scenario to use the ‘x64’ platform style rather than the custom ‘Gaming.Desktop.x64’<br />Reorganized .cmake files into their own subfolder.|
|February 2023|Removed custom build option BUILD_FOR_LTCG in favor of CMake 3.9’s standard `CMAKE_INTERPROCEDURAL_OPTIMIZATION`.<br /> Updated for new VS 2022 17.5 switches.|
|March 2023|Updated to add new target for Playfab.Services.C extension library.|
|June 2023|Xbox One titles need to use `/d2vzeroupper-` with VS 2022 or later as the default behavior has flipped from VS 2019|
