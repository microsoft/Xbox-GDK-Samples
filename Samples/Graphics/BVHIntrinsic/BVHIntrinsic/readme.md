  ![](./media/image1.png)

#   BVH Intrinsic Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This sample demonstrates how to access and use the Xbox Series X|S
"image_bvh_intersect_ray" instruction in order to accelerate Raytracing.
The Sample is split into two distinct scenes. The first demonstrates how
to traverse a BVH of triangles while the second demonstrates how to
trace rays against a BVH of voxels.

The sample also includes a software implementation of the hardware
intrinsic to allow the sample to run on Xbox One devices at a
much-reduced level of performance. The emulated intrinsic is not
guaranteed to be an exact match for the Xbox Series X|S intrinsic, but
merely to provide a close approximation.

![](./media/image3.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample features a flying / first-person camera to allow the camera
to be placed anywhere within the scenes. Full explanation of the
controls are listed below:

# Controls

| Action                                    |  Gamepad                  |
|-------------------------------------------|--------------------------|
| Toggle between Emulated (Software) / Native (Hardware) BVH intrinsic. |  A |
| Change visualisation colour               |  B                        |
| Change Scene (Voxels, Triangles)          |  X                        |
| Toggle HUD                                |  Start Button             |
| Switch resolution (720, 1080p, 4K)        |  DPad -- Up / Down        |
| Change Dragon LOD                         |  DPad -- Left / Right     |
| Switch visualization mode                 |  Left / Right Bumper      |
| Camera Forwards / Backwards / Strafe      |  Left stick               |
| Camera Yaw / Pitch                        |  Right stick              |
| Camera Up / Down                          |  Left / Right Trigger     |
| Fast flying mode (Turbo)                  |  Click + Hold Left stick  |
| Quit                                      |  View Button              |

# Implementation notes

## Triangles / Dragon Scene

The BVH for 7 LODs show were built using Intel Embree's BVH builder C++
API. Once built, a pass to convert the BVH from a hardware agnostic
format to one that matches Xbox Series X|S specific BVH node format for
FP16 AABBs and Triangles was performed. Appended to the end of the
buffer is a single DWORD indicating how many triangles were in the
model. The first 64 bytes in the file represent the 'root node' of the
BVH.

The shader "RaytraceTriangles.hlsl" is responsible for traversing the
BVH from root node to eventual triangle-hit or miss. This Compute Shader
maintains a small stack of 28 DWORDs per thread in order to store
potential future nodes in the tree to follow at a later time. The number
'28' was derived experimentally by viewing the content used in the demo.
By keeping the stack size as small as possible we maximise the number of
waves running on a CU/WGP as LDS-usage is the limiting factor for higher
occupancy.

The core traversal loop is deceptively simple and performs the following
steps:

1)  Intersect the ray against the next node in the traversal loop
    (initially the root node)

2)  If this node was a Triangle Node:

    a.  Calculate the 't' distance to the hit (if missed, 't' is
        Infinity).

    b.  Determine if this hit was nearer than the previous nearest hit.

        i.  If so, update the hit distance, the nearest node pointer and
            the triangle ID

        ii. If not, do nothing

    c.  Set the next node pointer to InvalidNodePtr so we pop one off
        the stack at Step 4.

3)  If this node was an FP16 AABB node:

    a.  Set the closest AABB hit (childPtrs.x) to be the next node we
        follow whether valid or not.

    b.  For the other 3 AABBs, push them onto the stack in reverse
        distance order if valid.

4)  If we don't have another node to follow on the next iteration of the
    loop...

    a.  Pop one off the stack if one is available

5)  If we still don't have another node to follow next, traversal
    terminates now.

The RaytraceResults structured returned contains information about the
hit:

-   'hit' -- whether a triangle was hit.

-   'leafIndex' -- an index uniquely identifying which Triangle **Node**
    the hit came from

-   'nearestT' -- the hit distance to the nearest hit, or Infinity if no
    hit.

-   'numIterations' -- the number of steps up and down the BVH the ray
    took to complete

