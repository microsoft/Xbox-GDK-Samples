@echo OFF

echo Launching AtgSample.exe from remote server... If this step fails, you need to connect to vpn!
START /B /D .\ /WAIT \\atg-samples.redmond.corp.microsoft.com\atgsample\AtgSample.exe

IF NOT "%ERRORLEVEL%" == "0" (
    REM Infrastructure\Tools\AtgSample.exe
	ECHO AtgSample.exe failed to run. Contact atgsv@microsoft.com for assistance!
    GOTO :end
)

:end