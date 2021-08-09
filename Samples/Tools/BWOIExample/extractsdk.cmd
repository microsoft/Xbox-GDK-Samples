@echo off
REM Build With/Out Installing (BWOI) Example
REM .
REM Extracts the MSIs in the Windows 10 SDK to set up BWOI (optional)
REM .
if %1.==. goto usage

if NOT EXIST "%1\Windows SDK Desktop Headers x64-x86_en-us.msi" goto nosdkfound

if %ExtractedFolder%.==. goto :missingextracted

if EXIST "%ExtractedFolder%\Windows Kits" goto :overwritewarn
if NOT EXIST "%ExtractedFolder%" ( md "%ExtractedFolder%"
if errorlevel 1 goto :makefailed )

for %%1 in ("%1\*.msi") do (echo "Extracting: %%1"
msiexec.exe /quiet /a "%%1" TARGETDIR="%ExtractedFolder%"
if errorlevel 1 goto :msifailed)
goto :eof

:usage
echo extractsdk sdk-source-installers-folder
exit /b 0

:nosdkfound
echo Can't find MSI files for the Windows 10 SDK
exit /b 1

:missingextracted
echo extractsdk requires the ExtractedFolder environment variable set by setenv.cmd
exit /b 1

:overwritewarn
echo ExtractedFolder for Windows 10 SDK already exists. Delete and rerun.
echo Conflicting folder: %ExtractedFolder%\Windows Kits
exit /b 1

:makefailed
echo Failed to create ExtractedFolder
exit /b /1

:msifailed
echo ERROR: MSI Extract failed
exit /b 1
