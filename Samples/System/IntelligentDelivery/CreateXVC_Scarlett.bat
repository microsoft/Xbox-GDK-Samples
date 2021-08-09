@echo Copying over the MicrosoftGame.config file for the Scarlett device family
copy /b MicrosoftGame_Scarlett.config .\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\MicrosoftGame.config

makepkg pack /v /f Chunks_Scarlett.xml /d .\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose /pd .\Gaming.Xbox.Scarlett.x64\Layout\Image
