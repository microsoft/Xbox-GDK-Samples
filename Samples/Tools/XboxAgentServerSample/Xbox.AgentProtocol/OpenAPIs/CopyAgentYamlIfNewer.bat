@echo off
setlocal enabledelayedexpansion

set "newFile=%~1"
set "existingFile=%~2"
set "outputFile=%~3\YamlUpdateTaskOutput.txt"

REM Remove any prior output file.
del %outputFile%

if exist "%newFile%" (
    for /f "tokens=1 delims=/ " %%a in ('OpenAPIs\GetAgentYamlVersion.bat "%newFile%" "%outputFile%"') do (
        set "version1=%%a"
    )

    for /f "tokens=1 delims=/ " %%a in ('OpenAPIs\GetAgentYamlVersion.bat "%existingFile%" "%outputFile%"') do (
        set "version2=%%a"
    )

    if "!version1!" gtr "!version2!" (
        copy /y "%newFile%" "%existingFile%"
        echo "%existingFile%" replaced by "%newFile%". >> "%outputFile%"
    ) else (
        echo "%existingFile%" up to date. >> "%outputFile%"
    )
) else (
    echo "%newFile%" does not exist. >> "%outputFile%"
)
exit /b
