  ![](./media/image1.png)

#   CMaskDecode Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

# This sample demonstrates how to extract fast clear values from the cmask surface.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

![](./media/image2.jpeg)

| Action                                |  Gamepad                      |
|---------------------------------------|------------------------------|
| Toggle MSAA                           |  A button                     |
| Toggle sub-tile display               |  B button                     |
| Rotate view                           |  Left thumbstick              |
| Reset view                            |  Left thumbstick (click)      |
| Exit                                  |  View Button                  |

# Implementation notes

The cmask surface is a metadata surface associated with a render target.
Each 8x8 tile of the render target has an associated 4 bit cmask entry.
The cmask entry contains the following pieces of information:

• If MSAA is not enabled, then each bit of cmask represents the "clear"
state of one 16-pixel sub-tile (the shape of the sub-tile depends on the data format, the micro-tiling mode
on Xbox One and the swizzling mode on Xbox Series X|S, see more detail in the documentation
on [CMask decoding](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/cmask-decoding))

• If MSAA is enabled, then the two high order bits of cmask each
represent the "clear" state of one 32-pixel sub-tile (for exact sub-tile shape, please refer to [CMask decoding](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/cmask-decoding) documentation).
The two low order bits contain fmask metadata.

The sample demonstrates how to decode the cmask onto a texture and how
to interpret the fast clear bits. This information could be used to
accelerate detection of clear areas of the underlying surface. One
application (not demonstrated here) might be to skip the
fast-clear-eliminate of a mostly empty texture.

The sample displays the decoded fast clear information as a red/green
overlay over the main scene. Green pixels belong to tiles (or sub-tiles)
which are entirely clear. Red pixels belong to tiles (or sub-tiles)
which are partially written to.

This sample is implemented using DirectX 12.X. The same shaders
(ColorDecompressUtility.hlsli / CMaskDecodeCS.hlsl) can be used for a
DirectX 11.X implementation if you remove the root signatures.

# Known issues

This sample depends on certain driver choices, which may change in the
future:

• Whether the cmask surface is created as tiled or linear

• What tiling mode (on Xbox One) or swizzling mode (on Xbox Series X|S) is chosen for the cmask surface

# Update history

The original version of the sample was written using the XSF-based
framework. It was rewritten to use the ATG sample templates in February
2018 with support for Xbox One X. Support for Xbox Series X|S was added
in May 2020.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
