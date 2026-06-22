![](./media/image1.png)

# SimpleROV

*This sample is compatible with the Microsoft Game Development Kit (March 2024)*

# Description

This sample demonstrates a simple usage of the Raster Order Views feature on Xbox Series Family and PC. In particular, it shows how to implement an
Order Independent Transparency algorithm using ROVs, and compares it against another OIT algorithm that uses atomics, as well as pure hardware
blending (with sorted geometry based on distance to camera).

![](./media/cover.png)

# Building the sample

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If running on PC, set the active solution platform to `Gaming.Xbox.Desktop.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Controls

| Action                                 |  Gamepad           |  Keyboard       |
|--------------------------------------- |--------------------|-----------------|
| Switch blend mode                      |  A/B               | Enter/Back      |
| Switch translucent model               |  X                 | Space           |
| Exit the sample.                       |  View Button       | Esc             |

# Implementation notes

Blending is a fixed function operation which happens in the render backend and ensures correct order of operations. Meaning, the blending 
will happen in the order of primitive submissions. Although we get this guarantee for writes to the rendertarget, the execution of PS threads
is not bound to this, and can happen in parallel/out of order. Anything written to a unordered access resource will, 
therefore, not have any guarantee regarding ordering.

Raster Order views change this. Writing to an ROV from a pixel shader enforces order of submission. Meaning that if a PS thread about to interact 
with the ROV shares sample coordinates with another (from a wave belonging to a primitive submitted earlier), the first PS thread will be forced 
to wait for the other one to finish operating on the ROV before being allowed to proceed. This on one hand has a negative impact in performance,
given it serializes work that would otherwise happen concurrently. But on the other hand, it allows opens up new possibilities, like implementing 
programmable blending in the pixel shader.

Using ROVs require no code changes on the D3D12 API side of things. They are created as any other unordered access resource.

HLSL does require one change. The hlsl resource type must be prefixed by 'RasterizerOrdered'. In the sample, this looks like this:

```
RWTexture2D<uint>					clearMask  : register(u0);
RWStructuredBuffer<NodeFragments>	nodeList   : register(u1);
```

becomes

```
RasterizerOrderedTexture2D<uint>					clearMask  : register(u0);
RasterizerOrderedStructuredBuffer<NodeFragments>	nodeList   : register(u1);
```

The sample shows 2 Order Independent Transparency techniques, plus fixed-hardware blending. It also allows
to switch between two translucent models (Dragons and Gargoyles), to compare performance with different 
levels of geometry overlap.

Fixed function blending can help when objects are easily sortable via camera distance (like the
dragons in the screenshot below). The transparency will be correct in terms of one draw call to the
next, but there is no way to ensure the primitives will be sorted correctly (no cheap way at least)
thus resulting in a less than ideal result.

![](./media/fixedhwblend.png)

OIT algorithms on the other hand allows to ensure per-pixel correct ordering, like in this screenshot:

![](./media/oitblending.png)

- **Per Pixel Linked List (PPLL)**: This technique consists on creating a linked list with blending
  information per pixel. It follows the principle of the A buffer, but with a limited amount of 
  nodes (this sample shows the results of using 4 nodes). To control the unordered access to UAVs,
  this algorithm makes use of atomics. It offers good results, but comes at a big cost in memory. 
  For example, for a 4K scene with 8 Nodes and each node using 3 unsigned ints, the size of the entire 
  linked-list is 3840 x 2160 x 8 x 3 x 4 = ~796MB. 

- **Multi Layered A B (MLAB)**: This algorithm tries to approximate the Transmittance function for
  each of the pixels, in order to later composite results using it. This implementation uses ROVs
  to ensure ordered access to the resources. Results will depend on the amount of nodes used to 
  approximate the function, but the usage in terms of memory is much lower than PPLL. For a 4K scene 
  with 4 Nodes (it does not need as many since it tries to approximate), each using 2 unsigned ints, 
  this technique uses for its node buffer 3840 x 2160 x 4 x 2 x 4 = ~265MB. 

In depth details of these techniques can be found in this excellent blog series: 
https://interplayoflight.wordpress.com/2022/06/25/order-independent-transparency-part-1/

# Notes

# Update history

02/16/2024 -- Created Sample.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
