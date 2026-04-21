@echo off

REM Build W/O Installing (BWOI) Example for the Microsoft GDK
REM
REM This command sets a few specific variables to drive the build

if DEFINED VSInstallDir (goto :vs_install_dir_defined)

if %1.==vspreview. goto :define_vs2026_insiders
if %1.==vs2026. goto :define_vs2026_install_dir
if %1.==vs2022. goto :define_vs2022_install_dir

echo "Usage: setenv {vs2022 | vs2026 | vspreview} [GDK edition] [extracted GDK path]"
exit /b 0

REM ****************************************************************************************************
REM VSInstallDir for Visual Studio 2026 Insiders
REM ****************************************************************************************************
:define_vs2026_insiders
if NOT EXIST "%ProgramFiles%\Microsoft Visual Studio\18\Insiders\" goto :error_preview
ECHO *** Using VS 2026 Insiders ***
SET VSInstallDir=%ProgramFiles%\Microsoft Visual Studio\18\Insiders\
goto :vs_install_dir_defined

:error_preview
echo *** ERROR: Can't find VS 2026 Insiders installation!
exit /b 1

REM ****************************************************************************************************
REM VSInstallDir for Visual Studio 2026
REM ****************************************************************************************************
:define_vs2026_install_dir
if NOT EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\18\Enterprise\" goto :vs2026_install_dir_professional
ECHO *** Using VS 2026 Enterprise ***
SET VSInstallDir=%ProgramFiles(x86)%\Microsoft Visual Studio\18\Enterprise\
goto :vs_install_dir_defined

:vs2026_install_dir_professional
if NOT EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\18\Professional\" goto :vs2026_install_dir_buildtools
ECHO *** Using VS 2026 Professional ***
SET VSInstallDir=%ProgramFiles(x86)%\Microsoft Visual Studio\18\Professional\
goto :vs_install_dir_defined

:vs2026_install_dir_buildtools
if NOT EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\" goto :vs2026_install_dir_unknown
ECHO *** Using VS 2026 Build Tools ***
SET VSInstallDir=%ProgramFiles(x86)%\Microsoft Visual Studio\18\BuildTools\
goto :vs_install_dir_defined

:vs2026_install_dir_unknown
@echo Couldn't figure out your Visual 2026 installation directory (need to use Visual Studio 2026 Enterprise, Professional, or Build Tools)
exit /b 1

REM ****************************************************************************************************
REM VSInstallDir for Visual Studio 2022
REM ****************************************************************************************************
:define_vs2022_install_dir
if NOT EXIST "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\" goto :vs2022_install_dir_professional
ECHO *** Using VS 2022 Enterprise ***
SET VSInstallDir=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\
goto :vs_install_dir_defined

:vs2022_install_dir_professional
if NOT EXIST "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\" goto :vs2022_install_dir_buildtools
ECHO *** Using VS 2022 Professional ***
SET VSInstallDir=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\
goto :vs_install_dir_defined

:vs2022_install_dir_buildtools
if NOT EXIST "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\" goto :vs2022_install_dir_unknown
ECHO *** Using VS 2022 Build Tools ***
SET VSInstallDir=%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\
goto :vs_install_dir_defined

:vs2022_install_dir_unknown
@echo Couldn't figure out your Visual 2022 installation directory (need to use Visual Studio 2022 Enterprise, Professional, or Build Tools)
exit /b 1

:vs_install_dir_defined

if EXIST "%VSInstallDir%MSBuild\Current\Bin\MSBuild.exe" (set "PATH=%VSInstallDir%MSBuild\Current\Bin;%PATH%")

REM ****************************************************************************************************
REM Pick a folder for the extracted content (do not use a deep path or you will hit _MAX_PATH issues)
REM ****************************************************************************************************
if NOT %3.==. (set ExtractedFolder=%3) else (set ExtractedFolder=%~d0\xtrctd.sdks\BWOIExample)

REM ****************************************************************************************************
REM Figure out our target edition
REM ****************************************************************************************************
if NOT %2.==. (set XdkEditionTarget=%2) else (set XdkEditionTarget=240300)

echo ExtractedFolder=%ExtractedFolder%
echo XdkEditionTarget=%XdkEditionTarget%
