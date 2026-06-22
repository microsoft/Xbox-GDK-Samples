  ![](./media/image1.png)

#   PrimeHTile Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

This sample demonstrates priming the HiZ part of the HTile with depth values, enabling subsequent rendering to fail the depth test pre-pixel shading. This is particularly relevant for titles which already have a CPU rasterized depth buffer for occlusion culling, enabling the same data to be used to speed up the GPU. This could be useful to reduce overdrawn during g-buffer lay down, however it may be of particular interest to titles using Forward+ which experience a heavier penalty for each pixel overdrawn.

Instead of using CPU rasterized depth, the sample uses the GPU to render depth at the HTile's resolution and still nets an overall saving in GPU time.

The sample renders a fractal of cubes, allowing the user to vary load
between being pixel shader limited and vertex shader limited. It also
implements parallax mapping and allows the user to toggle between this
and bump mapping, to show the effect of pixel shading expense.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

![](./media/image3.png)

| Action                                |  Gamepad                      |
|---------------------------------------|------------------------------|
| Toggle HTile pre-population           |  A button                     |
| Toggle Parallax / Bump Mapping        |  B button                     |
| Toggle Stencil Support                |  X button                     |
| Cycle Per Sample Bias                 |  Y button                     |
| Toggle Rotation                       |  Left/right shoulder button   |
| Occlusion Render Scale                |  Left thumbstick              |
| Rendering Scale                       |  Right thumbstick             |
| Change Fractal Iterations             |  D-Pad up/down                |
| Change Fractal Iterations Used In Occlusion |  D-Pad left/right |
| Change depth bias applied on HTile population |  Left/right triggers |
| View controller help                  |  Menu button                  |
| Exit                                  |  View Button                  |

# Implementation notes

HTile stores a min and max depth per 8x8 tile. When pre-pixel shader
depth culling is enabled, each triangle is coarsely rasterized at the
HTile's resolution. Any fragment which is beyond the far depth is
culled, any nearer than the near depth is trivially accepted and any
that lie in the range between the HTile's near and far depth must
undergo full resolution depth testing.

The sample uses the GPU to render an occlusion buffer at the resolution
of the HTile (\~1/8th full resolution), and to write these depths into
the HTile. As depth buffer and HTile swizzling is different, swizzling
is done in software by the HTile priming shader. HTile priming could be
done by the CPU, although typically HTile might want to be in ESRAM, so
a copy operation would still be needed.

The technique can be combined with traditional depth pre-pass. For
example:

1.  Bind HTile resolution depth buffer

    a.  Render fill limited occluders (say slab sides for buildings)

2.  Bind full resolution depth

    a.  Prime HTile using occlusion buffer

    b.  Render vertex limited occluders (say, the player's car)

Pre-generated depth buffers, software or hardware rasterized only have a single depth value and do not produce a range. The only sensible option therefore is to set both the HTile's near and far depths to the same value. This value however does not necessarily correctly represent the geometry that will be rendered later. The problem is somewhat analogous to shadow acne, where under sampling means that pixels are incorrectly rejected. The sample demonstrates three different methods of addressing this issue, each of which can be used in conjunction:

1. Adding an integer bias to the depth encoding

2. Making the occlusion geometry conservative, by shrinking it

3. During HTile encoding, taking a max of the 3x3 depth values. (This is very effective)

None of these methods provide a perfect fix on their own but a satisfactory result can be found. The different methods have different performance trade-offs.

HTile priming prevents the GPU from using depth compression, the reduction in overdraw should however more than offset this loss. As an aside, any depth buffer which has had its HTile primed does not need to be cleared, this comes for free.

It is possible that instead of using the graphics engine, or CPU to
render the occluding depth buffer, titles might want to use async
compute. See <https://github.com/jbarczak/CSRasterization>.

This sample is implemented using DirectX 12.X. The same shaders
(PrimeHTileCS.hlsl) can be used for a DirectX 11.X implementation if you
remove the root signatures.

# Update history

The original version of the sample was written using the XSF-based
framework. It was rewritten to use the ATG sample templates in March
2018 with support for Xbox One X using DirectX 12. In March 2024 we added
native support for Xbox Series X|S as previously it run in backward-compatible
mode.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
