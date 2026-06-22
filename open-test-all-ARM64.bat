echo off
setlocal
cls

echo %CD%
set FIND_FILE_SAMPLES=find_file_samples.txt


pushd Samples
pushd %1
for /R /D %%D in ("*") DO ( 
    pushd "%%D"
    for %%S in ("*.sln") DO ( 
        @REM ECHO %%S 
        del /Q "%FIND_FILE_SAMPLES%" 1>nul 2>nul
        findstr "Debug|ARM64" %%S > %FIND_FILE_SAMPLES%
        for %%I in (%FIND_FILE_SAMPLES%) do (
            set SIZE="%%~zI"
            IF NOT "%%~zI" == "0" (
                @REM echo devenv.com %%S
                devenv.com %%S
            )
            del /Q "%FIND_FILE_SAMPLES%" 1>nul 2>nul
        )
    )
    popd
)
popd

