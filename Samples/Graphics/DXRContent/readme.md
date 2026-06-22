 #   DXRContent Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This sample allows the user to experiment with different build flags for acceleration structures and see their 
effect on raytracing performance and AS size. It also demonstrates good and bad ways to set up content for raytracing.

# Building the sample

This sample is only supported on Xbox Series X|S. To build the sample, open the solution file `DXRContent.sln` in Visual Studio 2019 or later.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The left and right thumbsticks can be used to move the camera around the
scene. The D-Pad up/down switches the selected menu option. The left
and right D-Pad buttons change the values for the menu options. The A button
switches the active options menu. The X button rebuilds all BLASes. The Y button 
rebuilds the TLAS. The B button toggles between default values and user set values 
when in the BLAS or TLAS menus and toggles between good and bad content when in the 
content menu.

![](./media/image1.png)

# Debug Modes

Iterations: Visualizes the number of iterations for the primary rays using a heatmap.

![](./media/heatmap.png)

Shadow Iterations: Visualizes the number of iterations for the shadow rays using a heatmap.

![](./media/shadow_iterations.png)

Time: Visualizes the time per wave for tracing the primary ray using `__XB_s_memrealtime`.

![](./media/time.png)

Geometry Index: Renders each geometry in the BLAS a different color.

![](./media/geo_index.png)

Instances: Renders each instance of a BLAS using a different color.

![](./media/instances.png)

# Content

This sample has two versions of each piece of content (except the island) to demonstrate how different ways of setting up the same content can affect raytracing performance. 
In the Content menu, each piece of content can be set to "Good", "Bad", or "Off". In most cases the "Good" version results in faster trace times than the "Bad" version, but this sometimes depends on the viewing location.

Ship: The good version of the ship has multiple geometries in one BLAS, while the bad version has each geometry with a different material using a different BLAS. 
Having a different BLAS for each part of the object with a different material causes many overlapping bounding boxes in the TLAS hierarchy which is inefficient for traversal.

![](./media/ship_multi_blas.png)
![](./media/ship_single_blas.png)

Tree: The good version of the tree has the trunk, branches, and leaves all in one BLAS, while the bad version has each part in a different BLAS. Again this leads to a situation with multiple overlapping bounding boxes.

![](./media/tree_good.png)
![](./media/tree_bad.png)

Pier: The good version of the pier has one BLAS for the whole pier, while the bad version has the planks in one BLAS and the posts in the other BLAS. 
Since the planks and the posts are located in the same space, this causes the AABBs in the TLAS to overlap each other which is less efficient than having them in one BLAS.

![](./media/pier_good.png)
![](./media/pier_bad.png)

Ocean:
The two versions of the ocean are a single BLAS (with multiple geometries) and 4 separate BLASes. In this case there isn't a clear good and bad way to set up this content since the performance varies depending on the order in which rays encounter scene geometry. 
This is sensitive to the bounds of objects intersecting the ocean. 
In some cases it's beneficial to split the ocean into multiple chunks that can have tighter bounds, while in other cases it's beneficial to leave the ocean as a large model and increase the probability of rays intersecting it first.
In this sample when zoomed in close on one of the islands, the single BLAS version performs better. 
When zoomed further out with more of the ocean visible, the version with 4 separate BLASes performs better.
The optimal way to set up terrain meshes in a game will depend on the content included. One thing to note is that large objects overlapping the camera must always be tested by the ray traversal so it is often beneficial to split terrain into smaller pieces.

![](./media/ocean_single_blas.png)
![](./media/ocean_multi_blas.png)

Fence: The good version of the fence has the three segments as separate BLASes. The bad version has all segments of the fence as one BLAS. 
Since each of the three segments of the fence is straight and thin, having a separate BLAS allows it to have a very tightly fitting AABB.
Having all of the segments in one BLAS leads to a large empty space in the middle of the AABB which causes worse traversal performance.

![](./media/fence_good.png)
![](./media/fence_bad.png)

Shovel: The good version of the shovel has the geometry straight in object space and then the instance is rotated when it's placed in the TLAS. 
The bad version bakes the rotation into the geometry which means the AABB for the object has a lot of empty space.

![](./media/shovel_good.png)
![](./media/shovel_bad.png)

Skybox: The good version of the skybox is not having a skybox at all and instead looking up in the cube map if the ray misses all geometry. The bad version has a sphere with the skybox texture. 
When looking at the TLAS hierarchy in the PIX BVH viewer, the version with geometry for the skybox has a large bounding box for the partition around the skybox sphere which means all rays have to test intersection with it. 
The version without the skybox much more closely fits the rest of the geometry without lots of empty space.

![](./media/skybox_tlas.png)
![](./media/no_skybox_tlas.png)

# Implementation

At start-up, all of the models are loaded. During the first frame, the BLASes for all models are constructed. The TLAS is constructed using the currently selected models. 
When individual BLAS or TLAS settings are changed, the rebuilds don't happen until the X or Y button is pressed. Pressing the B button to toggle between using default 
settings or custom values triggers a rebuild. In the Content menu, switching between good and bad versions or turning a model off triggers a TLAS rebuild.

The offline builds for BLASes in this sample happen at runtime on the CPU using the offline builder. This is enabled by setting the driver hint with the `SetDriverHintX` API.
This is for testing purposes only and shouldn't be used in shipping titles.

The ray tracing in this sample uses inline raytracing to trace both primary and shadow rays. The visualization modes are built into the ray tracing shader.


# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
