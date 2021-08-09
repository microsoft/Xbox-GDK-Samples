echo Waiting for a Title process to start
xbconnect /wt

echo Copy over the current debug version of the tool to the default console
xbcp /x/title Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\*.exe xd:\DumpTool\
xbcp /x/title Gaming.Xbox.XboxOne.x64\Layout\Image\Loose\*.dll xd:\DumpTool\

echo Running the tool with args: %*
xbrun /x/title /O d:\DumpTool\DumpTool.exe -pdt:triage %*
