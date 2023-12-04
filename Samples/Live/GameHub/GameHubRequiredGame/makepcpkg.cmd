@IF NOT EXIST .\output\NUL mkdir .\output
@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)
copy /y .\MicrosoftGameConfig_PC.mgc .\Gaming.Desktop.x64\%CONFIG%\MicrosoftGame.config
makepkg pack /pc /f chunks.xml /d .\Gaming.Desktop.x64\%CONFIG% /pd .\output /contentid 3562ab75-2926-4443-8ccd-7c0f6d50f359
