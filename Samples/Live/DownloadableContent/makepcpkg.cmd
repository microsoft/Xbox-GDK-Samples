@IF NOT EXIST .\output\NUL mkdir .\output
@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)
makepkg pack /pc /f chunks.xml /d .\Gaming.Desktop.x64\%CONFIG% /pd .\output /contentid DD755A56-F89D-443F-81C9-47412CA0E048
