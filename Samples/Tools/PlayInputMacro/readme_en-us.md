  ![](./media/image1.png)

# PlayInputMacro Sample

## Description

This sample demonstrates how to play back recorded input recorded in the Xbox One Macro recorder to any console, primarily for automated testing.

## Building the sample

To build the sample, ensure the .NET 8 SDK (or greater) is installed, load the solution in VS2022 (or greater), and build.

## Using the sample

Ensure the .NET 8 runtime is installed on the machine from which this tool will be used.  Alternatively, you can build this
tool as a standalone release and publish as a single executable.

This tool has a dependency on `XtfInput.dll` and `xbtp.dll` included with the GDK, so ensure the GDK is installed on the target machine and the bin directory is included
in the environment path.

Alternatively, copy `XtfInput.dll` and `xbtp.dll` from the GDK's bin directory to the directory containing the PlayInputMacro executable.

### Recording a macro

Use Xbox One Manager's "macro recording" feature to record input using your PC.  For more information, please see the 
[Xbox One Manager documentation](https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/tools-console/xbox-tools-and-apis/xbom/manager-tool-gamepad-input#ID4EXE)
to learn how to record and save the macro file.

### Playing back the input

Once the macro file is saved, you can then run this tool from the command line as follows:

`PlayInputMacro <Console IP> <XML Macro File>`

This will play back the input recorded in the XML macro file against the console specified.

## Implementation notes

This sample uses .NET 8, and the .NET 8 SDK (or newer) must be installed via Visual Studio or separately.  Ensure a matching
.NET Runtime is installed to run the application.  Please see the [.NET 8](https://dotnet.microsoft.com/en-us/download/dotnet/8.0) page for more info.

## Update history

- April 2025: Initial Release

## Privacy Statement

For more information about Microsoft's privacy policies in general, see the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).