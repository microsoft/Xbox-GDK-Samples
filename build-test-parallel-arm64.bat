echo off
setlocal
cls

pushd %1

echo %time%
set startTime=%time%
set _CL_=/MP
set lock=%CD%/locks/wait%random%.lock

mkdir locks 2>nul
mkdir logs 2>nul

echo %CD%
set LOG_FILE=%CD%/logs/test_build
set ERROR_LOG_FILE=%CD%/logs/test_build.txt

for /R /D %%K in ("kits.*") DO ( 
    pushd "%%K"
    for /R /D %%D in ("*") DO ( 
        pushd "%%D"
        for %%S in ("*.vcxproj") DO ( 
            findstr "Debug|ARM64" %%S >nul 2>nul
            if not errorlevel 1 (
                @REM devenv.com %%S /clean "Debug|ARM64" 
                echo devenv.com %%S /rebuild "Debug|ARM64" /out "%LOG_FILE%_%%S.txt"
                devenv.com %%S /rebuild "Debug|ARM64" /out "%LOG_FILE%_%%S.txt"
            )
        )
        popd
    )
    popd
)

echo Building ARM64 samples in parallel
for /R /D %%K in ("Samples.*") DO ( 
    pushd "%%K"
    for /R /D %%D in ("*") DO ( 
        pushd "%%D"
        for %%S in ("*.sln") DO ( 
            findstr "Debug|ARM64" %%S >nul 2>nul
            if not errorlevel 1 (
                1>nul 2>nul nuget restore
                @REM devenv.com %%S /clean "Debug|ARM64" 
                @REM nuget restore
                echo devenv.com %%S /build "Debug|ARM64" /out "%LOG_FILE%_%%S.txt"
                start 9>"%lock%%%S_arm64" devenv.com %%S /build "Debug|ARM64" /out "%LOG_FILE%_%%S.txt"
            )
        )
        popd
    )
    popd
)

echo Waiting for ARM64 builds to finish
:WaitARM64
1>nul 2>nul ping /n 2 ::1
for %%F in ("%lock%*") do (
  (call ) 9>"%%F" || goto :WaitARM64
) 2>nul

for %%F in ("%lock%*") do (
  del "%%F"
)

set stopTime=%time%
echo startTime=%startTime%   stopTime=%stopTime%

popd

