echo off
setlocal
cls

pushd %1

echo %CD%
set FIND_FILE_INFRASTRUCTURE=find_file_infrastructure.txt
set FIND_FILE_SAMPLES=find_file_samples.txt
set LOG_FILE=%CD%/test_build
set ERROR_LOG_FILE=%CD%/test_build.txt

for /R /D %%K in ("kits.*") DO ( 
    pushd "%%K"
    for /R /D %%D in ("*") DO ( 
        pushd "%%D"
        for %%S in ("*.vcxproj") DO ( 
            @REM ECHO %%S 
            del /Q "%FIND_FILE_INFRASTRUCTURE%" 1>nul 2>nul
            findstr "Debug|ARM64" %%S > %FIND_FILE_INFRASTRUCTURE%
            for %%I in (%FIND_FILE_INFRASTRUCTURE%) do (
                set SIZE="%%~zI"
                IF NOT "%%~zI" == "0" (
                    devenv.com %%S /clean "Debug|ARM64" 
                    echo devenv.com %%S /rebuild "Debug|ARM64" /out "%LOG_FILE%_%%S.txt"
                    @REM devenv.com %%S /rebuild "Debug|ARM64" /out "%LOG_FILE%_%%S.txt"
                )
                del /Q "%FIND_FILE_INFRASTRUCTURE%" 1>nul 2>nul
            )
        )
        popd
    )
    popd
)

for /R /D %%K in ("Samples.*") DO ( 
    pushd "%%K"
    for /R /D %%D in ("*") DO ( 
        pushd "%%D"
        for %%S in ("*.sln") DO ( 
            @REM ECHO %%S 
            del /Q "%FIND_FILE_SAMPLES%" 1>nul 2>nul
            findstr "Debug|ARM64" %%S > %FIND_FILE_SAMPLES%
            for %%I in (%FIND_FILE_SAMPLES%) do (
                set SIZE="%%~zI"
                IF NOT "%%~zI" == "0" (
                    devenv.com %%S /clean "Debug|ARM64" 
                    @REM nuget restore
                    echo devenv.com %%S /build "Debug|ARM64" /out "%LOG_FILE%_%%S.txt"
                    @REM devenv.com %%S /build "Debug|ARM64" /out "%LOG_FILE%_%%S.txt"
                )
                del /Q "%FIND_FILE_SAMPLES%" 1>nul 2>nul
            )
        )
        popd
    )
    popd
)
popd

