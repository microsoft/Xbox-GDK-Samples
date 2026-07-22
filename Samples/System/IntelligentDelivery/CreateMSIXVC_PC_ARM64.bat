@rem Copy needed files and assets into a staged directory
@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)

@echo Copying build binaries and assets using %CONFIG% build
copy /b .\ARM64\%CONFIG%\IntelligentDelivery.exe .\ARM64\Layout\Image\Loose\IntelligentDelivery.exe
copy /b .\ARM64\%CONFIG%\libHttpClient.dll .\ARM64\Layout\Image\Loose\libHttpClient.dll
copy /b .\ARM64\%CONFIG%\XCurl.dll .\ARM64\Layout\Image\Loose\XCurl.dll

@IF NOT EXIST .\ARM64\Layout\Image\Loose\Assets\NUL mkdir .\ARM64\Layout\Image\Loose\Assets\
copy /b .\Assets\*Logo.png .\ARM64\Layout\Image\Loose\Assets\
copy /b .\Assets\SplashScreen.png .\ARM64\Layout\Image\Loose\Assets\SplashScreen.png
copy /b .\Assets\feature*.png .\ARM64\Layout\Image\Loose\Assets\
xcopy /s /y .\ARM64\%CONFIG%\Assets .\ARM64\Layout\Image\Loose\Assets\

echo Copying over the MicrosoftGame.config file for PC
copy /b MicrosoftGameConfig_PC.mgc .\ARM64\Layout\Image\Loose\MicrosoftGame.config

REM It is possible to /lk sign package for PC, but they as of yet cannot be installed

makepkg pack /pc /v /f Chunks_PC.xml /d .\ARM64\Layout\Image\Loose /pd .\ARM64\Layout\Image
