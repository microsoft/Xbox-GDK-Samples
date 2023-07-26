@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)
copy /Y ..\ComboDLL\Gaming.Xbox.Scarlett.x64\%CONFIG%\ComboDLL.dll .\Package_Scarlett
copy /Y ..\ComboDLL\Gaming.Xbox.XboxOne.x64\%CONFIG%\ComboDLL.dll .\Package_XboxOne

@IF NOT EXIST .\output\NUL mkdir .\output
makepkg pack /f chunks.xml /d ./Package_XboxOne /pd ./output
makepkg pack /f chunks.xml /d ./Package_Scarlett /pd ./output