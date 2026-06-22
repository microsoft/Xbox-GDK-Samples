@echo off

if not "%BUILDSERVER%"=="" goto :Exit

:: Check for help entries
if /I "%~1"=="" goto :DisplayUsage
if /I "%~2"=="" goto :DisplayUsage

:: Skip command prompt usage if 3rd parameter is set to "VSPostBuildStep"
if /I "%~3"=="VSPostBuildStep" goto :Good

:: First try to use xbconnect. If it fails, then this script wasn't opened with an Xbox gaming command prompt
xbconnect /s > nul
if ERRORLEVEL 0 if NOT ERRORLEVEL 1 goto :Good
goto :NeedGamingCommandPrompt

:Good

:: Parse Parameters
set Configuration=%1
set Platform=Gaming.Xbox.Scarlett.x64
if /I "%~2"=="Scarlett" set Platform=Gaming.Xbox.Scarlett.x64
if /I "%~2"=="Gaming.Xbox.Scarlett.x64" set Platform=Gaming.Xbox.Scarlett.x64
if /I "%~2"=="XboxOne" set Platform=Gaming.Xbox.XboxOne.x64
if /I "%~2"=="Gaming.Xbox.XboxOne.x64" set Platform=Gaming.Xbox.XboxOne.x64

:: Copy CPUTool
xbcp %~dp0CPUTool\x64\%Configuration% xd:\DevkitToolLauncherExampleTools\CPUTool

:: CPUTool needs basic crt/vc dlls to run on Xbox, so copy them from the DevkitToolLauncher project
xbcp %~dp0%Platform%\%Configuration%\*.dll xd:\DevkitToolLauncherExampleTools\CPUTool

:: Copy GPUTool
xbcp %~dp0GPUTool\%Platform%\Layout\Image\Loose xd:\DevkitToolLauncherExampleTools\GPUTool

goto :Exit

:NeedGamingCommandPrompt
echo.
echo You need to run this batch file from the Xbox Scarlett VS 2017/2019 Gaming Command Prompt.
echo.
goto :Exit

:DisplayUsage
echo.
echo DeployExampleTools.bat [Configuration] [Platform]
echo.
echo Configuration - Set the build configuration
echo        Values - Debug, Release, Profile
echo.
echo      Platform - Set the target platform
echo        Values - XboxOne, Scarlett
goto :Exit

:Exit
