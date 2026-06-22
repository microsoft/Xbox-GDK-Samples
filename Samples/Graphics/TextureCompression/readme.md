  ![](./media/image1.png)

#   TextureCompression Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2021)*

# Description

This sample demonstrates how to use the new hardware decompression unit
available on the Scarlett platform for texture data.

# Building the sample

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

This sample does not support Xbox One.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

![A close up of a logo Description automatically generated](./media/image3.png)

The sample comes with several example textures to illustrate what kind
of compression ratios are possible to archive for different BCn
encodings. You can flip through the textures and mip levels using the
controller:

| Action                                 |  Gamepad                     |
|----------------------------------------|-----------------------------|
| Next texture                           |  Dpad up                     |
| Previous texture                       |  Dpad down                   |
| Increase mip level                     |  Dpad right                  |
| Decrease mip level                     |  Dpad left                   |
| Exit                                   |  View Button                 |

# Implementation notes

The hardware decompression unit is pipelined with the NVMe controller
using the Direct Storage API. By enabling decompression on the requests,
data streamed from the NVMe drive is automatically piped through the
decompression unit before being written to memory. There is no need for
intermediate buffers or synchronization in your program to make this
happen. Please see the Direct Storage overview in the GDK documentation
for further information.

The .xbtc textures present in the sample have been compressed offline
using the new XBTC compression tool from the GDK. This tool consumes DDS
files and performs further compression using both the Deflate (i.e.
Zlib) algorithm and the proprietary BCPack algorithm specially designed
for BCn-encoded texture data. Please see the Xbox Texture Compressor
overview in the GDK documentation for further information.

# Known issues

There is currently no support for the PRT tile format which xbtc files
possibly can output (see --tilemode option in the XBTC.exe tool).

Due to a bug in the XG library, the sample xbtc files have been
compressed with the Standard tile mode (\_S). A later version of the
sample will enable support for the native (\_D) tile modes (see
--tilemode in the XBTC.exe tool).

All XBTC compression is currently non-destructive. As outlined in the
XBTC roadmap documentation, we will introduce options to trade quality
for size in a future release of the GDK.

# Update history

-   February 2020: Preview release

-   June 2021: Sample assets updated in response to XBTC format change.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
