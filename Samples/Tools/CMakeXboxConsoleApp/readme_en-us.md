# Cmake Xbox Console App

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This is an example of using the [CMake](https://cmake.org/)
cross-platform build system to produce a "Win32 console" application
that can be executed on the Xbox hardware using the Microsoft GDK. This
is suitable for non-graphics developer unit tests using 'printf' style
output.

![See the source image](./media/image1.png)

*If you are looking for details on using CMake for building standard
Microsoft GDK applications, see **CMakeExample** and
**CMakeGDKExample***.

# Building the sample (Visual Studio)

Using Visual Studio 2019 or 2022 select "Open a local folder..." from
the New Project Dialog or the "File -\> Open -\> Folder..." menu command
and open the sample folder:

> This requires that you have the "C++ CMake tools for Windows" component (`Microsoft.VisualStudio.Component.VC.CMake.Project`) installed.

If needed, edit the CMake **XdkEditionTarget** variable (either in the
CMakePresets.json or CMakeList.txt) to ensure you have the correct GDK
edition referenced.

The CMake tool should generate the cache automatically upon opening.
Otherwise select the CMakeList.txt and select "Generate Cache" from the
right-button menu. Then use the "Build -\> Rebuild All" menu command.
The build products are in the "**out**" subfolder.

See [Microsoft
Docs](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio)
for more information on CMake in Visual Studio.

The default setup includes the **x64-Debug** and **x64-Release**,
**x64-Clang-Debug** and **x64-Clang-Release** configurations defined as
CMake Presets.

> This requires that you have the "C++ Clang Compiler for Windows" (`Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Llvm.Clang`) component installed.

*If you press F5, it is attempting to run on the development PC and not
the remote console so it may or may not fail. You need to deploy the
program per the instructions below to run successfully.*

# Building the sample (command-line)

You can also generate and build from the command line using a *VS 2019
or 2022 Developer Command Prompt*:

```
cd CMakeXboxConsoleApp
cmake -B out -DXdkEditionTarget=221000
cmake --build out
```

CMake Presets are also provided

```
cmake --list-presets
cmake --preset=x64-Debug
cmake --build out\build\x64-Debug
```

# Using the sample

To deploy the sample, open an *Xbox Gaming Command Prompt* instance and
change to the sample directory:

```
cd CMakeXboxConsoleApp\out\build\<config>
xbcp bin\Console\*.exe xd:\
xbcp bin\Console\*.dll xd:\
```

To run the sample:

```
xbrun /O D:\CMakeXboxConsoleApp.exe
```

The program will run in the context of the System OS.

If you wish to run in the context of the Game OS instead, you can use a
similar procedure. First start by running a Game OS title on the target
console. A good candidate is to use Visual Studio's New Project dialog
and create a default "Direct3D 12 Xbox Game project" with the Microsoft
GDK. Build and deploy it, and leave it running.

Then use:

```
xbcp /x/title bin\Console\*.exe xd:\
xbcp /x/title bin\Console\*.dll xd:\
```

To run the sample:

```
xbrun /x/title /O D:\CMakeXboxConsoleApp.exe
```

Keep in mind this works by injecting a process into the Game OS VM.
Multi-process game titles are not supported at this time, and several
components including graphics, audio, and GameRuntime are not tested nor
support for use in multiple process scenarios. It is also a good idea to
keep the 'hosting' title very simple and limit its use of CPU resources.

# Implementation Details

For PC Desktop, the **CMakeLists.txt** for a Win32 console exe (i.e.
/SUBSYSTEM:CONSOLE) would be something like the following:

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

For the System and Game OS on the Xbox hardware, we must use a different
set of link libraries and ensure we don't pick up any unsupported
libraries. We should also enable the proper API partitioning to avoid
using unsupported APIs, and this sample ensure you are building with the
platform headers and libraries.

The application running on the Xbox hardware also needs to provide the
Visual C++ Runtime DLLs it requires, as well as the ucrtbased.lib if
it's built for Debug.

The Xbox "console" CMake in this sample is set up to build the EXE to
run on either Xbox Series X|S or Xbox One hardware. Since we can't use
Direct3D for a console app, we avoid the major API differences in the
platforms and can reasonably expect the same EXE to run on both
platforms. This is also by the specific XboxOne and Scarlett include/lib
paths are not set in the CMakeLists.txt.

If desired, you can enable additional compiler CPU targeting
specifically for the Xbox Series X|S hardware. This is done by setting
the build option `OPTIMIZE_FOR_SCARLETT` to ON. The resulting EXE will run
as before on Xbox Series X|S, but will fail to run on Xbox One. To
demonstrate this, the sample makes use of the DirectXMath
**XMVerifyCPUSupport** function which performs he relevant CPUID checks.

# Additional Information

For more details on all the complier & linker switches used in this
example, see **CMakeExample**.

The CMake project in this sample supports an opt-in build option to use
Build With/Out Installing (BWOI). If enabled, it requires an
ExtractedFolder environment variable which points to the extracted
Microsoft GDK created by the *BWOIExample*'s `extractgdk.cmd` script. It
can optionally also have an extracted Windows 10 SDK (19041) for the May
2020 GDK or later. The CMake project does not need the results of the
vctargets.cmd script because it doesn't use the Gaming.\*.x64 MSBuild
platforms.

To enable this build option, set `BUILD_USING_BWOI` to True using
CMakeSettings.json. Alternately, if building with the command line, add
`-DBUILD_USING_BWOI=ON` to the generation step.

See the **BWOIExample** for more details.

# Known Issues

If you use the clang/LLVM toolset, be sure you are using the Windows 10
SDK (19041) which includes DirectXMath 3.14. In DirectXMath 3.13 and
before, the XMVerifyCPUSupport implementation doesn't build correctly
for that toolset. See <https://walbourn.github.io/directxmath-3.14/> for
details.

# Version History

|Date|Notes|
|---|---|
|May 2020|Initial version.|
|June 2020|Updated for the June 2020 GDK FAL release.|
|November 2020|Cleaned up CMake files, added _CONSOLE define.|
|February 2021|Minor CMake cleanup.|
|August 2021|Improvements to toolchain files.|
|October 2021|Updates for BWOI.|
|January 2022|Added VS 2022 support.<br />CMake cleanup and added CMake Presets file.|
|October 2022|Removed VS 2017 support.|
|February 2023|Removed custom build option BUILD_FOR_LTCG in favor of CMake 3.9’s standard CMAKE_INTERPROCEDURAL_OPTIMIZATION.<br />Updated for new VS 2022 17.5 switches.|
|March 2023|Updated to add new target for Playfab.Services.C extension library.|
|June 2023|Xbox One titles need to use `/d2vzeroupper-` with VS 2022 or later as the default behavior has flipped from VS 2019|
