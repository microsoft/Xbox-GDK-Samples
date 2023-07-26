![](./media/image1.png)

# LampArray Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2023 QFE1)*

# Description

This sample demonstrates how to use the LampArray API to operate lights in RGB devices, such as keyboards and mice.

> **Please note:** As of the March 2023 QFE1 release, the GDK LampArray API only supports the following devices on console. Support for additional devices will be added in future recovery releases.
> - Razer Turret for Xbox One (keyboard and mouse)
> - Razer BlackWidow Tournament Edition Chroma V2

# Building the sample

If using an Xbox One devkit, set the active solution platform to Gaming.Xbox.XboxOne.x64.

If using an Xbox Series X|S devkit, set the active solution platform to Gaming.Xbox.Scarlett.x64.

*For more information, see* __Running samples__*, in the GDK documentation.*

# Using the sample

Ensure you have a compatible device connected.  Use the keyboard arrow keys or a Gamepad's DPad to move between the sample effects.

Press the Esc key or View button to exit.

# Implementation notes

The effect implementations are found in the `LightingEffects.cpp` file.  The callback and other functionality is located in the `Lighting.cpp` file.

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).
