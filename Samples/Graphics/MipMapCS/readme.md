  ![](./media/image1.png)

#   MipMapCS Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# 

# Description

This sample demonstrates how to create mips for a texture using a
compute shader avoiding thread local storage in a multi-pass approach.
It also demonstrates how to create mips using AMD FidelityFX SPD in a
single pass approach using LDS and wave intrinsics.

![](./media/image2.jpeg)

# Building the Sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using the Xbox Series X|S devkit, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

| Action                      |  Gamepad                                |
|-----------------------------|----------------------------------------|
| Toggle mips vs. single display |  A button |
| Toggle between multi-pass and FFX SPD |  Y button |
| Toggle between FP16 and FP32 shaders |  X button |
| Previous or next mip level  |  D-pad down or up                       |
| Move the camera             |  Right stick                            |
| Zoom in or out              |  Left or right trigger                  |
| Exit                        |  View Button                            |

# Implementation notes

In multi-pass mode the shader can generate 4 mips in one dispatch. If
the generated mip has odd dimensions (except 1) in any direction, it is
better to start a new dispatch with that mip, otherwise we don\'t
consider all the values while generating the mip. For odd sized
textures, the first mip generated considers multiple samples so that
undersampling doesn't occur.

With AMD FidelityFX SPD the shader generates all mips in one dispatch
using a combination of LDS and wave intrinsics. A more detailed
explanation and original source code can be found
[here](https://github.com/GPUOpen-Effects/FidelityFX-SPD). The version
used in this sample has been updated to use Xbox intrinsics.

Both approaches support FP16 versions of the shaders on Scarlett which
make use of FP16 packed math and reduce VGPR usage by packing two FP16
values into one register.

# Known issues

The shader is implemented using Xbox intrinsics, but should be updated
to use WaveIntrinsics with Shader Model 6 (which should work on at least
a subset of Desktop systems).

# Update history

June 2020: Initial release for GameCore on Xbox

February 2023: Add support for AMD FidelityFX SPD and FP16 versions of
the shader

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
