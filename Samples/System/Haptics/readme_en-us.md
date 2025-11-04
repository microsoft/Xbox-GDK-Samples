# Advanced Haptics Sample

# Description

This sample demonstrates how to use the Advanced Haptics feature provided by the GameInput API.

# Building the sample

Build with Visual Studio 2022 (or greater).

This sample uses the [GameInput NuGet package](https://www.nuget.org/packages/Microsoft.GameInput) which
will be restored during the build process.  Also ensure the GameInput Redistributable is installed as
described below.

# Running the sample

Ensure that the GameInput runtime redistributable is installed.  This can be found in the NuGet package, or
it can be installed via WinGet by running `winget install Microsoft.GameInput`.

After starting the title, any connected controllers will be listed.  Newly added/removed controllers will
update the UI, and any controllers with haptic capabilities will show a more detailed view allowing the
selection of various haptic effects with playback via WASAPI or XAudio2.  Multiple devices can be connected
with individual playback occuring on each.

# Implementation notes

This sample demonstrates how to be notified when a controller is plugged in, when haptics are ready, if
available, and how to write audio data to the haptic endpoints using WASAPI and XAudio2.

# Known Issues/Expectations

At present, the only haptics-enabled controller supported by GameInput is the PlayStation®5 DualSense® controller.

# Update history

- 10/2025 -- Initial release
