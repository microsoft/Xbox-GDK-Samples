  ![](./media/image1.png)

#   DXR Procedural Geometry

*This sample is compatible with the Microsoft Game Development Kit (March 2023) and Windows 10 (Version 1903) May 2019 Update*

# Description

This sample demonstrates simple DirectX Raytracing (DXR) drawing of different kinds of procedurally generated geometry (and triangle based geo with the floor plane). 

![](./media/image01.png)

# Building the sample

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If using PC with appropriate hardware and Windows 10 release, set the
active solution platform to `Gaming.Deskop.x64`.

This sample does not support Xbox One.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

This sample allows to move around with a free camera (which can be controlled with the left and right sticks on the gamepad, and alternatively with the directional keys + mouse). Pressing A button/C key (gamepad/keyboard) toggles the geometry animation. The default for geometry is to be animated. Pressing B button/V key toggles the shadow casting light animation. The default for the light animation is off.

# Controls

| Action                       |  Gamepad         |  Keyboard         |
|------------------------------|------------------|-------------------|
| Toggle animated geometry     |  A               |  C                |
| Toggle animated light        |  B               |  V                |
| Toggle raytracing type       |  X               |  X                |
| Exit                         |  View Button     |  Escape           |

(*) Toggle raytracing type only works on Scarlett.

# Implementation notes

This sample demonstrates DXR usage with both triangle and procedurally generated geometry. For the first, the Bottom level acceleration structure (BLAS) will contain the triangles that conform the geometry, and whenever a ray intersects against the acceleration structure leafs, a hit is registered. After traversal, a miss shader or closest-hit shader will be executed, depending on whether the ray intersected any geometry.

For procedurally generated geometry, the BLAS does not hold the geometry. Instead, it contains the definition for an AABB (D3D12_RAYTRACING_AABB). Whenever a ray intersects agains an AABB, an intersection shader with custom logic that determines whether the ray intersects the geometry or not will be executed. At the end of traversal, a closest-hit or miss shader will also run depending on whether there was a hit registered or not.

To determine which shader to run (depending on what geometry the ray intersects against), Shader tables are used, containing shader table records. These consist of a 32 bytes shader indentifier and a set of local root parameters that will be locally bound. Later, when calling traceRay, information contained in the dispathRays call, the traceRay call and the geometry (from the BLAS) will be used to index the correct entry on the table. For this sample, we define a ray gen shader table (with one entry), a miss shader table (with two entries, one for shading and one for shadow misses), and a hit shader table (with 22 entries, one per primitive type per ray type).

There are two Bottom-Level Acceleration Structures (BLAS), one which contains the triangle geometry (floor plane, consisting of 2 triangles), and one which contains all the Axis aligned Bounding Boxes (AABB)s. The TLAS will hold one instance for each of this BLASes. The TLAS and BLAS are built at the start of the frame on this sample, and are not being updated.

The Miss Shader is invoked for every ray that misses and writes black to the UAV (this is in place of pre-clearing the UAV to black before the DispatchRays call).

The Raygen shader will cast one ray per pixel into the scene using a perspective projection. When a hit is registered, two more rays will be cast. One to test for shadows, which will go in the direction of the shadow casting scene light, and one which will calculate a reflection trayectory based on the incoming ray and the normal of the geometry at the intersection point. The shadow ray will determine whether this point is occluded or not, while the reflection ray will check against the scene's geometry for reflections contributions. Reflections and shadows have a limit of 3 bounces (recursion limit) before stopping.

The Closest-Hit shader will be in charge of shading based on the material information for the primitive, whether the point is in shadow, ambient light info, and whether any reflection contribution exists from other geometry.

For Xbox Series consoles only, *inline raytracing* is available. 

### Inline Raytracing

*For a more in depth dive into inline raytracing on Xbox Consoles, see "XDXR Standalone Traverse" in the GDK.chm or online GDK documentation.*

Inline Raytracing (in this sample) is only available on Scarlett. XDXR does not currently support DXR 1.1 RayQuery object. To cover for this, a single header traversal implementation is provided in the GDK (with several variants) which shares most of DXR 1.0 traversal logic, and wraps it in an API that is largely equivalent to the RayQuery object API. This header can be used in any shader-stage which allows user allocation of LDS, including Compute and Pixel shaders. In this particular sample, the traversal happens in a compute shader (see *InlineRaytracing.hlsl*).

This sample includes the XDXRStandaloneTraverseHT, which according to the docs *This version of traversal implements a “coherent” traversal algorithm which may be faster than other algorithms for highly-coherent content like shadow rays or mirror reflections. This is equivalent to specifying XBOX_RAY_FLAGS_COHERENT_RAYS_HINT in your DXR 1.0 TraceRay() ray-flags*. Press X at any point to switch between raytracing modes.

This sample uses the InlineAHS/InlineIS lambda system, which allows intersecion shaders (IS) or anyhit shaders (AHS) to be inlined in the ray traversal. This improves traversal speed, since the program does not need to break out of the traversal loop to give control back to the user everytime it would need to run one of these shaders. In this particular sample, we do not use AHS, only IS. (see *InlineRaytracing.hlsl*).

# Update history

04/10/2023 -- Sample GDK porting.

# Notes

- There is a bug being investigated which causes visual artifacts when compiling with DXR on Scarlett with /Od. For this reason, the debug configuration is not using /Od at the time. This should be corrected once the bug is fixed.
- There is a potential bug with the intrinsic when used in nested traceRay calls. Commenting out this usage in the meantime.
- There is a driver bug which causes the sample to crash when failing validation due to not having allocated Compute Scratch (this should not be necessary since Compute and DXR scratch are not the pool). This was fixed for June 2023, but it will cause the sample to throw on validated drivers for 2303 (March). This means the debug config for March will throw an exception. Sample runs ok on Profile and Release. This should be updated once the driver bug is fixed.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
