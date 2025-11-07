@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)
@IF NOT EXIST .\Package\Assets\Fonts\NUL mkdir .\Package\Assets\Fonts
copy ..\AlternateExperience\x64\%CONFIG%\AlternateExperience.exe .\Package
copy ..\AlternateExperience\x64\%CONFIG%\Assets\Fonts\*.* .\Package\Assets\Fonts
copy ..\AlternateExperience\Assets\*.* .\Package\Assets

@IF NOT EXIST .\output\NUL mkdir .\output
makepkg pack /pc /f chunks.xml /d ./Package /pd ./output
