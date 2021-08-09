@echo off
rem CreateInstallPackage.bat
rem
rem Creates an install package for the IntelligentDelivery sample. it fills the package with 
rem large filler files so the streaming process takes long enough to be observed and tested.
rem 
rem Advanced Technology Group ( ATG )
rem Copyright (C) Microsoft Corporation. All rights reserved.

@IF NOT EXIST .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\NUL GOTO NOLOOSEDIR

echo Creating a 1K filler file to be used to test streaming install. 
echo/|set /P ="1234567890abcdef" > filler.txt
for /L %%a in (1,1,63) do ( echo/|set /P ="1234567890abcdef" ) >> filler.txt

echo Creating larger filler files from the original 1K filler file to be used to test streaming install. 
copy /b filler.txt + filler.txt + filler.txt + filler.txt filler4.txt
copy /b filler4.txt + filler4.txt + filler4.txt + filler4.txt filler16.txt
copy /b filler16.txt + filler16.txt + filler16.txt + filler16.txt filler64.txt
copy /b filler64.txt + filler64.txt + filler64.txt + filler64.txt filler256.txt
copy /b filler256.txt + filler256.txt + filler256.txt + filler256.txt filler1024.txt
copy /b filler1024.txt + filler1024.txt + filler1024.txt + filler1024.txt filler4096.txt
copy /b filler4096.txt + filler4096.txt + filler4096.txt + filler4096.txt filler16384.txt
copy /b filler16384.txt + filler16384.txt + filler16384.txt + filler16384.txt filler65536.txt
copy /b filler65536.txt + filler65536.txt + filler65536.txt + filler65536.txt filler262144.txt

echo Creating 17 copies of the 256MB filler file to be used as filler across 8 different chunks
echo on
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb1.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb2.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb3.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb4.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb5.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb6.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb7.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb8.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb9.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb10.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb11.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb12.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb13.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb14.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb15.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb16.txt
copy /b filler262144.txt .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\filler256mb17.txt
@echo off

echo Copying over the MicrosoftGame.config file for the Xbox One device family
copy /b MicrosoftGame_XboxOne.config .\Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\MicrosoftGame.config

echo Deleting temporary filler files
del filler.txt
del filler4.txt
del filler16.txt
del filler64.txt
del filler256.txt
del filler1024.txt
del filler4096.txt
del filler16384.txt
del filler65536.txt
del filler262144.txt

GOTO END

:NOLOOSEDIR
echo You must build the sample before running this batch file.

:END 