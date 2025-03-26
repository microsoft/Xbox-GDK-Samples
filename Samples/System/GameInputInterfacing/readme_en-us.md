  ![](./media/image1.png)

#   GameInputInterfacing Sample

*This sample is compatible with the Microsoft Game Development Kit (June 2020)*

# Description

This sample demonstrates how to effectively interface and read inputs
from a gamepad, arcade sticks, racing wheels, and more in the GDK

# Building the sample

- If building for Xbox One, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.
- If building for Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.
- If building for PC, set the active solution platform to Gaming.Desktop.x64.\
**NOTE: This requires the [GameInput NuGet package](https://www.nuget.org/packages/Microsoft.GameInput) and its 
included redistributable to be installed.  Please see [GameInput on PC](https://learn.microsoft.com/gaming/gdk/_content/gc/input/overviews/input-nuget) for more information.**.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Every supported input device connected will be listed onscreen along
with panels for each method of reading its input. Provide input to see
the readings.

# Implementation notes

This sample demonstrates how to use new GameInput API to read input
from devices in a wide variety of ways.

Note that motion and touch are PC only features. Mouse and keyboard will
be available in a future update.

# Version History

- March 2025: Added support for GameInput v1 on PC via the
  [GameInput NuGet package](https://www.nuget.org/packages/Microsoft.GameInput)

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
