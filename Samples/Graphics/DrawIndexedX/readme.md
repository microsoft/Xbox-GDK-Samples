  ![](./media/image1.png)

#   DrawIndexedX Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# 

# Description

This sample shows how to use the Xbox only feature -- DrawIndexedX. This
feature combines threads for multiple consecutive draw calls into a
single wave and executes them as a single draw and can be used instead
of DrawIndexedInstanced for a single instance. In case of multiple
instances, DrawIndexedX can be separately called for each instance and
the instance data can be passed into a buffer with some bits of index
buffer identifying the correct instance. The packing happens only if
there are no state changes between draw calls. In case of state changes,
the draw call behaves same as DrawIndexedInstanced. Only the index
buffer and index buffer format format can be changed for each
DrawIndexedX call. An offset can be provided which offsets into the
vertex buffer. The vertex buffer needs to be combined into a single
buffer containing all vertex data. Few bits of the index buffer can be
used to pass in data to help identify draw calls (draw ID) in the
shader. More data specific to a draw call can be passed into a
structured buffer and extracted using the draw ID. The simplest place to
use this and see some gains is during the depth pass as no state changes
are usually required and so the threads can be combined. During the
color pass, it may result in a gain, but depends on how much divergence
is caused by combining the threads.

**On Xbox One X and Xbox Series X|S, this feature behaves the same as
DrawIndexedInstanced due to some hardware settings.**

```cpp
void DrawIndexedX(
    D3D12_GPU_VIRTUAL_ADDRESS IndexBufferLocation,
    DXGI_FORMAT IndexFormat,
    UINT IndexCount,
    UINT IndexOffset
);
```

Changes required in existing shader code to support DrawIndexedX:

-   Vertex ID for the draw now needs to be extracted from SV_VERTEXID as
    the index buffer data might have been padded to include other
    information like drawID, materialID etc. More data can be passed
    into shaders using a structured buffer which can be indexed based on
    the drawID.

-   Descriptors which vary per-draw need to be moved into arrays as
    shown in the pixel shader for SRVs.

-   Might require to use the NonUniformResourceIndex keyword when using
    values which might be different for threads within a wave. Without
    the keyword, the shader assumes that the value is uniform for all
    threads in the wave.

Observations for the simple scene in the sample:

-   The depth pass shows a slight decrease in timing when using
    DrawIndexedX.

-   The number of VS waves generated for the scene drops (from 2438 to
    2183).

-   Only about 1-2% of the waves diverge in the pixel shader when using
    a texture array for the SRVs.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

This sample is supported for Xbox Series X|S beginning with the October
2021 Recovery. Set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

## Screenshot

![Sample Screenshot](./media/image3.png)

Debug Waves:

![DrawIndexedX_Debug_DIX](./media/image4.png)

| Action                                 |  Gamepad                     |
|----------------------------------------|-----------------------------|
| Switch between DrawIndexedX and DrawIndexedInstanced |  A |
| Debug screen -- To view number of divergent waves |  Y |
| Display model using Atlas texture      |  X                           |
| Display depth buffer                   |  Left Shoulder               |

# 

# Implementation notes

The sample executes the following passes:

-   Depth only pass

This pass benefits the most from DrawIndexedX as the vertex shader
doesn't need to branch and there are no pixel shader waves. So all the
calls can be packed together to execute as a single draw.

-   Debug only -- Total Waves count

Pass to count total waves passed to the vertex and pixel shader for the
frame.

-   Mesh Render (Color) pass with Depth EQUALS

    -   This pass draws the mesh to the final buffer and applies the
        texture to it. Different materials are used by the draw calls.
        This material data is passed along with the index buffer
        information. The extra unused bits from a 32 bit index buffer
        can be used to identify which draw call is being executed by the
        shader. Based on the drawID, other information like the material
        ID can be extracted, which can be passed into the pixel shader
        in a constant buffer.

```cpp
uint actualVertexID = vertexID & 0xFFFFF;
uint materialIndex = (vertexID \>\> 20) & MAX_MATERIALS;
```

-   Passing different materials for each of the models can cause
    divergence in the waves generated for DrawIndexedX. Divergence can
    be reduced by using an atlas of multiple textures. The sample uses
    an atlas of all the textures and shows how divergence is completely
    eliminated in that case.

To check the reduction in the number of waves, check the following
counters in PIX:

-   SPI_PERF_VS_WAVE

Indicates number of VS waves generated for the draw call. Check the
number for the DrawIndexedXPerformanceGroup. This should be less than
the waves generated when using DrawIndexedInstanced. This can also be
calculated in the shader as shown in the sample.

-   SPI_PERF_PS_CTL_WAVE

Indicates number of PS waves generated for the draw call.

-   VGT_PERF_VGT_PA_CLIPP_EOP / 2

Indicates number of draw calls executed. If a state change occurs, the
draws get separated into separate groups and this number indicates the
number of separate batches (draws) created.

-   You can also check PIX to see how the contexts get used for the draw
    calls.

When using DrawIndexedX:

![](./media/image5.png)

When using DrawIndexedInstanced:

![](./media/image6.png)

# Known issues

This feature just behaves the same as DrawIndexedInstanced on XboxOneX
due to some hardware settings.

# Update history

**September 2016:** Initial release

**August 2021:** Xbox Series X|S support to match Xbox One X behavior
enabled by new recovery
