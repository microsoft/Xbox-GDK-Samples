@echo off

:: First try to use xbconnect. If it fails, then this script wasn't opened with an Xbox gaming command prompt
xbconnect /s > nul
if ERRORLEVEL 0 if NOT ERRORLEVEL 1 goto :Good
goto :NeedGamingCommandPrompt

:Good

:: Launch
xbapp launch b484cd03-523b-4450-bb87-b467f2a14fa6_8wekyb3d8bbwe!Game %*
goto :Exit

:NeedGamingCommandPrompt
echo.
echo You need to run this batch file from the Xbox Scarlett VS 2017/2019 Gaming Command Prompt.
echo.

:Exit
