  ![](./media/image1.png)

#   Gamepad Vibration Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

# 

# Description

This sample demonstrates how to use vibration with a gamepad on an Xbox
One.

![](./media/image3.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

For PC, you can set the active solution platform to Gaming.Desktop.x64.
**This requires the June 2022 GDK or later**.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Use left and right on the DPad to cycle between different vibration
examples. Use the triggers in most of these examples to increase the
amount of vibration.

Please note that there are some 3^rd^ party controllers which do not
have trigger rumble motors, so effects using those motors on these
controllers will not cause vibration.

# Implementation notes

This sample demonstrates how to use the GameInput API to set vibration
levels on an Xbox One gamepad.

# Update history

-   Initial release April 2019

-   Updated in June 2019 for minor breaking change to
    **SetRumbleState**.

-   February 2020: Updated for changes to GameInput API.

-   June 2022: Added support for GameInput on PC (June 2022 GDK or
    later)

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
