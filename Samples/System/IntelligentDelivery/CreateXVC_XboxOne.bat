@echo Copying over the MicrosoftGame.config file for the Xbox One device family
copy /b MicrosoftGame_XboxOne.config .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\MicrosoftGame.config

copy /b .\Assets\feature*.png .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\Assets\

REM Using the same key for subsequent package creation is important to maintain CUv3 compatibility
@IF NOT EXIST key.lekb makepkg genkey /ekb key.lekb

makepkg pack /v /f Chunks_XboxOne.xml /d .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose /pd .\Gaming.Xbox.XboxOne.x64\Layout\Image /lk key.lekb
