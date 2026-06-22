  ![](./media/image1.png)

#   Build Your Own Bundles Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

On Xbox One you can directly write data into a buffer, which can be read
and executed directly by the GPU using the ExecuteIndirectBundleX API.
This sample covers various techniques of writing out this data and
compares it with regular draws and draw bundles. The buffer data can be
written by the CPU or GPU and can be fed back into the GPU in subsequent
passes. This feature is referred to Build Your Own Bundles (BYOB). All
the "high frequency" API calls can be written into a buffer directly.
The code is the same as available from d3d12_x.h but it writes to a
supplied buffer instead of writing it directly to the command buffer.

Few things to keep in mind while writing data into a buffer to be
executed on the GPU:

-   The buffer should be created with the PAGE_GPU_EXECUTE flag

-   The buffer size to be executed should not exceed 4MB

-   It is easy to hang the GPU if you pass in wrong packet information

-   There are NO_OP packets available to supply some custom data which
    are ignored by the GPU

-   If using a write-combined buffer to write from the GPU, do watch out
    for all write-combining rules.

The sample shows multiple methods of drawing instances of a mesh.
Multiple PSOs are used which are randomly assigned to the instances. The
GPU times are the same for each of the methods. For 16384 meshes, it
80.3ms without culling and 39.2 ms with culling (7439 meshes drawn). The
CPU times vary and are listed along with the method description. Drawing
methods:

-   **BYOB - Update each instance and draw**

This technique updates the BYOB bundle data for each instance and then
calls ExecuteIndirectBundleX for that instance. The buffer using this
technique is small in size as it only has data for a single instance.
It sends the draw data for each instance separately to the GPU one
after the other. CPU: No Cull = 7.01 ms, Cull = 6.65 ms

-   **BYOB - Update all instance and then draw**

Update and write the BYOB bundle data for all instances into a single
buffer and finally pass the buffer to the GPU. The buffer used for
this technique will have a bigger size than the previous technique,
but all the data is sent to the GPU in one single
ExecuteIndirectBundleX. CPU: No Cull = 33.5 ms, Cull = 19 ms

-   **BYOB - Build bundle at runtime**

Build the entire buffer at runtime and then send the buffer in a
single ExecuteIndirectBundleX command. CPU: No Cull = 7.4 ms, Cull =
6.8 ms

-   **BYOB - Draw using GPU**

Write the GPU packet data to draw all mesh instances into a buffer
using a Compute Shader and then pass that buffer to the GPU using
ExecuteIndirectBundleX. Culling is performed on the GPU. CPU: No Cull
= 6.0 ms, Cull = 6.0 ms

-   **Draw using bundles**

This technique uses the normal bundles to draw the data using the
ExecuteBundle API. CPU: No Cull = 6.9 ms, Cull = 6.5 ms

-   **Direct draw**

Directly draw the meshes using DrawIndexedInstanced. CPU: No Cull =
7.7 ms, Cull = 6.9ms

All the buffer writes on the CPU are performed using a single thread.
There will be performance benefit when writing to the executable buffer
using multiple threads. If the size of each state setting and draw is
kept constant, multiple threads can easily write out into the buffer on
the CPU. NO_OP packets can be used to fill up unused space in the
buffer. Writing the packet data through the GPU reduces CPU time and
writing out the packet data isn't expensive. This method is the fastest
among those tested in the sample.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

## Screenshot

![](./media/image3.png)

| Action                                 |  Gamepad                     |
|----------------------------------------|-----------------------------|
| Change drawing technique               |  A or B buttons              |
| Cull meshes                            |  X button                    |
| Show/hide more description about the selected technique |  Y button |
| Exit                                   |  View Button                 |

# Implementation notes

All the code used in the sample for writing out the bundle is available
in WriteOwnBundlesHelper.h. Writing packet data into the buffer is based
on code from the header file d3d12_x.h.

For example, DrawIndexedInstanced is defined as:

```cpp
D3DINLINE void D3DAPI DrawIndexedInstanced(
    _In_ UINT IndexCountPerInstance,
    _In_ UINT InstanceCount,
    _In_ UINT StartIndexLocation,
    _In_ INT BaseVertexLocation,
    _In_ UINT StartInstanceLocation)
{
    D3D12XBOX_PPUT pPut = m_Putter.m_pCurrent;
    if (pPut < m_Putter.m_pLimit_Draw)
    {
        m_Putter.PutD(pPut, D3D12XBOX_PACKET_DRAW_INDEXED_INSTANCED);
        m_Putter.PutD(pPut, InstanceCount);
        m_Putter.PutD(pPut, StartIndexLocation);
        m_Putter.PutD(pPut, IndexCountPerInstance);
        m_Putter.PutD(pPut, BaseVertexLocation);
        m_Putter.PutD(pPut, StartInstanceLocation);
        m_Putter.m_pCurrent = pPut;
    }
    else
    {
        CommandListFunction()-\>DrawIndexedInstanced(this,
        IndexCountPerInstance,
        InstanceCount,
        StartIndexLocation,
        BaseVertexLocation,
        StartInstanceLocation);
    }
}
```

While writing out the bundle, just change it to:

```cpp
void DrawIndexedInstancedBYOB(
    _Inout_ UINT32** writeAddress,
    _In_ UINT IndexCountPerInstance,
    _In_ UINT InstanceCount,
    _In_ UINT StartIndexLocation,
    _In_ INT BaseVertexLocation,
    _In_ UINT StartInstanceLocation)
{
    **writeAddress = D3D12XBOX_PACKET_DRAW_INDEXED_INSTANCED;
    *(*writeAddress + 1) = InstanceCount;
    *(*writeAddress + 2) = StartIndexLocation;
    *(*writeAddress + 3) = IndexCountPerInstance;
    *(*writeAddress + 4) = BaseVertexLocation;
    *(*writeAddress + 5) = StartInstanceLocation;
    *writeAddress += 6;
}
```

# Known issues

PredicationBuffer does not currently work when using
ExecuteIndirectBundleX. Pass 0 for PredicationBufferOffset.

# Update history

-   April 2019: Initial Release

-   November 2019: Updated to support project Xbox Series X|S
