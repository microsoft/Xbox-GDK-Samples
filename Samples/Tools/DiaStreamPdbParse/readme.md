  ![](./media/image1.png)

#   DiaStreamPdbParse Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This sample shows how to use the DIA (Debug Interface Access) SDK to
resolve symbols within your title, especially from call stacks generated
during an exception. It also shows how to use the IStream interface to
control PDB data loading from the disk. Lastly it provides an Xbox
version of the DIA DLL named msdia140-xbox.dll.

The Xbox version of DIA uses HeapAlloc to create the BSTR objects that
are returned to the caller. They need to be freed by the caller through
HeapFree, a helper class named DIAString is provided in symbolLookup.cpp
that does the correct thing across both Desktop as well as console. The
DIAString class can be used as a drop-in replacement for BSTR when using
the DIA APIs.

The reason on the Xbox for using HeapAlloc/HeapFree for the BSTR objects
is because the standard BSTR memory management method through
SysAllocString/SysFreeString is not available to the title. DIA query
functions would return BSTR that had been allocated through
SysAllocString, but the title would have no way to free that memory.

The Xbox version of DIA also does not include references to the
Shlwapi.dll which does not exist on the console.

Note: You are free to use the version of the DIA DLL (msdia140-xbox.dll)
that ships with this sample in your retail and development title
environments. However, currently the Xbox version is NOT supported by
the Visual Studio team. If you have issues with it, please reach out
through the Xbox forums for assistance.

# Building the sample

This sample supports the following platforms

|Platform|Notes|
|---|---|
|x64|Using the version of DIA installed as part of Visual Studio.|
|Gaming.Scarlett.xbox.x64<br />Gaming.XboxOne.xbox.x64|Using the msdia140-xbox.dll on the console. It's automatically copied to the console as part of the build process.|

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample will automatically capture the current call stack, resolve
the symbols for each function, and display the results on the screen.

# Implementation notes

The DumpCallstack function in symbolLookup.cpp is the main entry point
for this sample. It can take either a specific thread or a reference to
an `EXCEPTION_POINTERS` object for the call stack to parse.

At the top of symbolLookup.cpp are several defines that control the
amount and type of symbol information to collect. Certain symbol
information can be expensive to collect, especially on the rotational
drive in an Xbox One Family console.

PdbMemoryStream.cpp/h contains IPdbMemoryStream which is derived from
IStream. This class is provided to DIA to replace its default PDB
loading method. Use of this class allows much finer control in the
amount of memory used as well as the read patterns to minimize the
impact on the title.

# Update history

Initial release March 2022

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
