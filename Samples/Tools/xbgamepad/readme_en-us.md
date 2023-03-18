# xbgamepad

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This is a minimal win32 console app that takes input from the locally
connected XINPUT device (e.g. Xbox gamepad) and send those inputs to an
Xbox One or Xbox Series X|S devkit using the XTF (Xbox Tools Framework)
libraries that ship with the XDK and GDK.

# Running

It can be run on a machine without a GDK installed as it will use the
dlls in the xtfdlls folder if they are present. The tool requires
Windows 8 or higher.

# Usage

```
xbgamepad /x:<devkit ipv4 address> [/r:<update rate in hz - default is 30>]
```

Access to the devkit via TCP ports 4211 and 4212 is required.

# Build Pre-requisites

-   Visual Studio 2019 (16.11) or Visual Studio 2022

-   Windows 10 SDK

-   Recent GDK install for XTF headers and libraries (project can be
    modified to use XDK by changing the environment variable GameDK to
    the XDK version in the includes and linker inputs)

# Distributing

To run on a machine with no GDK installed you need to copy the xbtp.dll
and xtfinput.dll files from an existing GDK installation. They are in
`%GameDK%\bin`. You can place them side-by-side with xbgamepad.exe on the
machine without an installed GDK.
