echo off
setlocal
cls

REM These blocks are when a clean copy of the source is needed for later testing
REM if "%1"=="" (
REM     echo Missing name of the working directory
REM     goto:eof 
REM )

REM echo Looking for working directory
REM pushd %1
REM if %errorlevel% == 0 (
REM     popd
REM     echo Cleaning old working directory
REM     for /D %%D in ("%1\*") DO ( 
REM         rd /S /Q %%D
REM     )
REM )

echo %time%
set startTime=%time%
set GXDK_VERSION=22000.4360.220602-2100
set XDKEditionTarget=2206
echo Cleaning LocalDist
REM rd /S /Q LocalDist
echo Creating LocalDist, this will take a while
msbuild Infrastructure/Build/SubtreeDist_gxdk.proj /t:DistributeAll /p:SelfContainedSamples=true -verbosity:quiet

REM echo Copying source to destination
REM xcopy LocalDist\XGD %1\source /S /C /Q /I

pushd LocalDist
1>nul 2>nul del /s /q TestOnly
1>nul 2>nul rd /s /q TestOnly
popd

echo %time%
set durpioStartTime=%time%
set lock=%CD%/locks\wait%random%.lock
set _CL_=/MP

mkdir locks
mkdir logs
echo %CD%
set FIND_FILE=find_file.txt
set LOG_FILE=%CD%/logs/test_build
set ERROR_LOG_FILE=%CD%/logs/test_build.txt

echo Building Xbox samples
for /R /D %%K in ("LocalDist.*") DO ( 
    pushd "%%K"
    for /R /D %%D in ("*") DO ( 
        pushd "%%D"
        for %%S in ("*.sln") DO ( 
            del /Q "%FIND_FILE%" 1>nul 2>nul
            findstr "Debug|Gaming.Xbox.XboxOne.x64" %%S > %FIND_FILE%
            for %%I in (%FIND_FILE%) do (
                set SIZE="%%~zI"
                IF NOT "%%~zI" == "0" (
                    1>nul 2>nul nuget restore
                    start 9>"%lock%%%S_xbox" devenv.com %%S /rebuild "Debug|Gaming.Xbox.XboxOne.x64" /out "%LOG_FILE%_%%S.txt"
                )
                del /Q "%FIND_FILE%" 1>nul 2>nul
            )
        )
        popd
    )
    popd
)

echo waiting for Xbox projects to finish building
:WaitXbox for all processes to finish (wait until lock files are no longer locked)
1>nul 2>nul ping /n 2 ::1
for %%F in ("%lock%*") do (
  (call ) 9>"%%F" || goto :WaitXbox
) 2>nul

for %%F in ("%lock%*") do (
  del "%%F"
) 
echo %time%
set scarlettStartTime=%time%

echo Build Scarlett versions
for /R /D %%K in ("LocalDist.*") DO ( 
    pushd "%%K"
    for /R /D %%D in ("*") DO ( 
        pushd "%%D"
        for %%S in ("*.sln") DO ( 
            del /Q "%FIND_FILE%" 1>nul 2>nul
            findstr "Debug|Gaming.Xbox.Scarlett.x64" %%S > %FIND_FILE%
            for %%I in (%FIND_FILE%) do (
                set SIZE="%%~zI"
                IF NOT "%%~zI" == "0" (
                    REM devenv.com %%S /clean "Debug|Gaming.Xbox.Scarlett.x64" 
                    1>nul 2>nul nuget restore
                    start 9>"%lock%%%S_scarlett" devenv.com %%S /rebuild "Debug|Gaming.Xbox.Scarlett.x64" /out "%LOG_FILE%_%%S.txt"
                )
                del /Q "%FIND_FILE%" 1>nul 2>nul
            )
        )
        popd
    )
    popd
)

echo waiting for Scarlett projects to finish building
:WaitScarlett for all processes to finish (wait until lock files are no longer locked)
1>nul 2>nul ping /n 2 ::1
for %%F in ("%lock%*") do (
  (call ) 9>"%%F" || goto :WaitScarlett
) 2>nul

for %%F in ("%lock%*") do (
  del "%%F"
) 
SET stopTime=%time%
echo startTime=%startTime%   stopTime=%stopTime%    durpioStartTime=%durpioStartTime%   scarlettStartTime=%scarlettStartTime%
