  ![](./media/image1.png)

#   Variable Rate Shading Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# 

# Description

Both Anaconda (Xbox Series X) and Lockhart support tier2.x Variable Rate
Shading. This is a super set of tier2 VRS documented here:
<https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html>

The technique reduces pixel shader load by invoking the pixel shader
less than once per pixel and broadcasting the result to multiple pixels.
This is particularly attractive for forward rendering, including
transparencies but also benefits deferred renders.

This sample renders the exact same scene twice per frame, timing both
and reports the saving due to VRS. The scene rendered with VRS derives
the shading rate from the previous frame rendered with VRS and not the
ground truth. Debug modes are available to view the ground truth, scene
rendered with VRS, the absolute difference and the shading rate used.

Raymarched shadows and AO are used to give the pixel shader some useful
work to do.

# ![](./media/image3.png)

#  Building the sample

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

This sample does not support PC or Xbox One.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

| Action                           |  Gamepad                           |
|----------------------------------|-----------------------------------|
| Change debug mode                |  DPad left/right                   |
| Control sun                      |  Y Button and Shoulder Buttons     |
| Modify shading rate tolerance    |  Trigger Buttons                   |
| Camera                           |  Sticks and DPad up/dowm           |
| Change Visualisation             |  X Button                          |
| Cycle Shading Rate Generation Shaders |  B Button |
| Exit                             |  View Button                       |

# 

# Implementation notes

The API surface for VRS is very small. The main items of interest are:

`Terrain::RenderVRS` -- which makes the calls to RSSetShadingRate and
RSSetShadingRateImage

`Terrain::CalculateShadingRat` -- this issues the compute shader which
generates the shading rate from the previous frame

And in HLSL, the function `BuiildShadingRate` in GenerateShadingRate.hlsl

Four variants `BuildShadingRate` exist, for wave32 and wave64, working
from full resolution or half resolution data. From our experiments
wave64 is faster, and half resolution may be recommended.

# Known issues

The sample has not yet been tested on Lockhart hardware.

# Update history

Initial release January 2020

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
