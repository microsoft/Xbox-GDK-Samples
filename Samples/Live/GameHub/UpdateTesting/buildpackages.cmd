@echo off
@IF NOT EXIST .\output\NUL mkdir .\output
for %%i in (Related Required) do (
copy /y .\GameHub%%iGameV1.mgc ..\GameHub%%iGame\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\MicrosoftGame.config
makepkg pack /f ..\GameHub%%iGame\chunks.xml /d ..\GameHub%%iGame\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose /pd .\output
copy /y .\GameHub%%iGameDLCV1.mgc ..\GameHub%%iGame\DLCPackage\Package_Scarlett\MicrosoftGame.config
makepkg pack /f ..\GameHub%%iGame\DLCPackage\chunks.xml /d ..\GameHub%%iGame\DLCPackage\Package_Scarlett /pd ./output
copy /y .\GameHub%%iGameV2.mgc ..\GameHub%%iGame\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\MicrosoftGame.config
makepkg pack /f ..\GameHub%%iGame\chunks.xml /d ..\GameHub%%iGame\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose /pd .\output
copy /y .\GameHub%%iGameDLCV2.mgc ..\GameHub%%iGame\DLCPackage\Package_Scarlett\MicrosoftGame.config
makepkg pack /f ..\GameHub%%iGame\DLCPackage\chunks.xml /d ..\GameHub%%iGame\DLCPackage\Package_Scarlett /pd ./output
)
