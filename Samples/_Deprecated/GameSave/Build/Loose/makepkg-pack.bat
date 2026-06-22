@echo off
md ..\..\AppPackages\GameSaveXDK > NUL
IF [%1] EQU [] (
  makepkg.exe pack /f Chunks.xml /d . /pd ..\..\AppPackages\GameSaveXDK /productid 9c020119-1eca-46fe-99dd-84d3f0e2ecb0
) ELSE (
  makepkg.exe pack /f Chunks.xml /d . /pd ..\..\AppPackages\GameSaveXDK /productid 9c020119-1eca-46fe-99dd-84d3f0e2ecb0 /contentid %1
)

