  ![](./media/image1.png)

#   VRS and Sparse Lighting Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# 

# Description

This sample contains a more optimized shader for generating a VRS
shading rate image than the previous VRS sample.

This sample implements sparse deferred lighting. It renders the
g-buffers with VRS. It then builds a buffer of pixels to light, avoiding
the redundancy of lighting adjacent pixels from the same triangle and
with the same g-buffer values. This subset of pixels is lit and the
values copied horizontally, vertically or diagonally. The sample
demonstrates significant GPU savings both from VRS and from Sparse
Lighting.

The sample presents two different methods of calculating the subset of
pixels to light, using compressed depth plane equations or using
SV_Coverage and SV_ShadingRate. This latter method requires 4 bits of
g-buffer space and is required for 'expanded' depth tiles, or for PC.

The sample demonstrates reducing the total number of waves required in
the deferred lighting pass, the significant cost of which is ray
marching soft shadows. Other sparse passes are possible using the same
technique and buffers, including sparse SSR, SSAO, SSGI, screen space
shadow mask, fog & etc.. The sample implements 'rotating' which pixel is
lit per frame. This may be useful when combined with TAA, to converge on
the ground truth.

Finally, the sample demonstrates 'deblocking', a method of removing 'VRS
squares' as a post process.

# ![A picture containing text, mountain, nature, highland Description automatically generated](./media/image2.jpeg)

#  Building the sample

If using Project Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

This sample does not support Xbox One or PC.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

| Action                           |  Gamepad                           |
|----------------------------------|-----------------------------------|
| On screen UI options             |  DPad                              |
| Control sun                      |  Y Button and Shoulder Buttons     |
| Modify shading rate tolerance    |  Trigger Buttons                   |
| Camera                           |  Sticks                            |
| Hide UI                          |  X Button                          |
| Exit                             |  View Button                       |

# 

# Implementation notes

The screen is divided into 16x16 tiles, each has a count (R8 entry in a
texture) and up to 224 dwords of payload data per tile. A payload dword
consists of a tile relative x/y coordinate at 4 bits each, a 21bit unorm
depth and 3 bits for whether to copy the end result of the lighting
calculation horizontally, vertically or diagonally. If the number of
lighting calculations in a tile is greater than 224, no wave32 could
early out and therefore all 256 pixels are lit in the conventional
manner.

Enough waves are dispatched to light every pixel, the win comes when
waves get to immediately early out on a tile due to a reduce pixel
count. Each wave with work processes pixels within the same 16x16 tile,
meaning that light lists are still scalar to the wave, an important
optimization.

Deferred lighting shaders are trivially modified to support sparse
lighting by calling SparseLightingPrefix84 at the start of the shader
and SparseLightingPostfix at the end of the shader.

The shaders of most interest will likely include:

1.  GenerateShadingRateCS.hlsl

2.  DepthjDecompressAndBuildSparseBuffersCS.hlsl

3.  SparseLightingCommon.hlsli

4.  LightingShader.hlsli

5.  Deblock.hlsli

6.  SparseLightingShowPixelCopiesCS.hlsl may also be interesting to
    study as a debug shader

The sample also implements a depth decompression computer shader that
takes compressed reverseZ and outputs linear forward Z, see
DepthDecompress.hlsl.

Further detail on this sample can be found on our [Microsoft GameDev
YouTube talk](https://www.youtube.com/watch?v=Sswuj7BFjGo):

# Known issues

Shading rate determination is in some cases temporally unstable, and
this can be made worse by the post deblocking process to remove VRS
squares. Ordinarily TAA would converge on the ground truth, hiding this
slight instability, but the sample does not have a TAA implementation.

# Update history

Initial release June 2021

Updated November 2021 to include:

1.  Added support for new luminance curves trialed with ID tech

2.  Added depth discontinuity check option with UI toggle

3.  Added better pixel rotation now also including case where 3/4 of the
    pixels of a 2x2 block of pixels are covered, optimised by
    pre-computing copy codes on the CPU

4.  Entirely new edge detection filter which does not use Sobel, so
    dropped all references to Sobel

5.  Fixed non determinism bug

6.  Optimisations worth 145us in the opening scene

7.  Added better code sharing between shaders computing sparse buffers
    either from compressed depth or as a stand alone shader using
    coverage only

Updated June 2022 with:

1.  Gears5 optimisation, to compute data only within a tile, and not
    load border pixels. This significantly simplified the shader

2.  Easier portability of shading rate determination shader to PC, with
    variable tile sizes

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
