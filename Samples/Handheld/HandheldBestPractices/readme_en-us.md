# Handheld Device Best Practices

# Description

This sample demonstrates the best practices for running a title on handheld devices, including how to:
- Determine if the device is a handheld
- Get various device properties
- Get integrated display properties
- Handle DPI changes and scale UI
- Display the virtual keyboard for text entry, and get user-entered text
- Determine network and Bluetooth connectivity
- Determine network devices and types
- Determine audio endpoints and currently used audio device
- Handle input from Gamepad, Keyboard and Mouse
- Determine input modality (i.e. last input device used)

# Building the sample

Build with Visual Studio 2022 (or greater) and copy to a handheld device.

**NOTE: This sample requires the [GameInput NuGet package](https://www.nuget.org/packages/Microsoft.GameInput) 
and its included redistributable to be installed.  Please see 
[GameInput on PC](https://learn.microsoft.com/gaming/gdk/_content/gc/input/overviews/input-nuget) for more information.**.

# Running the sample

Ensure the handheld device is running the latest build of Windows 11 24H2 or greater.  Also ensure that
the GameInput runtime redistributable is installed.  This can be found in the NuGet package, or
it can be installed via WinGet by running `winget install Microsoft.GameInput`.

Copy the sample executable to a handheld device and run.  Alternatively, setup the
[Remote Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/?cid=learn-onpage-download-cta#remote-tools-for-visual-studio-2022)
on the handheld device for remote deploy and debug.  For more information, please see:

[Remote Debugging a C++ Project in Visual Studio](https://learn.microsoft.com/en-us/visualstudio/debugger/remote-debugging-cpp?view=vs-2022)

When the sample is run, various properties and details about the device will be displayed.

*Navigation Controls*

- DPad: Move between controls
- A: Activate control
- Y: Activate text input box
- X + LB/RB: Select Window
- X + Left Stick: Move Selected Window
- X + DPad: Resize Selected Window
- View: Exit

# Implementation notes

While the sample is written with DirectX in mind, device information is determined
using standard/generic Win32 APIs whenever possible to ensure wide compatibility.
There may be other ways to accomplish tasks using DirectX and other APIs, and several
of those are shown as alternate examples in the Snippets directory.  Use what is
appropriate for your title.

Wherever possible, device and other information is pulled at application startup and
kept until an incoming notification changes the state.  While some of these properties
could be polled per frame, please use these notifications instead to not impact performance.
Be sure to look at the WM_* message handling as well as network connectivity hints
to see how this works.

While one can determine if a title is running on a handheld device by using the
`RtlGetDeviceFamilyInfoEnum` method, it is encouraged to look at all device properties
and states to make decisions rather than a blanket hardcoding of
functionality behind a form factor check.  This will ensure that future devices, and even devices
that are not not handhelds, such as laptops, tablets, and 2-in-1s, also benefit in UI scaling, 
power consumption, and performance characteristics.

# Known Issues/Expectations

- If a device is in desktop or mouse/keyboard mode, Gamepad inputs will not be recognized. Ensure
  the device is set to Gamepad mode via Armory Crate or other OEM software.
- The Gamepad virtual keyboard requires Windows 11 24H2 26100.3613 or greater.  If the device is running
  an older version of Windows, the sample will not display the virtual keyboard.
- When running on a device that isn't a handheld, you may see errors regarding
  `GetDeviceScreenDiagonalSizeInInches` and `GetDeviceHDRStatus`. These snippets are written
  to only look at integrated displays, which may not exist on non-handheld devices.
- Errors in the input section are expected if the GameInput redistributable is not installed and/or
  no gamepad is present.

# Update history

- 07/2025 -- Initial release
