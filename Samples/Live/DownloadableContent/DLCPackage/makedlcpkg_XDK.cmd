@IF NOT EXIST .\output\NUL mkdir .\output
"%DurangoXDK%bin\makepkg.exe" pack /f chunks.xml /d ./Package_XDK /pd ./output
