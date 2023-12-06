@IF NOT EXIST .\output\NUL mkdir .\output
copy /y .\MicrosoftGameConfig_XboxOne.mgc .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\MicrosoftGame.config
makepkg pack /f chunks.xml /d .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose /pd .\output /contentid b9f06633-a22a-4e3e-a2cd-9a8b41ef02c1
