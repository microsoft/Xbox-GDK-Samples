@echo off

REM Build W/O Installing (BWOI) Example for the Microsoft GDK 
REM
REM This command sets a few specific variables to drive the build

if DEFINED VSInstallDir (goto :vs_install_dir_defined)

if %1.==vspreview. goto :define_vs2022_preview
if %1.==vs2019. goto :define_vs2019_install_dir
if %1.==vs2022. goto :define_vs2022_install_dir

echo "Usage: setenv {vs2019 | vs2022 | vspreview} [GDK edition] [extracted GDK path]"
exit /b 0

REM ****************************************************************************************************
REM VSInstallDir for Visual Studio 2019 / VS 2022 Preview
REM ****************************************************************************************************
:define_vs2022_preview
if NOT EXIST "%ProgramFiles%\Microsoft Visual Studio\2022\Preview\" goto :define_vs2019_preview
ECHO *** Using VS 2022 Preview ***
SET VSInstallDir=%ProgramFiles%\Microsoft Visual Studio\2022\Preview\
goto :vs_install_dir_defined

:define_vs2019_preview
if NOT EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Preview\" goto :error_preview
ECHO *** Using VS 2019 Preview ***
SET VSInstallDir=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Preview\
goto :vs_install_dir_defined

:error_preview
echo *** ERROR: Can't find VS 2022 Preview or VS 2019 Preview installation!
exit /b 1

REM ****************************************************************************************************
REM VSInstallDir for Visual Studio 2019
REM ****************************************************************************************************
:define_vs2019_install_dir
if NOT EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\" goto :vs2019_install_dir_professional
ECHO *** Using VS 2019 Enterprise ***
SET VSInstallDir=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\
goto :vs_install_dir_defined

:vs2019_install_dir_professional
if NOT EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\" goto :vs2019_install_dir_buildtools
ECHO *** Using VS 2019 Professional ***
SET VSInstallDir=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\
goto :vs_install_dir_defined

:vs2019_install_dir_buildtools
if NOT EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\" goto :vs2019_install_dir_unknown
ECHO *** Using VS 2019 Build Tools ***
SET VSInstallDir=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\
goto :vs_install_dir_defined

:vs2019_install_dir_unknown
@echo Couldn't figure out your Visual 2019 installation directory (need to use Visual Studio 2019 Enterprise, Professional, or Build Tools)
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
