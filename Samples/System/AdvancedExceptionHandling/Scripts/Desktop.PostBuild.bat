REM Desktop build target Post Build script
REM Copies all files from the Assets\ subfolder within the project to the build target destination folder

REM %1 is expected to be the build project directory or $(ProjectDir), default to ""
REM %2 is expected to be the build target directory or $(TargetDir), default to "Gaming.Desktop.x64"

ECHO OFF

:GET_PROJECT_DIR
SET PROJECT_DIR=""
IF "%1" == "" GOTO MAKE_SOURCE
  SET PROJECT_DIR=%1

:MAKE_SOURCE
SET PROJECT_DIR=%PROJECT_DIR:"=%
SET SOURCE_DIR="%PROJECT_DIR%Assets"

:GET_TARGET_DIR
SET TARGET_DIR="Gaming.Desktop.x64\"
IF "%2" == "" GOTO MAKE_DEST
  SET TARGET_DIR=%2

:MAKE_DEST
SET TARGET_DIR=%TARGET_DIR:"=%
SET DEST_DIR="%TARGET_DIR%Assets"

:PERFORM_COPY
REM ECHO %SOURCE_DIR%
REM ECHO %DEST_DIR%

robocopy %SOURCE_DIR% %DEST_DIR% /MIR
if ERRORLEVEL 1 goto :eof
exit 0
