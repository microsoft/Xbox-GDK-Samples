@echo off
setlocal enabledelayedexpansion

set "file=%~1"
set "search=version:"
set "outputFile=%~2"

echo Searching file %file% >> "%outputFile%"

for /f "usebackq tokens=*" %%a in ("%file%") do (
    set "line=%%a"
    if "!line:~0,8!" EQU "%search%" (
        set "value=!line:~9!"
        echo Version !value! was found. >> "%outputFile%"
        echo !value!
        exit /b
    )
)

echo Value not found. >> "%outputFile%"
exit /b
