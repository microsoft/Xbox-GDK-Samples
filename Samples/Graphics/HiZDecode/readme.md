  ![](./media/image1.png)

#   HiZDecode Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

# This sample demonstrates how to extract hi z values from the htile surface.

![](./media/image2.jpeg)

# Using the sample

This sample uses the following controls.

| Action                                 |  Gamepad                     |
|----------------------------------------|-----------------------------|
| Move camera toward/away from origin    |  Left Thumbstick Up/Down     |
| Orbit camera                           |  Right Thumbstick            |
| Reset camera                           |  Right Thumbstick (Click)    |
| Toggle stencil                         |  A Button                    |
| Toggle resummarize                     |  B Button                    |
| Toggle depth compression               |  Y Button                    |
| Toggle depth in DRAM vs ESRAM          |  X Button                    |
| Toggle decompress                      |  D-Pad L/R                   |
| Exit                                   |  View Button                 |

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using the Xbox Series X|S devkit, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Implementation notes

The htile surface is a metadata surface associated with a depth-stencil buffer. Each 8x8 tile of the depth-stencil buffer has an associated 32-bit htile entry. The htile entry contains the following pieces of information:

-   The hi z information for the tile, which encodes a conservative minimum and maximum depth

-   The hi stencil information for the tile, which records whether all pixels in the tile agree on certain facts about stencil.

-   Depth compression metadata, which encodes whether the tile contains clear, compressed, or expanded depths

-   Stencil compression metadata, which encodes whether the tile contains clear, compressed, or expanded stencil values

The sample demonstrates how to decode the hi z and hi stencil portion of htile onto textures. These textures could then be used for various purposes including:

-   Accelerate culling of the scene

-   Classification of tiles for lighting

(Those applications are not demonstrated here.)

The sample displays the decoded hi z and hi stencil textures in thumbnail views. The hi z thumbnail has the minimum depth in the red and blue channels and the maximum depth in the green channel. Due to the channel choices, tiles with a larger depth range will have a greener tinge. The hi stencil thumbnail shows the value of hi stencil test 0 in the red channel and the value of hi stencil test 1 in the green channel. On Xbox Series X|S the green channel is always set to 0 due to htile format.

Htile encoding depends on whether stencil is present in the associated depth buffer. When stencil is absent, the minimum and maximum depths are both explicit 14-bit fixed-point values. When stencil is present , one end of the depth range is an explicit 14-bit fixed-point value, and the other end is implicitly given by a 6-bit delta value. The delta uses a custom encoding, described in the shader comments.

There are up to two hi stencil tests, each of tests has three possible outcomes:

1.  No pixels in the tile satisfy the test

2.  All pixels in the tile satisfy the test

3.  Some pixels satisfy the test and some pixels don\'t (or alternately, the value is not up to date)

The driver chooses hi stencil states automatically (as of the February 2015 XDK). You can read hi stencil states in the PIX GPU State view, from the registers: 

`Context - DB - DB_SRESULTS_COMPARE_STATE\<0|1>`

The sample offers the option to perform a "resummarize" pass. This pass takes around 50 us, and it updates all htile values to correspond with the latest depth data. Without the resummarize pass, htile is still valid, but some hi z ranges will be wider than necessary, and some hi stencil values will contain the "don't know" code, even when one of the other codes would be correct.

The cost of resummarize may be amortized by combining it with an existing decompress operation. Decompress typically takes about the same amount of time with or without a resummarize.

# Known issues

This sample depends on certain driver choices, which may change in the future:

-   Whether the htile surface is created as tiled or linear

-   What tiling mode is chosen for the htile surface

Due to driver limitations, the sample does not support use of hi stencil on uncompressed stencil buffers.

# Update history

The original version of this samples was written in 2012. This DirectX
12 version was released in September 2018. The sample was ported to GDK
and Xbox Series X|S in June 2020.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
