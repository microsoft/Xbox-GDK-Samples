  ![](./media/image1.png)

#   Multisampling Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

The sample renders geometry which is subject to aliasing, and it allows
the user to observe the effect of all supported MSAA modes on that
geometry. An adjustable zoom rectangle shows a close-up view of a small
area of the screen. There are four zoom modes:

1.  Resolved. Pixels are drawn as their resolved colors, using
    point-sampling.
2.  Nearest sample. Pixels are subdivided into shapes indicating the
    area of influence of each sample.
3.  Single sample. Pixels are drawn as the color of the selected sample,
    using point-sampling.
4.  All samples. Pixels are drawn as a rectangular array of all sample
    values.

Similar visualizations are also available within the PIX Pipeline view
when an MSAA render target is selected.

## EQAA

Multisampling modes with a 'Quality' setting of 0 are required to use
the Direct3D standard sample locations, and to behave in a consistent
manner across all graphics cards. Multisampling modes with 'Quality'
setting greater than 0 are hardware-specific. They can use non-standard
sample locations, and they can use additional calculation and/or hidden
additional samples. For instance, with a sample count of two and a
quality setting of zero, any Direct3D 12 class GPU renders only one
intermediate color along an edge between two solid-colored regions. With
a sample count of two and a quality setting of four, however, the Xbox
One GPU renders three intermediate colors. In this case, the two
individual sample colors do not fully represent the information used by
the hardware to compute the final pixel color.

The term EQAA (Enhanced Quality Anti-Aliasing) refers to AMD's
proprietary implementation of MSAA "Quality" levels greater than 0. The
XFest 2012 presentation [Color Compression and Multisampling on Xbox
One](https://developer.xboxlive.com/en-us/platform/documentlibrary/events/Pages/Xfest2012.aspx)
describes some of the details of EQAA. An EQAA surface has the higher
quality for a given memory footprint than a standard MSAA surface, but
it also is slightly more expensive in performance.

The Xbox One implementation of EQAA is not compliant with standard
Direct3D in these ways:

1.  ResolveSubresource is not supported. A title must resolve an EQAA
    surface manually.
2.  Sampling via a view of type `D3D12_SRV_DIMENSION_TEXTURE2DMS` is not
    supported. A title must decode an EQAA surface manually.

The sample demonstrates how to decode and resolve EQAA surfaces. Both of
these tasks utilize an auxiliary "FMask" surface, which is part of the
metadata associated with a compressed color target. Xbox One Direct3D
allows titles to build a shader resource view of the FMask surface by
requesting one of the built-in, non-standard
`D3D12XBOX_DATA_FORMAT_FMASK*` formats.

The "Nearest sample" visualization distinguishes between the API-visible
samples (larger white dots) and the "extra" EQAA samples (smaller black
dots). EQAA samples can be "unknown" (meaning they have no associated
color), and such samples are marked by a crosshatch pattern.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

## Controls

| Action                                        |  Gamepad              |
|-----------------------------------------------|----------------------|
| Change which scene is rendered                |  LB/RB Button         |
| Change MSAA sample count                      |  Dpad L/R             |
| Change MSAA quality                           |  Dpad U/D             |
| Cycle visualization mode (in the rectangular inset) |  A Button |
| Cycle selected sample (for the single sample visualizer) |  B Button |
| Change which light is selected                |  X Button             |
| Change the function of the Right Stick        |  Y Button             |
| Adjust Zoom, Camera, or Light                 |  Right Stick          |
| Reset Zoom, Camera, and Light                 |  R3 Button            |
| Cycle among manual resolve shaders            |  L3 Button            |

## ![](./media/image3.png)

# 

# 

# 

# 

# 

# 

# 

# Known Issues

none

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
