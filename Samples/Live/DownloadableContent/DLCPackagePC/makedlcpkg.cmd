@IF NOT EXIST .\output\NUL mkdir .\output
makepkg pack /pc /f chunks.xml /d ./Package /pd ./output
