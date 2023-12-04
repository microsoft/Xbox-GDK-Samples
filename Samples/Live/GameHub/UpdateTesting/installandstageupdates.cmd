@echo off
for %%i in (output\*100*.xvc) do (
  xbapp install "%%i"
)
for %%i in (output\*200*.xvc) do (
  start cmd /k xbapp update "%%i" /a
)
