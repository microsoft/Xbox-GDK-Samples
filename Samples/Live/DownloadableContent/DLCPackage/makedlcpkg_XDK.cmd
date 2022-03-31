@IF NOT EXIST .\output\NUL mkdir .\output
"%DurangoXDK%bin\makepkg.exe" pack /f chunks.xml /d ./Package_XDK /pd ./output /productid 4a505039-434a-3057-c050-435757347c00
