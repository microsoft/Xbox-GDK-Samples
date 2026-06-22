@echo off

:: Check for help entries
if /I "%~1"=="" goto :DisplayUsage
if /I "%~2"=="" goto :DisplayUsage

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
if /I "%~2"=="x64" set Platform=x64

:: Build
msbuild "%~dp0DevkitToolLauncher.sln" /t:Build /p:Configuration=%Configuration% /p:Platform=%Platform%
goto :Exit

:NeedGamingCommandPrompt
echo.
echo You need to run this batch file from the Xbox Scarlett VS 2017/2019 Gaming Command Prompt.
echo.
goto :Exit

:DisplayUsage
echo.
echo Build.bat [Configuration] [Platform]
echo.
echo Configuration - Set the build configuration
echo        Values - Debug, Release, Profile
echo.
echo      Platform - Set the target platform
echo        Values - XboxOne, Scarlett, x64
goto :Exit

:Exit
