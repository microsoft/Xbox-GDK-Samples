@echo Copying over the MicrosoftGame.config file for the Xbox One device family
copy /b MicrosoftGame_XboxOne.config .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\MicrosoftGame.config

makepkg pack /v /f Chunks_XboxOne.xml /d .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose /pd .\Gaming.Xbox.XboxOne.x64\Layout\Image
