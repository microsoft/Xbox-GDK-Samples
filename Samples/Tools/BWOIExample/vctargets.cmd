@echo off
REM Build With/Out Installing (BWOI) Example
REM .
REM Sets up the VCTargets folders needed for VS 2019 BWOI
REM .

if %XDKEditionTarget%.==. goto :missingxdktarg
if %ExtractedFolder%.==. goto :missingextracted
if "%VSInstallDir%."=="." goto :missingvsinstalldir

if NOT EXIST "%ExtractedFolder%\Microsoft GDK" goto :gdkmissing
if NOT EXIST "%VSInstallDir%\MSBuild" goto :missingvsinstalldir
if NOT EXIST "%VSInstallDir%\MSBuild\Microsoft\VC\v160\Microsoft.cpp.Default.props" goto :missingvsinstalldir

if NOT EXIST "%ExtractedFolder%\VCTargets150" md "%ExtractedFolder%\VCTargets150"
if NOT EXIST "%ExtractedFolder%\VCTargets160" md "%ExtractedFolder%\VCTargets160"

echo Set up VS 2019 160 VC Targets (Platform Toolset v142)

robocopy /NJH /NJS /NDL /NC /NS /S "%VSInstallDir%\MSBuild\Microsoft\VC\v160" %ExtractedFolder%\VCTargets160
robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GRDK\VS2019\flatDeployment\MSBuild\Microsoft\VC\v160" "%ExtractedFolder%\VCTargets160"
robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GXDK\VS2019\flatDeployment\MSBuild\Microsoft\VC\v160" "%ExtractedFolder%\VCTargets160"

if NOT EXIST "%VSInstallDir%\MSBuild\Microsoft\VC\v150\Microsoft.Cpp.Default.props" goto :skipv150

echo Set up VS 2019 150 VC Targets (Platform Toolset v141)

robocopy /NJH /NJS /NDL /NC /NS /S "%VSInstallDir%\MSBuild\Microsoft\VC\v150" %ExtractedFolder%\VCTargets150
robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GRDK\VS2019\flatDeployment\MSBuild\Microsoft\VC\v150" "%ExtractedFolder%\VCTargets150"
robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GXDK\VS2019\flatDeployment\MSBuild\Microsoft\VC\v150" "%ExtractedFolder%\VCTargets150"

goto :eof

:missingxdktarg
echo vctargets requires the XDKEditionTarget environment variable set by setenv.cmd
:missingextracted
echo vctargets requires the ExtractedFolder environment variable set by setenv.cmd
exit /b 1

:gdkmissing
echo ExtractedFolder does not contain the extracted GDK
exit /b 1

:missingvsinstalldir
echo Script needs the VSInstallDir set to VS 2019's install directory
exit /b 1

:skipv150
echo WARNING: MSVC v141 C++ build tools for x64/x86 component not found; skipping support for v141 platform toolsets
exit /b 0
