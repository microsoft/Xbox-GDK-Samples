![](./media/image1.png)

# SimpleFFBWheel Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2023)*

# Description

This sample demonstrates how to use a force feedback steering wheel with the GDK.

# Building the sample

If using an Xbox One devkit, set the active solution platform to Gaming.Xbox.XboxOne.x64.

If using an Xbox Series X|S devkit, set the active solution platform to Gaming.Xbox.Scarlett.x64.

**Note:** The Desktop build is currently not supported due to an issue with racing wheels on PC. 
A future verison of this sample will include Desktop support.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Ensure you have a wheel with force feedback connected.  Use the wheel's pedals or dpad to acclerate and brake.
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

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).
