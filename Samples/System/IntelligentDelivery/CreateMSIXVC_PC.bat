@rem Copy needed files and assets into a staged directory
@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)

@echo Copying build binaries and assets using %CONFIG% build
copy /b .\x64\%CONFIG%\IntelligentDelivery.exe .\x64\Layout\Image\Loose\IntelligentDelivery.exe
copy /b .\x64\%CONFIG%\libHttpClient.dll .\x64\Layout\Image\Loose\libHttpClient.dll
copy /b .\x64\%CONFIG%\XCurl.dll .\x64\Layout\Image\Loose\XCurl.dll

@IF NOT EXIST .\x64\Layout\Image\Loose\Assets\NUL mkdir .\x64\Layout\Image\Loose\Assets\
copy /b .\Assets\*Logo.png .\x64\Layout\Image\Loose\Assets\
copy /b .\Assets\SplashScreen.png .\x64\Layout\Image\Loose\Assets\SplashScreen.png
copy /b .\Assets\feature*.png .\x64\Layout\Image\Loose\Assets\
xcopy /s /y .\x64\%CONFIG%\Assets .\x64\Layout\Image\Loose\Assets\

echo Copying over the MicrosoftGame.config file for PC
copy /b MicrosoftGameConfig_PC.mgc .\x64\Layout\Image\Loose\MicrosoftGame.config

REM It is possible to /lk sign package for PC, but they as of yet cannot be installed

makepkg pack /pc /v /f Chunks_PC.xml /d .\x64\Layout\Image\Loose /pd .\x64\Layout\Image
