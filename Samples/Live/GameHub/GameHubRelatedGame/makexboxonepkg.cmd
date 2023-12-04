@IF NOT EXIST .\output\NUL mkdir .\output
copy /y .\MicrosoftGameConfig_XboxOne.mgc .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\MicrosoftGame.config
makepkg pack /f chunks.xml /d .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose /pd .\output /contentid 46951ea8-69c1-4ab8-a158-524afab1e635
