@IF "%1" == "" (SET CONFIG=Debug) ELSE (SET CONFIG=%1)
@echo using %CONFIG% build.
cd .\DLCPackage
call makedlcpkg.cmd
call makedlcpkg_XDK.cmd
cd ..\DLCPackagePC
call makedlcpkg.cmd
cd ..\DLCExePackage
call makedlcpkg.cmd %CONFIG%
cd ..\DLCExePackagePC
call makedlcpkg.cmd %CONFIG%
cd ..\DLCDllPackage
call makedlcpkg.cmd %CONFIG%
cd ..\DLCDllPackagePC
call makedlcpkg.cmd %CONFIG%
cd ..
