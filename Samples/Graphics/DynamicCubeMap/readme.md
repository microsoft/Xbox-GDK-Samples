  ![](./media/image1.png)

#   DynamicCubeMap Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This sample demonstrates how to render to a cubemap at runtime using various render methods including geometry shaders, instancing, and mesh shaders.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The left and right thumbsticks can be used to move the camera around the
scene. The D-Pad can move the reflective sphere up and down. The left
and right D-Pad buttons cycle through the render modes. The Y button
turns async compute on and off.

![](./media/image2.jpeg)

# Controls

| Action                                |  Gamepad                      |
|---------------------------------------|------------------------------|
| Turn async compute on/off             |  Y button                     |
| Move the sphere up/down               |  D-Pad up/down                |
| Change rendering mode                 |  D-Pad left/right             |
| Move camera                           |  Left Thumbstick              |
| Rotate camera                         |  Right Thumbstick             |

# Implementation notes

The sample demonstrates 4 ways of rendering the scene geometry into a
cubemap dynamically at runtime.

1.  Loop: The draw calls for the scene geometry are submitted in a loop
    with each iteration rendering to a different face of the cubemap
    using the appropriate transformation matrix.

2.  Geometry Shader: The geometry is submitted once and amplified to
    each cubemap face in a geometry shader. The geometry shader loops
    over each cubemap face and uses SV_RenderTargetArrayIndex to send
    the transformed geometry to the appropriate cubemap face.

3.  Instancing: The geometry is submitted once using an instance count
    of 6. In the vertex shader SV_InstanceID is used to choose the
    appropriate transformation matrix and assign the geometry to the
    corresponding cubemap face using SV_RenderTargetArrayIndex.

4.  Mesh Shader (Scarlett only): The geometry is submitted once and uses
    an amplification shader to replicate the geometry for each cubemap
    face. The amplification shader also performs culling at the meshlet
    level so only meshlets that would be visible in a particular cubemap
    face are sent to the mesh shader. The mesh shader transforms the
    geometry and sends it to the appropriate cubemap face using
    SV_RenderTargetArrayIndex.

After rendering the geometry to the cubemap the sample creates a mipmap
chain for the cubemap using a compute shader. When using the async
compute mode, the mip generation will happen on the async compute queue
in parallel with rendering the parts of the scene that do not use the
cubemap.

After generating the cubemap mips the sample renders a reflective sphere
that samples from the cubemap texture.

# Known issues

There is a Scarlett driver bug with amplification shaders that causes
some geometry to disappear.

There is a Scarlett driver bug with geometry shaders that causes several
small missing triangles.

These driver bugs are fixed in the March 2023 GDK so the mesh shader and
geometry shader implementations are disabled in prior GDK versions.

# Update history

The original version of the sample was written using the XSF-based
framework. It was rewritten to use the ATG sample templates in January
2023 and also add support for mesh shaders as a render method.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
