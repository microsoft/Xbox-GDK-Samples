@rem Copy needed files and assets into a staged directory
@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)

@echo Copying build binaries and assets using %CONFIG% build
@IF NOT EXIST .\ARM64\Layout\Image\Loose\NUL mkdir .\ARM64\Layout\Image\Loose\
copy /b .\ARM64\%CONFIG%\*.exe .\ARM64\Layout\Image\Loose\
copy /b .\ARM64\%CONFIG%\*.dll .\ARM64\Layout\Image\Loose\

@IF NOT EXIST .\ARM64\Layout\Image\Loose\Assets\NUL mkdir .\ARM64\Layout\Image\Loose\Assets\
xcopy /s /y .\ARM64\%CONFIG%\Assets .\ARM64\Layout\Image\Loose\Assets\

echo Copying over the MicrosoftGame.config file for Desktop
copy /b .\DefaultExperience\MicrosoftGameConfig_PC.mgc .\ARM64\Layout\Image\Loose\MicrosoftGame.config

REM It is possible to /lk sign package for PC, but they as of yet cannot be installed

makepkg pack /pc /v /f PackageLayout.xml /d .\ARM64\Layout\Image\Loose\ /pd .\ARM64\Layout\Image
