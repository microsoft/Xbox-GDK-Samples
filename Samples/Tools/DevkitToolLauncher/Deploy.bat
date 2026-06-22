@echo off

:: Check for help entries
if /I "%~1"=="" goto :DisplayUsage

:: First try to use xbconnect. If it fails, then this script wasn't opened with an Xbox gaming command prompt
xbconnect /s > nul
if ERRORLEVEL 0 if NOT ERRORLEVEL 1 goto :Good
goto :NeedGamingCommandPrompt

:Good

:: Parse Parameters
set Platform=Gaming.Xbox.Scarlett.x64
if /I "%~1"=="Scarlett" set Platform=Gaming.Xbox.Scarlett.x64
if /I "%~1"=="XboxOne" set Platform=Gaming.Xbox.XboxOne.x64

:: Deploy
xbapp deploy %~dp0%Platform%\Layout\Image\Loose /f:DevkitToolLauncher%1
goto :Exit

:NeedGamingCommandPrompt
echo.
echo Either deploy failed or you need to run this batch file from the Xbox Scarlett VS 2017/2019 Gaming Command Prompt.
echo.
goto :Exit

:DisplayUsage
echo.
echo Deploy.bat [Platform]
echo.
echo Platform - Set the target platform
echo   Values - XboxOne, Scarlett, x64
goto :Exit

:Exit
