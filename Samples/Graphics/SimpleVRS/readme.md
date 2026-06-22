  ![](./media/image1.png)

#   SimpleVRS Sample

*\* This sample is compatible with the Microsoft Game Development Kit
(March 2022).*

# Description

The goal of this sample is to succinctly demonstrate how [Tier 2
screen-space Variable Rate
Shading](https://docs.microsoft.com/en-us/windows/win32/direct3d12/vrs#tier-2)
(VRS) can be used to minimize the cost of expensive pixels shaders with
few lines of code.

Variable Rate Shading (VRS) reduces the total number of pixel shader
invocations by broadcasting a pixel's final value to a portion of its
neighbors (region of influence specified through the VRS D3D12 API).
This is ideal for areas of a scene that appear uniformly shaded or have
far less per-pixel detail.

In this sample, the classic simple triangle, is introduced behind a wall
of moving and semi-transparent cloud and rendered with an artificially
expensive pixel shader to show how VRS shading
rates can reduce heavy pixel shader workloads. Triangle pixels that are
partially occluded by translucent cloud pixels are shaded at
a coarser rate due to their reduced visibility. Performance savings are
highlighted using average elapsed GPU time measurements for the shading
rate image generated and the rendered scene. A visualization mode is
also present to illustrate the assignment of shading rates within the
scene. 

![Chart, surface chart Description automatically generated](./media/image3.png)

# Building the Sample

This sample exclusively runs on Xbox Series X|S consoles. By default,
the active solution platform will be Gaming.Xbox.Scarlett.x64

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

| Action                                   |  Gamepad                   |
|------------------------------------------|---------------------------|
| Enable/Disable VRS                       |  A button                  |
| Toggle Shading Rate Visualization        |  B button                  |

# Implementation notes

The primary purpose of this simple sample is to help users become
familiarized with enabling Tier 2 VRS and focus on performance benefits
specific to VRS. Potential simplifications and optimizations in the rest
of the sample have been omitted to simplify the implementation.

**Summary of Render Passes in SimpleVRS** 

The following steps summarize and contribute to the rendering of each
frame in this sample: 

1.  Clear all render targets. 

2.  Disable any VRS state set previously and render the alpha values of
    cloud sprites to a small buffer. 

3.  Populate the shading rate image based on the grouping of similar
    alpha values and enable VRS with the new shading rate map. 

4.  Render the opaque cloud sprites and the triangle with the
    artificially expensive pixel shader. 

**Important VRS API Components and Functions**

As seen in the sample's AllocateShadingRateImage function, the shading
rate image GPU resource is initialized with the
**D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE** resource state so that a
compute shader can populate it with shading rate values. It must be
created with the **DXGI_FORMAT_R8_UINT** image format,as specified in
the [Direct3D 12 VRS
specification](https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html).
On Scarlett, the tile size is 8x8. This means each shading rate value in
the shading rate image maps to a square of 64 pixels in the full
resolution framebuffer. To fully cover the screen, the shading rate
image dimensions are 1/64^th^ of the framebuffer.

In the Render function (VRS.cpp), VRS is disabled by setting the
**D3D12_SHADING_RATE_1X1** shading rate via **RSSetShadingRate**, so
that the shading rate image can be computed and rendered at its full
resolution. The shading rate image is then set using
**RSSetShadingRate** and **RSSetShadingRateImage**, after the compute
shader is finished filling the shading rate image with appropriate
values.

**Scene Rendering**

Before the triangle and full-resolution cloud sprites are rendered into
the framebuffer, the alpha values of the cloud sprites are rendered into
a smaller off-screen render target to be processed by a compute shader
for populating the VRS screen-space image.

Additionally, ExpensiveShaderFunction (PixelShader.hlsl) was added to
make the scene triangle's pixel shader expensive enough to demonstrate
the performance benefit from using coarse VRS shading rates.

**VRS Screen-Space Image Determination**

Shading rates are chosen based on the horizontal and vertical alpha
gradients per pixel as well as the presence of completely transparent
pixels to determine if a pixel is a part of a cloud's outline. Pixels
that are part of vertical edges are designated with the
**D3D12_SHADING_RATE_1X2** shading rate while those belonging to
horizontal edges are instead assigned with **D3D12_SHADING_RATE_2X1**.
**D3D12_SHADING_RATE_2X2** is otherwise assigned by default to count for
pixels a part of any horizontal-vertical intersections. For the outlines
of clouds, center pixels are additionally checked against any
transparent pixels that could imply there is a vertical or horizontal
edge that separates the cloud from its transparent background.
Completely transparent pixels are always assigned at full rate
(**D3D12_SHADING_RATE_1X1**). An example of this shading rate
calculation is illustrated in Figure 1.

![](./media/image4.png)
![](./media/image5.png)

***Figure 1:** Comparison between the cloud sprite rendered in the
scene, and a visualization of the shading rates assigned to it. Green
areas are shaded with 2X2, Blue regions are 2X1, 1X2 shading rates are
colored in yellow, and 1x1 is represented by dark red.*

# Update history

08/08/2021 -- Sample creation.

10/15/2021 -- Added 1440p support for Xbox Series S and Xbox One S.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
