![](./media/image1.png)

# Advanced Lighting Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This sample demonstrates various techniques to improve rendering
performance when dealing with big number of lights on scene.

![](./media/image2.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If running on PC, set the active solution platform to `Gaming.Xbox.Desktop.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Controls

| Action                                       |  Gamepad               |
|----------------------------------------------|-----------------------|
| Switch between techniques                    |  A / B                 |
| Modify Light's Radius                        |  Right/Left DPad       |
| Toggle Lights                                |  LB                    |
| Toggle Rendering Light Source (colored particles) |  RB |
| Mode the Camera around  |  LT-RT for moving up, down Right Stick to rotate view Left Stick to move translate             |
| Exit the sample.                             |  View Button           |

# Implementation notes

The sample demonstrates the following rendering techniques:

-   Deferred Rendering

-   Light Volumes

-   Tiled Deferred

-   Clustered Deferred

This sample shows how these techniques compare against one another by
showing a timer of how long they take in the GPU, as well as how much
time the whole frame took.

For Light volumes, the algorithm consists on rendering sphere meshes
using the light's world position and radius to position and scale them
on the scene. This way, we only do work for a particular light on the
pixels rasterized by the sphere. Of course, we still need to see if said
pixel is or isn't affected by the light (using the position information,
the light's attenuation, and the scene information provided by the
geometry buffer).

Light's Volume disadvantage comes when multiple spheres overlap when
rasterized, since we end up calling multiple draw calls for the same
pixel.

Tiled Deferred is a solution to this problem. Instead of rendering
spheres, we divide the screen into tiles, and then bin the lights into
these, so each tile holds a list of all the lights that affects it.
Then, we can dispatch a Compute Shader that will assign a threadgroup
per tile and do all the lighting calculations, skipping the tiles that
have no lights.

Tiled Deferred improves over light volumes, but it still has some
issues. One is that since we are looking at which lights affect a tile,
we might be wasting time on lights that are not affecting the scene
elements in that tile (for instance, they might be further than the
farthest element on the tile). This can be solved by defining depth
bounds, so we only consider lights that are between the scene's min and
max depth. But then another issue is depth discontinuities: If the same
tile has two objects at different depths, every light in between will be
included even if it does not affect elements on the scene.

To solve this, the third technique presented is Clustered Deferred. It
is basically the same principle as Tiled, but instead of dividing the
screen into tiles (2D), it divides the fustrum into cells (3D clusters).
Doing this and binning the lights on these clusters allows to save
processing on the lights that do not affect anything on the scene but
are in between its depth bounds.

# Notes

-   Currently Light Volumes are not being Fustrum Culled.

-   Light Sources are not affected by other light sources as the sample
    is right now.

-   There are artifacts which show up if the number of lights on one
    tile surpass the maximum allowed (512). This will happen if the
    radius is inflated enough.

# Update history

9/26/2022 -- Ported Sample.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
