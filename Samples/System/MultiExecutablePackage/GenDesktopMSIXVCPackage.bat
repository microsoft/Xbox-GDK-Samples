@rem Copy needed files and assets into a staged directory
@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)

@echo Copying build binaries and assets using %CONFIG% build
@IF NOT EXIST .\Gaming.Desktop.x64\Layout\Image\Loose\NUL mkdir .\Gaming.Desktop.x64\Layout\Image\Loose\
copy /b .\Gaming.Desktop.x64\%CONFIG%\*.exe .\Gaming.Desktop.x64\Layout\Image\Loose\
copy /b .\Gaming.Desktop.x64\%CONFIG%\*.dll .\Gaming.Desktop.x64\Layout\Image\Loose\

@IF NOT EXIST .\Gaming.Desktop.x64\Layout\Image\Loose\Assets\NUL mkdir .\Gaming.Desktop.x64\Layout\Image\Loose\Assets\
xcopy /s /y .\Gaming.Desktop.x64\%CONFIG%\Assets .\Gaming.Desktop.x64\Layout\Image\Loose\Assets\

echo Copying over the MicrosoftGame.config file for Desktop
copy /b .\DefaultExperience\MicrosoftGameConfig_PC.mgc .\Gaming.Desktop.x64\Layout\Image\Loose\MicrosoftGame.config

REM It is possible to /lk sign package for PC, but they as of yet cannot be installed

makepkg pack /pc /v /f PackageLayout.xml /d .\Gaming.Desktop.x64\Layout\Image\Loose\ /pd .\Gaming.Desktop.x64\Layout\Image
