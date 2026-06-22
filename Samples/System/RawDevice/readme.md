# RawDevice Sample

*This sample is compatible with the Microsoft Game Development Kit
(April 2021)*

# Description

This sample demonstrates how to read and write raw device messages using
GameInput

![](./media/image1.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Connect an Xbox compatible racing wheel to see the readings. Press the
menu button to send a request to the wheel for a Wheel Static
Configuration report and see the reading that appears.

# Implementation notes

This sample demonstrates how to use the GameInput API to read and write
messages from and to devices that support raw device messages.

# Version History

July 2021: Initial release

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
