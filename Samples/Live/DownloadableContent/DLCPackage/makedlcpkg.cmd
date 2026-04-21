@IF NOT EXIST .\output\NUL mkdir .\output
makepkg pack /f chunks.xml /d ./Package_XboxOne /pd ./output
makepkg pack /f chunks.xml /d ./Package_Scarlett /pd ./output
makepkg pack /pc /f chunks.xml /d ./Package_PC /pd ./output