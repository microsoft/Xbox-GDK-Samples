![](./media/image1.png)

# SimpleFFBWheel Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2023)*

# Description

This sample demonstrates how to use a force feedback steering wheel with the GDK.

# Building the sample

- If building for Xbox One, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.
- If building for Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.
- If building for PC, set the active solution platform to Gaming.Desktop.x64.\
**NOTE: This requires the [GameInput NuGet package](https://www.nuget.org/packages/Microsoft.GameInput) and its 
included redistributable to be installed.  Please see 
[GameInput on PC](https://learn.microsoft.com/gaming/gdk/_content/gc/input/overviews/input-nuget) for more information.**.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Running the sample

Ensure the GameInput Redistributable (GameInputRedist.msi) is installed.  This installer can be found in the NuGet package,
or it can be installed separately using WinGet via a command prompt with the following command:

`winget install Microsoft.GameInput`

# Using the sample

Ensure you have a wheel with force feedback connected.  Use the wheel's pedals or dpad to accelerate and brake.
You should feel the force feedback on the wheel change based on the simulated vehicle's speed.

Additionally, press the Menu button on the wheel simulate temporarily driving over gravel or other rough surface.

# Implementation notes

This sample demonstrates how to use the new GameInput API to read input
from a racing wheel and provide force feedback using the Spring, Damper, and
SawtoothUp force feedback effects.

- The Spring effect is used to return the wheel to center as the vehicle accelerates.
- The Damper effect is used to provide "weight" to the wheel, which increases with speed.
- The SawtoothUp effect is used for the "gravel" effect.  This is a periodic effect which,
for this sample, doesn't take speed into account.  It is a one-shot vibration effect.

Note that the scaling and values used in this sample aren't intended to provide an accurate
vehicle simulation, only to demonstrate how effect parameters can be updated based on the title's
simulation.

Comments marked as `// [SAMPLE]` are specific to the implementation of this sample.

# PC-specific notes

Racing wheels on PC can operate in either GIP mode or HID mode.

If the device is in GIP mode, typically the mode when NO drivers are installed AND its firmware supports Xbox consoles (i.e. it probably has
an Xbox Guide button), no mappings are needed and the device will be automatically detected properly.

If the device is in HID mode, typically the mode when drivers from the manufacturer are installed, Windows needs to know how to map its 
input axes, buttons, and switches to the appropriate GameInput items.  If these mappings to do not exist, the wheel will not be 
detected as a GameInputKindRacingWheel.


# Version History

- March 2025: Added support for GameInput v1.x on PC via the
  [GameInput NuGet package](https://www.nuget.org/packages/Microsoft.GameInput)
- July 2025: Added support for GameInput v2.x on PC via the
  [GameInput NuGet package](https://www.nuget.org/packages/Microsoft.GameInput)

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).
