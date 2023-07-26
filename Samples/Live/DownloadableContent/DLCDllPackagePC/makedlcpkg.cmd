@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)
copy ..\ComboDLL\Gaming.Desktop.x64\%CONFIG%\ComboDLL.dll .\Package
@IF NOT EXIST .\output\NUL mkdir .\output
makepkg pack /pc /f chunks.xml /d ./Package /pd ./output
