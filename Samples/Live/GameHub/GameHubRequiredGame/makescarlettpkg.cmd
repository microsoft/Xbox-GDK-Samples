@IF NOT EXIST .\output\NUL mkdir .\output
copy /y .\MicrosoftGameConfig_Scarlett.mgc .\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\MicrosoftGame.config
makepkg pack /f chunks.xml /d .\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose /pd .\output /contentid 3562ab75-2926-4443-8ccd-7c0f6d50f359
