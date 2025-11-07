@IF NOT EXIST .\output\NUL mkdir .\output
@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)
copy /y .\MicrosoftGameConfig_PC.mgc .\x64\%CONFIG%\MicrosoftGame.config
makepkg pack /pc /f chunks.xml /d .\x64\%CONFIG% /pd .\output /contentid 46951ea8-69c1-4ab8-a158-524afab1e635
