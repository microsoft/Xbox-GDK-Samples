@IF NOT EXIST .\output\NUL mkdir .\output
copy /y .\MicrosoftGameConfig_Scarlett.mgc .\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\MicrosoftGame.config
makepkg pack /f chunks.xml /d .\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose /pd .\output /contentid b9f06633-a22a-4e3e-a2cd-9a8b41ef02c1
