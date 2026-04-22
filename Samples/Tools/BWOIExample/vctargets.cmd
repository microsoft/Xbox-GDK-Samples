@echo off
REM Build With/Out Installing (BWOI) Example
REM .
REM Sets up the VCTargets folders needed for VS 2022 or 2026 BWOI
REM .

if %XDKEditionTarget%.==. goto :missingxdktarg
if %ExtractedFolder%.==. goto :missingextracted
if "%VSInstallDir%."=="." goto :missingvsinstalldir

if NOT EXIST "%ExtractedFolder%\Microsoft GDK" goto :gdkmissing
if NOT EXIST "%VSInstallDir%\MSBuild" goto :missingvsinstalldir

:define_vs_2026_bwoi

if NOT EXIST "%VSInstallDir%\MSBuild\Microsoft\VC\v180\Microsoft.cpp.Default.props" goto :define_vs_2022_bwoi

if NOT EXIST "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2026" goto :needsvs2026gdk

if NOT EXIST "%ExtractedFolder%\VCTargets160" md "%ExtractedFolder%\VCTargets160"
if NOT EXIST "%ExtractedFolder%\VCTargets170" md "%ExtractedFolder%\VCTargets170"
if NOT EXIST "%ExtractedFolder%\VCTargets180" md "%ExtractedFolder%\VCTargets180"

echo Set up VS 2026 180 VC Targets (Platform Toolset v145)

robocopy /NJH /NJS /NDL /NC /NS /S "%VSInstallDir%\MSBuild\Microsoft\VC\v180" %ExtractedFolder%\VCTargets180
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2026\pc\v180" "%ExtractedFolder%\VCTargets180"
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2026\gen9\v180" "%ExtractedFolder%\VCTargets180"
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2026\gen8\v180" "%ExtractedFolder%\VCTargets180"

if NOT EXIST "%VSInstallDir%\MSBuild\Microsoft\VC\v170\Microsoft.Cpp.Default.props" goto :eof

echo Set up VS 2026 170 VC Targets (Platform Toolset v143)

robocopy /NJH /NJS /NDL /NC /NS /S "%VSInstallDir%\MSBuild\Microsoft\VC\v170" %ExtractedFolder%\VCTargets170
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2026\pc\v170" "%ExtractedFolder%\VCTargets170"
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2026\gen9\v170" "%ExtractedFolder%\VCTargets170"
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2026\gen8\v170" "%ExtractedFolder%\VCTargets170"

if NOT EXIST "%VSInstallDir%\MSBuild\Microsoft\VC\v160\Microsoft.Cpp.Default.props" goto :eof

echo Set up VS 2026 160 VC Targets (Platform Toolset v142)

robocopy /NJH /NJS /NDL /NC /NS /S "%VSInstallDir%\MSBuild\Microsoft\VC\v160" %ExtractedFolder%\VCTargets160
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2026\pc\v160" "%ExtractedFolder%\VCTargets160"
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2026\gen9\v160" "%ExtractedFolder%\VCTargets160"
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2026\gen8\v160" "%ExtractedFolder%\VCTargets160"

goto :eof

:define_vs_2022_bwoi
if NOT EXIST "%VSInstallDir%\MSBuild\Microsoft\VC\v170\Microsoft.cpp.Default.props" goto :missingvsinstalldir

if %XDKEditionTarget GEQ 251000 goto :newlayoutdirs
if NOT EXIST "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GXDK\VS2022" goto :needsvs2022gdk
if NOT EXIST "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GRDK\VS2022" goto :needsvs2022gdk
:newlayoutdirs

if EXIST "%ExtractedFolder%\VCTargets180" goto :mismatchVCTargets
if NOT EXIST "%ExtractedFolder%\VCTargets160" md "%ExtractedFolder%\VCTargets160"
if NOT EXIST "%ExtractedFolder%\VCTargets170" md "%ExtractedFolder%\VCTargets170"

echo Set up VS 2022 170 VC Targets (Platform Toolset v143)

robocopy /NJH /NJS /NDL /NC /NS /S "%VSInstallDir%\MSBuild\Microsoft\VC\v170" %ExtractedFolder%\VCTargets170
if %XDKEditionTarget GEQ 251000 (
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2022\pc\v170" "%ExtractedFolder%\VCTargets170"
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2022\gen9\v170" "%ExtractedFolder%\VCTargets170"
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2022\gen8\v170" "%ExtractedFolder%\VCTargets170"
) else (
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GRDK\VS2022\flatDeployment\MSBuild\Microsoft\VC\v170" "%ExtractedFolder%\VCTargets170"
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GXDK\VS2022\flatDeployment\MSBuild\Microsoft\VC\v170" "%ExtractedFolder%\VCTargets170"
)

if NOT EXIST "%VSInstallDir%\MSBuild\Microsoft\VC\v160\Microsoft.Cpp.Default.props" goto :eof

echo Set up VS 2022 160 VC Targets (Platform Toolset v142)

robocopy /NJH /NJS /NDL /NC /NS /S "%VSInstallDir%\MSBuild\Microsoft\VC\v160" %ExtractedFolder%\VCTargets160
if %XDKEditionTarget GEQ 251000 (
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2022\pc\v160" "%ExtractedFolder%\VCTargets160"
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2022\gen9\v160" "%ExtractedFolder%\VCTargets160"
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\xbox\build\VS2022\gen8\v160" "%ExtractedFolder%\VCTargets160"
) else (
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GRDK\VS2022\flatDeployment\MSBuild\Microsoft\VC\v160" "%ExtractedFolder%\VCTargets160"
    robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GXDK\VS2022\flatDeployment\MSBuild\Microsoft\VC\v160" "%ExtractedFolder%\VCTargets160"
)

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
echo Script needs the VSInstallDir set to VS 2022 or VS 2026 install directory
exit /b 1

:needsvs2022gdk
echo VS 2022 BWOI support requires the March 2022 (2203) or later Microsoft GDK
exit /b 1

:needsvs2026gdk
echo VS 2026 BWOI support requires the April 2206 (2604) or later Microsoft GDK
exit /b 1

:mismatchVCTargets
echo Found VS 2026 targets when processing VS 2022. Recommend you delete "%ExtractedFolder%\VCTargets*" then re-run.
exit /b 1