-   'triIndexInLeaf' -- 0-3 depending on which triangle index in the
    leaf we hit

Because this BVH contains overlapping AABBs it is not enough to stop the
traversal at the first hit and assume it is the nearest possible hit for
this ray. For this reason, when a new 'nearest hit' is found, we update
'nearestT' to that distance. 'nearestT' is passed in to the BVH
Intrinsic as the new maximum distance we're interested in finding hits
at. Since we have a confirmed hit, we no longer care about hits that
occur at distances greater than that distance.

The LOD0 model contains almost 5 million polygons and represents a model
tessellated far beyond that of normal game content. LOD1-6, starting at
200,000 polygons and decreasing by \~half with each LOD represent a much
more reasonable level of content for modern video games. The size of the
BVHs vary between 266MB and 0.3MB.

The four visualization modes are:

-   Normal -- This mode simply visualizes each triangle by assigning it
    a unique index and then producing a greyscale colour.

-   Depth -- A reverse-Z linear depth value.

-   Traversal Cost -- A 'heatmap' of ray complexity. The more steps it
    took a ray to hit (or miss) its target the brighter the pixel. Rays
    that glance close to the surface take more iterations than those
    travelling through free space or impacting the model head-on.

-   Primitive -- A simple colorization according to 'leafIndex'. Since
    triangles can be stored together in groups of 1-4 triangles (in a
    Triangle Node) it can be interesting to see how these triangles were
    stored.

[Voxels / Rungholt by kescha](https://www.planetminecraft.com/project/rungholt-3505409/)

The Voxels scene is made up of 16.7 million voxels from the Rungholt
dataset available online. As with the Dragon datasets, the model was
built using Intel's Embree BVH builder and converted as a
post-processing step to the native Xbox Series X|S format. Only one LOD
is available.

This BVH differs from the other scene by the fact that Voxels are
treated as the 'leaf' type in the tree -- meaning no triangles are
stored in the BVH. The upper levels of the BVH are stored in the same
way as in the triangle scene, but the 'child pointers' in the leaf nodes
have a reserved bit to indicate that they are in fact a leaf and no
further traversal is required (bit 30, 0x40000000). The lower 30 bits
encode the position of the AABB that we've just hit (stored in an
`11_8_11` -- XYZ) format -- allowing a dataset up to 2048 x 256 x 2048 in
size.

The advantage of storing this information in the child node pointer
itself is that no ancillary data structure need be stored alongside the
BVH. This information is returned directly to us by the BVH intrinsic
without additional load operations.

In contrast to the Triangles BVHs, this BVH contains no overlapping
AABBs in a single AABB node. With this guarantee we can terminate
traversal the first time we get a hit and not worry about there being
potentially closer hits in our LDS stack. The traversal routine
(RaytraceVoxels.hlsl) is simpler than that of the Triangles routine for
that reason.

However, once the first (and closest) hit has been found, the BVH
Intrinsic gives us no information about the hit distance or any
coordinates on the AABB itself where the hit occurred. The only
information we have is which block was hit. For this reason, there is an
additional step performed in ALU (AABBIntersect) that helps determine
which side of the AABB we hit and at what distance. Those two pieces of
information are enough to light and shade the voxel. Should you wish to
texture the voxels it is trivial to turn the 'ray end' position into a
texture coordinate.

The sample then goes on to launch up to three more rays per pixels.
These rays are:

1)  A simple directional shadow ray for direct illumination.

2)  A reflection ray off the water and glass surfaces.

3)  A shadow reflection ray, to determine direct illumination on the
    reflected surface.

No function calls or recursion are needed, it is simply an iterative
process and could be extended to Ambient Occlusion, Refraction or Global
Illumination in the future without further passes if desired.

A separate ancillary structure containing a per-block 'ID' (one byte)
indicates what block type we hit. Since there are no spare bits
available in the FP16 AABB node when using the child pointer store
"block position" this has to be fetched from a buffer based on an index
derived from our position in the BVH.

# Update history

11/14/2019 -- Sample creation.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
