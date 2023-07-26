@rem Copy needed files and assets into a staged directory
@IF NOT EXIST key.lekb makepkg genkey /ekb key.lekb
makepkg pack /v /f PackageLayout.xml /d .\DefaultExperience\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose /pd .\DefaultExperience\Gaming.Xbox.XboxOne.x64\Layout\Image  /lk key.lekb
