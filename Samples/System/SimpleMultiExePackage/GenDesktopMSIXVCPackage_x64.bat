@rem Copy needed files and assets into a staged directory
@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)

@echo Copying build binaries and assets using %CONFIG% build
@IF NOT EXIST .\x64\Layout\Image\Loose\NUL mkdir .\x64\Layout\Image\Loose\
copy /b .\x64\%CONFIG%\*.exe .\x64\Layout\Image\Loose\
copy /b .\x64\%CONFIG%\*.dll .\x64\Layout\Image\Loose\

@IF NOT EXIST .\x64\Layout\Image\Loose\Assets\NUL mkdir .\x64\Layout\Image\Loose\Assets\
xcopy /s /y .\x64\%CONFIG%\Assets .\x64\Layout\Image\Loose\Assets\

echo Copying over the MicrosoftGame.config file for Desktop
copy /b .\DefaultExperience\MicrosoftGameConfig_PC.mgc .\x64\Layout\Image\Loose\MicrosoftGame.config

REM It is possible to /lk sign package for PC, but they as of yet cannot be installed

makepkg pack /pc /v /f PackageLayout.xml /d .\x64\Layout\Image\Loose\ /pd .\x64\Layout\Image
