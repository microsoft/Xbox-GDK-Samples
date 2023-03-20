# Simple MSAA

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

![](./media/image1.png)

# Description

This sample implements an MSAA render target & depth/stencil buffer for
a 3D scene using DirectX 12.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using an Xbox Series X|S devkit, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

| Action                      |  Gamepad                                |
|-----------------------------|----------------------------------------|
| Toggle MSAA vs. single-sample |  A button |
| Exit                        |  View Button                            |

# Implementation notes

The UI is drawn without MSAA, and makes use of an explicit resolve
rather than relying on an implicit resolve of an MSAA swapchain.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
