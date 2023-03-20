# Simple Texture Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

# Description

This sample demonstrates how to render a simple textured quad using
Direct3D 12.

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using an Xbox Series X|S devkit, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample has no controls other than exiting.

# Implementation notes

The texture is loaded here using a simple helper that uses the Windows
Imaging Component (WIC) and is designed for simplicity of learning. For
production use, you should look at the DirectX Tool Kit's
[DDSTextureLoader](https://github.com/Microsoft/DirectXTK12/wiki/DDSTextureLoader)
and
[WICTextureLoader](https://github.com/Microsoft/DirectXTK12/wiki/WICTextureLoader).

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
