![](./media/image1.png)

# ShadowMap Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

This sample demonstrates various methods of shadow mapping.

![](./media/image2.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If running on PC, set the active solution platform to `Gaming.Xbox.Desktop.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Controls

| Action                                       |  Gamepad               |
|----------------------------------------------|-----------------------|
| Change Remapping (Filtering) Technique       |  B                     |
| Change PCF Mode                              |  Y                     |
| Change MSAA level (only valid when using Filtering) |  X |
| Change Kernel size (used for PCF and Filtering) |  A |
| Change Shadow Map Dimensions                 |  LB                    |
| Toggle Shadow Culling Mode                   |  RB                    |
| Toggle Shadows on and off                    |  Start                 |
| Move the light forward and backwards         |  DPad Up and Down      |
| Mode the Camera around  |  LT-RT for moving up, down Right Stick to rotate view Left Stick to move translate             |
| Exit the sample.                             |  View Button           |

# Implementation notes

The sample demonstrates the following shadow map techniques:

-   Percentage Closer Filtering (PCF)

-   Variance

-   Exponential

-   Exponential Variance

For simplicity, the sample supports only a single "cascade" or shadow
frustum. Large world shadow maps must usually implement multiple
cascades, for multiple distance scales within the same scene.

For D3D12 hardware, PCF has a built in performance advantage over other
techniques, due to HLSL support for SampleCmp and GatherCmp, which
retrieve four shadow map taps for the price of one.

The other shadow map techniques can be grouped under the category of
"filterable" shadow maps. They are preferable to PCF in some
circumstances, such as when the shadow map is static and can therefore
be pre-filtered offline.

The sample demonstrates three different implementations of PCF, which
yield equivalent results, but have different costs:

-   **SampleCmp step by 1:** The shader uses the SampleCmp HLSL method.
    The shadow map samples are one texel apart. This method is *not
    generally recommended* because the number of samples is
    unnecessarily high.

-   **SampleCmp step by 2:** The shader uses the SampleCmp HLSL method.
    The shadow map samples are two texels apart. In order to achieve the
    desired sample weights, the texture coordinates are perturbed.

-   **GatherCmp:** The shader uses the GatherCmp HLSL method. The shadow
    map samples are two texels apart. The samples are weighted and
    combined manually in HLSL code.

Different shadow map methods have different requirements for culling and
depth bias. PCF, notably, can work with frontface culling, which is
often cheaper than backface culling, since more geometry tends to face
towards the light source than away from it.

The onscreen menu allows mixing and matching of different options. An
option which is grayed out and has the text "-" is not applicable within
the context (for instance, filter mode on a non-filterable shadow map).
An option which is displayed in red with the word "invalid" after it has
a bad value (for instance, depth bias is set to 0 when a positive depth
bias is required).

# Update history

7/9/2022 -- Ported Sample from June 2020 XDK.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
