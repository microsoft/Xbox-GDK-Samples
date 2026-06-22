rmdir /s /q PC\Package
mkdir PC\Package
xcopy /Y ..\GdkMetadata\* PC\Loose
makepkg genmap /f layout.xml /d PC\Loose
makePkg pack /f layout.xml /d PC\Loose /pd PC\Package /nogameos /pc
