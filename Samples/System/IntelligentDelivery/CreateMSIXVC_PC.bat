@rem Copy needed files and assets into a staged directory
@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)

@echo Copying build binaries and assets using %CONFIG% build
copy /b .\Gaming.Desktop.x64\%CONFIG%\IntelligentDelivery.exe .\Gaming.Desktop.x64\Layout\Image\Loose\IntelligentDelivery.exe
copy /b .\Gaming.Desktop.x64\%CONFIG%\XCurl.dll .\Gaming.Desktop.x64\Layout\Image\Loose\XCurl.dll

@IF NOT EXIST .\Gaming.Desktop.x64\Layout\Image\Loose\Assets\NUL mkdir .\Gaming.Desktop.x64\Layout\Image\Loose\Assets\
copy /b .\Assets\*Logo.png .\Gaming.Desktop.x64\Layout\Image\Loose\Assets\
copy /b .\Assets\SplashScreen.png .\Gaming.Desktop.x64\Layout\Image\Loose\Assets\SplashScreen.png
copy /b .\Assets\feature*.png .\Gaming.Desktop.x64\Layout\Image\Loose\Assets\
xcopy /s /y .\Gaming.Desktop.x64\%CONFIG%\Assets .\Gaming.Desktop.x64\Layout\Image\Loose\Assets\

echo Copying over the MicrosoftGame.config file for PC
copy /b MicrosoftGame_PC.config .\Gaming.Desktop.x64\Layout\Image\Loose\MicrosoftGame.config

REM It is possible to /lk sign package for PC, but they as of yet cannot be installed

makepkg pack /pc /v /f Chunks_PC.xml /d .\Gaming.Desktop.x64\Layout\Image\Loose /pd .\Gaming.Desktop.x64\Layout\Image
