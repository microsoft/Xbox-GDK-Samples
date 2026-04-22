@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)
@IF NOT EXIST .\Package_Scarlett\Assets\Fonts\NUL mkdir .\Package_Scarlett\Assets\Fonts
copy /Y .\AlternateExperience\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\AlternateExperience.exe .\Package_Scarlett
copy /Y .\AlternateExperience\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\Assets\Fonts\*.* .\Package_Scarlett\Assets\Fonts
copy /Y .\AlternateExperience\Assets\*.* .\Package_Scarlett\Assets
@IF NOT EXIST .\Package_XboxOne\Assets\Fonts\NUL mkdir .\Package_XboxOne\Assets\Fonts
copy /Y .\AlternateExperience\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\AlternateExperience.exe .\Package_XboxOne
copy /Y .\AlternateExperience\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\Assets\Fonts\*.* .\Package_XboxOne\Assets\Fonts
copy /Y .\AlternateExperience\Assets\*.* .\Package_XboxOne\Assets
@IF NOT EXIST .\Package_PC\Assets\Fonts\NUL mkdir .\Package_PC\Assets\Fonts
copy .\AlternateExperience\x64\%CONFIG%\AlternateExperience.exe .\Package_PC
copy .\AlternateExperience\x64\%CONFIG%\Assets\Fonts\*.* .\Package_PC\Assets\Fonts
copy .\AlternateExperience\Assets\*.* .\Package_PC\Assets

@IF NOT EXIST .\output\NUL mkdir .\output
makepkg pack /f chunks.xml /d ./Package_XboxOne /pd ./output
makepkg pack /f chunks.xml /d ./Package_Scarlett /pd ./output
makepkg pack /pc /f chunks.xml /d ./Package_PC /pd ./output