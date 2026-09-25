<!-----
---
page_type: sample
languages:
- cpp
products:
- gdk
urlFragment: "gamepad"
extendedZipContent:
- path: LICENSE
  target: LICENSE
- path: Kits
  target: Kits
- path: Media
  target: Media
description: "This sample demonstrates gamepad input, motion sensor readings, vibration control, and audio-driven haptics using GameInput on Xbox and Windows PC."
---
----->

# Gamepad Sample

*This sample is compatible with the Microsoft GDK (October 2025) or later. ARM64 requires the April 2026 GDK or later.*

# Description

This sample demonstrates how to read input from gamepads using the GameInput API on
Xbox Series X|S, Xbox One, and Windows PC (x64 and ARM64). It presents a Dear ImGui
interface (using the repository's `ImGuiAtg` framework) with a tab per connected device, and covers:

- **Device enumeration** -- automatic detection of gamepad connect/disconnect via device callbacks
- **Multiple gamepad support** -- each connected gamepad gets its own tab
- **Gamepad state reading** -- buttons, analog triggers, and thumbstick axes
- **Visual analog indicators** -- vertical trigger bars and 2D thumbstick crosshairs with live values
- **Sensor/motion data** -- accelerometer, gyroscope, and orientation quaternion
- **3D controller model** -- real-time orientation visualization driven by sensor data, rendered with DirectXTK12
- **Vibration control** -- interactive sliders and preset effects for all four rumble motors (low/high frequency and the two impulse triggers)
- **Haptic playback** -- WAV-based haptic feedback via WASAPI and XAudio2 on supported controllers
- **Reading modes** -- toggle between polling (`GetCurrentReading`) and callback-driven (`RegisterReadingCallback`) input
- **Device info** -- display name, vendor/product IDs, supported input kinds, and reading timestamp

# Building the sample

- If building for Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.
- If building for Xbox One, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.
- If building for PC, set the active solution platform to `x64` or `ARM64`.

The `Microsoft.GameInput` NuGet package is restored automatically for every platform and
overrides the GameInput version included with the GDK. This sample requires Microsoft.GameInput
3.5 or later and uses the GameInput v3 API.

**NOTE:** On PC, the GameInput runtime must be installed. See
[GameInput on PC](https://learn.microsoft.com/gaming/gdk/_content/gc/input/overviews/input-nuget)
for more information.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Running the sample

On PC, ensure the GameInput runtime is installed. It can be installed from the NuGet package's
redistributable, or via WinGet from a command prompt:

`winget install Microsoft.GameInput`

Connect one or more gamepads. The UI displays a tab for each connected device. Press buttons,
move the thumbsticks, and pull the triggers to see live readings and visual indicators. The
**Vibration** section drives the rumble motors, the **Haptics** section plays WAV-based haptic
effects, and the **Sensors** / **3D Orientation** sections display motion data on supported
controllers.

# Using the sample

Use the on-screen ImGui controls to select the reading mode, configure vibration, and play
haptic effects. Hold LB + RB and press D-pad up/down to move through the currently available
navigation groups. Release LB + RB to navigate within the selected group normally. 
Use LB + RB + D-pad left/right to select the previous or next connected-gamepad tab.

When multiple gamepads are connected, only the most recently connected gamepad controls
Dear ImGui navigation. Navigation input is not merged across gamepads. If that gamepad
disconnects, navigation falls back to the next most recently connected gamepad.

# Controls

| Action | Keyboard & Mouse | Gamepad |
|---|---|---|
| Exit | Alt+F4 | LB + RB + View + Menu |
| Toggle light/dark theme | F2 | LB + RB + Y |
| Toggle gamepad navigation | F3 | LB + RB + X |
| Select previous/next gamepad tab | N/A | LB + RB + D-pad left/right |
| Move between interactive UI groups | N/A | LB + RB + D-pad up/down |
| Toggle fullscreen (PC only) | F11 or Alt+Enter | N/A |

# Implementation notes

- Uses the **GameInput v3** API from the Microsoft.GameInput 3.5 or later NuGet package on both
  PC and Xbox. Device connect/disconnect is handled through
  `IGameInput::RegisterDeviceCallback` with `GameInputBlockingEnumeration` so already-connected
  devices are reported at startup.
- Gamepad state is read via `IGameInput::GetCurrentReading` (polling) or
  `IGameInput::RegisterReadingCallback` (callback mode) with `GameInputKindGamepad | GameInputKindSensors`.
- `GamepadVibration.h` defines the vibration effects and per-gamepad state.
  `GamepadVibration.cpp` calculates each preset directly into `GameInputRumbleParams` and applies it
  via `IGameInputDevice::SetRumbleState`. The sample sends a zeroed rumble state when the title is
  suspended or shut down.
- `GamepadHaptics.h` defines the WAV media item presented by the UI, while `GamepadHaptics.cpp`
  handles media selection and playback. The lower-level `HapticsManager` helper wraps XAudio2 and
  WASAPI for WAV-to-haptic streaming.
- 3D model rendering uses `ImGuiAtg::ModelViewer`, which renders the controller model into an
  offscreen render target oriented by the device's sensor quaternion.
- The Focus Policy section, the custom WAV file picker, and the fullscreen toggle are desktop-only
  and are compiled out on Xbox console.

# Known issues / expectations

- Motion sensor data (accelerometer, gyroscope, orientation) and audio-drive haptics are currently only 
- available on controllers that report sensor and haptic support, such as the DualSense®.
- The 3D controller model requires orientation sensor support; otherwise the section displays an
  unsupported message.

# Update history

- 07/2026 -- Rewritten.  Added a 3D controller model, audio-driven haptics, vibration presets, and ARM64 support.
- 04/2026 -- Added motion sensor readings for supported devices.
- 07/2025 -- Added support for GameInput v2.x on PC via the Microsoft.GameInput NuGet package.
- 03/2025 -- Added support for GameInput v1.x on PC via the Microsoft.GameInput NuGet package.
- 06/2022 -- Added support for GameInput on PC.
- 10/2018 -- Initial GDK release.

# Privacy statement

When compiling and running a sample, the file name of the sample executable will be sent to
Microsoft to help track sample usage. To opt-out of this data collection, you can remove the
block of code in Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see the
[Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).

"DualSense" is a registered trademark or trademark of Sony Interactive Entertainment Inc.