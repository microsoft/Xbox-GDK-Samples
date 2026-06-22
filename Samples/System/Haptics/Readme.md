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
available, and how to write audio data to the haptic endpoints using WASAPI and XAudio2.  Haptics-related code
is located in the reusable library located in the `HapticsManager` directory.

# Haptics Manager

This sample has separated out much of the haptics code into a reusable class that can be integrated into other 
projects. To use, copy the HapticsManager.h and HapticsManager.cpp files into your project along with the Audio 
subdirectory.  Then...

- After the call to `GameInputCreate`, create an instance of `HapticsManager` and call `Initialize`, passing the
created `IGameInput` instance:
```c++
ComPtr<IGameInput> g_gameInput {};
std::unique_ptr<HapticsManager> g_hapticsManager;
...
g_hapticsManager = std::make_unique<HapticsManager>();
if (FAILED(g_hapticsManager->Initialize(g_gameInput.Get())))
{
    // handle error
}
```
- To write audio data to a haptic-enabled controller, call `GetHapticsDevice` with the `GameInputDevice` to retrieve
the `HapticsDevice` wrapper, then call one of the `Play` methods:
```c++
const HapticsDevice* hapticsDevice = g_hapticsManager->GetHapticsDevice(giDevice);

// if the device doesn't have haptic capabilities, hapticsDevice will be nullptr
if (hapticsDevice)
{
    // load and play a WAV file
    hapticsDevice->PlayWAVFile("filename.wav", HapticPlaybackEngine::XAudio2);

    // OR play a pre-loaded WAV file
    hapticsDevice->PlayWAVData(&wavData, wavDataSize, HapticPlaybackEngine::XAudio2);
}
```
- To stop playing audio data, call the `Stop` method:
```c++
    hapticsDevice->Stop();
```

# Known Issues/Expectations

At present, the only haptics-enabled controller supported by GameInput is the PlayStation®5 DualSense® controller.

# Update history

- 01/2026 -- Updated with HapticsManager
- 10/2025 -- Initial release
