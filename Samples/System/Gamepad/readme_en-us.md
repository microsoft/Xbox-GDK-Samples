![](./media/image1.png)

# Gamepad Sample

*This sample is compatible with the Microsoft Game Development Kit (June 2020)*

# Description

This sample demonstrates how to read inputs from a gamepad on the Xbox One and PC

![](./media/image3.png)

# Building the sample

- If building for Xbox One, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.
- If building for Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.
- If building for PC, set the active solution platform to Gaming.Desktop.x64.\
**NOTE: This requires the [GameInput NuGet package](https://www.nuget.org/packages/Microsoft.GameInput) and its 
included redistributable to be installed.  Please see [GameInput on PC](https://learn.microsoft.com/gaming/gdk/_content/gc/input/overviews/input-nuget) for more information.**.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Press buttons to see them displayed and move the thumbsticks and triggers to see their readings.

# Implementation notes

This sample demonstrates how to use the new GameInput API to read input
from a gamepad, including buttons, thumbsticks and triggers.

# Version History

- October 2018: Initial GDK release
- February 2020: Updated for changes to GameInput API.
- June 2022: Added support for GameInput on PC (June 2022 GDK or later)
- March 2025: Added support for GameInput v1.x on PC via the
  [GameInput NuGet package](https://www.nuget.org/packages/Microsoft.GameInput)

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).
