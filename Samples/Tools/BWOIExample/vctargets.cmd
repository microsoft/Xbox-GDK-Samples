@echo off
REM Build With/Out Installing (BWOI) Example
REM .
REM Sets up the VCTargets folders needed for VS 2019 / VS 2022 BWOI
REM .

if %XDKEditionTarget%.==. goto :missingxdktarg
if %ExtractedFolder%.==. goto :missingextracted
if "%VSInstallDir%."=="." goto :missingvsinstalldir

if NOT EXIST "%ExtractedFolder%\Microsoft GDK" goto :gdkmissing
if NOT EXIST "%VSInstallDir%\MSBuild" goto :missingvsinstalldir

:define_vs_2022_bwoi

if NOT EXIST "%VSInstallDir%\MSBuild\Microsoft\VC\v170\Microsoft.cpp.Default.props" goto :define_vs_2019_bwoi

if NOT EXIST "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GXDK\VS2022" goto :needsvs2022gdk
if NOT EXIST "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GRDK\VS2022" goto :needsvs2022gdk

if NOT EXIST "%ExtractedFolder%\VCTargets160" md "%ExtractedFolder%\VCTargets160"
if NOT EXIST "%ExtractedFolder%\VCTargets170" md "%ExtractedFolder%\VCTargets170"

echo Set up VS 2022 170 VC Targets (Platform Toolset v143)

robocopy /NJH /NJS /NDL /NC /NS /S "%VSInstallDir%\MSBuild\Microsoft\VC\v170" %ExtractedFolder%\VCTargets170
robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GRDK\VS2022\flatDeployment\MSBuild\Microsoft\VC\v170" "%ExtractedFolder%\VCTargets170"
robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GXDK\VS2022\flatDeployment\MSBuild\Microsoft\VC\v170" "%ExtractedFolder%\VCTargets170"

if NOT EXIST "%VSInstallDir%\MSBuild\Microsoft\VC\v160\Microsoft.Cpp.Default.props" goto :skipv160

echo Set up VS 2022 160 VC Targets (Platform Toolset v142)

robocopy /NJH /NJS /NDL /NC /NS /S "%VSInstallDir%\MSBuild\Microsoft\VC\v160" %ExtractedFolder%\VCTargets160
robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GRDK\VS2022\flatDeployment\MSBuild\Microsoft\VC\v160" "%ExtractedFolder%\VCTargets160"
robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GXDK\VS2022\flatDeployment\MSBuild\Microsoft\VC\v160" "%ExtractedFolder%\VCTargets160"

:skipv160

if NOT EXIST "%VSInstallDir%\MSBuild\Microsoft\VC\v150\Microsoft.Cpp.Default.props" goto :eof
if %XDKEditionTarget% LSS 241000 goto :eof

if NOT EXIST "%ExtractedFolder%\VCTargets150" md "%ExtractedFolder%\VCTargets150"

echo Set up VS 2022 150 VC Targets (Platform Toolset v141)

robocopy /NJH /NJS /NDL /NC /NS /S "%VSInstallDir%\MSBuild\Microsoft\VC\v150" %ExtractedFolder%\VCTargets150
robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GRDK\VS2022\flatDeployment\MSBuild\Microsoft\VC\v150" "%ExtractedFolder%\VCTargets150"
robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GXDK\VS2022\flatDeployment\MSBuild\Microsoft\VC\v150" "%ExtractedFolder%\VCTargets150"

goto :eof

:define_vs_2019_bwoi
if NOT EXIST "%VSInstallDir%\MSBuild\Microsoft\VC\v160\Microsoft.cpp.Default.props" goto :missingvsinstalldir

if EXIST "%ExtractedFolder%\VCTargets170" goto :mismatchVCTargets
if NOT EXIST "%ExtractedFolder%\VCTargets160" md "%ExtractedFolder%\VCTargets160"

echo Set up VS 2019 160 VC Targets (Platform Toolset v142)

robocopy /NJH /NJS /NDL /NC /NS /S "%VSInstallDir%\MSBuild\Microsoft\VC\v160" %ExtractedFolder%\VCTargets160
robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GRDK\VS2019\flatDeployment\MSBuild\Microsoft\VC\v160" "%ExtractedFolder%\VCTargets160"
robocopy /NJH /NJS /NDL /NC /NS /S "%ExtractedFolder%\Microsoft GDK\%XDKEditionTarget%\GXDK\VS2019\flatDeployment\MSBuild\Microsoft\VC\v160" "%ExtractedFolder%\VCTargets160"

if NOT EXIST "%VSInstallDir%\MSBuild\Microsoft\VC\v150\Microsoft.Cpp.Default.props" goto :eof
if %XDKEditionTarget% LSS 241000 goto :eof

if NOT EXIST "%ExtractedFolder%\VCTargets150" md "%ExtractedFolder%\VCTargets150"

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
echo Script needs the VSInstallDir set to VS 2019 or VS 2022 install directory
exit /b 1

:needsvs2022gdk
echo VS 2022 BWOI support requires the March 2022 (2203) or later Microsoft GDK
exit /b 1

:mismatchVCTargets
echo Found VS 2022 targets when processing VS 2019. Recommend you delete "%ExtractedFolder%\VCTargets*" then re-run.
exit /b 1
