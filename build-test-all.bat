REM echo off
setlocal
cls

pushd %1

echo %CD%
set FIND_FILE=find_file.txt
set LOG_FILE=%CD%/test_build
set ERROR_LOG_FILE=%CD%/test_build.txt

for /R /D %%K in ("kits.*") DO ( 
    pushd "%%K"
    for /R /D %%D in ("*") DO ( 
        pushd "%%D"
        for %%S in ("*.vcxproj") DO ( 
            ECHO %%S 
            del /Q "%FIND_FILE%" 1>nul 2>nul
            findstr "Debug|Gaming.Xbox.XboxOne.x64" %%S > %FIND_FILE%
            for %%I in (%FIND_FILE%) do (
                set SIZE="%%~zI"
                IF NOT "%%~zI" == "0" (
                    devenv.com %%S /clean "Debug|Gaming.Xbox.XboxOne.x64" 
                    devenv.com %%S /rebuild "Debug|Gaming.Xbox.XboxOne.x64" /out "%LOG_FILE%_%%S.txt"
                )
                del /Q "%FIND_FILE%" 1>nul 2>nul
            )
        )
        popd
    )
    popd
)

for /R /D %%K in ("GDKSamples.*") DO ( 
    pushd "%%K"
    for /R /D %%D in ("*") DO ( 
        pushd "%%D"
        for %%S in ("*.sln") DO ( 
            ECHO %%S 
            del /Q "%FIND_FILE%" 1>nul 2>nul
            findstr "Debug|Gaming.Xbox.XboxOne.x64" %%S > %FIND_FILE%
            for %%I in (%FIND_FILE%) do (
                set SIZE="%%~zI"
                IF NOT "%%~zI" == "0" (
                    REM devenv.com %%S /clean "Debug|Gaming.Xbox.XboxOne.x64" 
                    devenv.com %%S /build "Debug|Gaming.Xbox.XboxOne.x64" /out "%LOG_FILE%_%%S.txt"
                )
                del /Q "%FIND_FILE%" 1>nul 2>nul
            )
        )
        popd
    )
    popd
)
popd

