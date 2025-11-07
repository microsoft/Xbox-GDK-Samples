@IF NOT EXIST .\output\NUL mkdir .\output
@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)
copy /y .\MicrosoftGameConfig_PC.mgc .\x64\%CONFIG%\MicrosoftGame.config
makepkg pack /pc /f chunks.xml /d .\x64\%CONFIG% /pd .\output /contentid b9f06633-a22a-4e3e-a2cd-9a8b41ef02c1
