  ![](./media/image1.png)

#   PixelBinning Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

This sample demonstrates how to sort pixels by an id associated with
each pixel to later dispatch compute workloads with waves processing
pixels either with the same id or with a minimal set of non-identical
ids.

The sample's rendering loop starts from generating an id/key for every
pixel, which takes a random value in the range \[0 \... the number of
bins). Then binning (or partial sorting) pass re-arranges pixels and
produces a buffer of 32-bit elements storing packed pixel coordinates.
See

![](./media/image3.png)

Figure 1. An example of binning across the entire image. All coordinates
of pixels with the same id are stored adjacently in memory. This
indirection buffer of pixel coordinates can be used to dispatch
different compute kernels for each group of pixels with the same id.

The binning pass can operate in two modes. The first one is the *global*
mode which collects pixels with the same id from the entire the image
(). The second mode is *local*; it collects pixels only within a macro
tile of a given size rather than the entire image () Depending on the
application, it could be that one mode is more suitable than the other.

For example, when the performance of a workload can be improved by
processing pixels with the same id together, and same ids indicates
processing non-divergent data, the local mode can be beneficial as it
allows to reduce data divergency within every individual wave which
processes pixels from the indirection buffer. Such a workload can be a
raytracing workload where waves processing more spatially coherent rays
take less time than waves processing arbitrary rays (Deligiannis &
Schmid, 2019).

The global binning can be beneficial in case when specialized compute
kernels can process pixels with the same id from the indirection buffer
much faster than generic compute kernel handling all types of pixels.
Becoming more popular tile-based (Garawany, 2016) (Turanszki, 2020)
classification approaches are great candidates for the global binning.
If there\'s is a need to classify tiles instead of pixels, an input
texture can be just a lower resolution texture storing per pixel an id
of tile.

![](./media/image4.png)

Figure 2. An example of binning within macro tiles. Pixels with the same
id and from the same macro tile of size 4x2 are stored adjacently. This
mode doesn't allow to change shader per pixel id, but chances that
wavefronts process pixels with the same ids are higher.

# Building the sample

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample provides many options to demonstrate performance
characteristics of the sorting depending on resolution, the bit width of
ids/keys used to sort pixel, sorting locality and memory budget
dedicated for temporal use during sorting.

![](./media/image5.png)

Figure 3. A color map where similarly colored pixels have the same
id/key used for sorting. In this case, only eight ids are used \[0:7\].

The sample has several visualization modes to give an idea of what data
is generated and how pixel binning works. A color map () assigns a
unique color to every unique pixel id to highlight regions of pixels
with the same id. By pressing A button it's possible to switch between
visualization modes.

One of them shows the indirection buffer containing pixel coordinates
after the binning pass. Depending on the binning mode (*global* or
*local*), the content of the buffer is visualized differently. For
global binning mode, each pixel shows an id of the buffer's element
stored at the index computed by multiplying pixel's vertical coordinate
by horizontal resolution in pixels and adding pixel's horizontal
coordinate (a row-major order). For the local binning mode,
visualization is a bit more complicated, but the idea is the same: every
macro tile shows a portion of the buffer of size equal to the number of
pixels in the macro tile.

![](./media/image6.png)

![](./media/image7.png)

Figure 4. A visualization of the indirection buffer. On the left image
-- the resulting buffer after global binning, on the right -- the
resulting buffer after local binning.

Another visualization mode is a divergency heat map (). It colors each
*micro tile* depending on how many unique ids they contain and
highlights the shape of *macro tiles* ().

![](./media/image10.png)

Figure 5. An example of divergency heat map showing how many unique ids
are used in every micro tile of size 64x32 relative to either total
number of pixels in a micro tile or the maximal number of ids depending
on which value is smaller -- in this case, it's min(2048, 8). For
example, if pixels in a micro tile have ids 1, 5, 6 then the heat map
contains a color key corresponding to the rate of 3/8=0.375 for this
tile.

Macro tiles have the size of multiple micro tiles. The internal
distinction between two types of tiles exists because the sample
supports controllable memory budget for temporal use by the binning
pass. This budget is specified as the number of 4-byte elements. Every
macro tile needs a fixed number of 4-byte elements which means that the
total number of macro tiles varies depending on the memory budget.

The size of the macro tile also controls the ordering of pixel
coordinates in the indirection buffer. The bigger the size of the macro
tiles is -- the weaker ordering is. For example, if the size of the
macro tile is greater or equal to the size of the entire image, the
position of a group of pixels with the same id which comes from the same
micro tile is indeterministic within the final sequence. On the other
hand, if the macro tile has the same size as the micro tile, then groups
of pixels have deterministic positions within the final sequence.
Coordinates of pixels from the same micro tile are always stored
adjacently in memory, but their order within the micro tile is also
indeterministic.

![](./media/image11.png)

Figure 6. A divergency heat map showing that the shape of macro tiles
can be a multiple of several micro tiles (in this case 4x2). The shapes
of macro tiles is demonstrated as regions with slightly darker color
tones arranged according to the checkerboard pattern.

| Action                                 |  Gamepad                     |
|----------------------------------------|-----------------------------|
| Increase number of 4-byte counters     |  RB                          |
| Decrease number of 4-byte counters     |  LB                          |
| Increase number of bins                |  RT                          |
| Decrease number of bins                |  LT                          |
| Increase resolution                    |  D-pad right                 |
| Decrease resolution                    |  D-pad left                  |
| Toggle between local and global binning |  Y |
| Cycle between key color map, divergency map and sorted pixel buffer visualization |  A |
| Change text color                      |  B                           |
| Exit                                   |  View Button                 |

# Implementation notes

All code which does pixel binning, related visualization and correctness
tests exists in two files: *PixelBinner.h/.cpp* Those files form a small
drop-in library. *PixelBinner.h* contains extensive documentation of how
to use the provided pixelBinner\* API.

The sample contains several steps. The first step generates 2d simplex
noise (Quilez, 2013) which is used to output pixel ids. The second step
is the binning pass itself (or partial sorting) which works by
dispatching several compute kernels.

The first kernel consists of the threadgroup of size 512/1024 threads
which process a single *micro tile* of size 64x32 or 64x64 pixels
depending on what size of the threadgroup is chosen. This kernel counts
how many pixels are associated with every id in a specified range within
a processed micro tile. The widest supported range of ids is \[0:2047\].
Just before such threadgroup finishes, it increments global counter per
each id per macro tile.

The following one or two kernels (depending on the number of macro
tiles) compute global exclusive prefix sum of counters written by the
first kernel.

The last kernel starts with the same set of operations as the first
kernel but also writes pixel coordinates to appropriate locations of the
indirection buffer after fetching global inclusive prefix sum. This step
finalizes the binning pass. The indirection buffer containing pixel
coordinates is ready to use when this step finishes.

There's also an optional kernel which outputs a buffer of arguments for
*ExecuteIndirect*. This kernel is used only for *global* binning. It
computes indirect dispatch arguments and root constants for every id.

This gives an opportunity to dispatch unique shader for every id.

After The binning pass completes, there are several optional
visualizations passes and the pass which does correctness check. The
latter finishes the sample's rendering loop.

# Known issues

-   There's no special handling of ids that should be excluded from
    binning/classification. This means that handling them still have a
    non-zero cost.

# References

# 

Deligiannis, J., & Schmid, J. (2019). "It Just Works": Ray-Traced
Reflections in 'Battlefield V'. *GDC*, (pp. Slide 20-24). Retrieved from
https://developer.download.nvidia.com/video/gputechconf/gtc/2019/presentation/s91023-it-just-works-ray-traced-reflections-in-battlefield-v.pdf

Garawany, R. E. (2016). Deferred Lighting in Uncharted 4. *Siggraph.
Advances in Real-Time Rendering course*, (pp. Slide 21-31). Retrieved
from https://advances.realtimerendering.com/s2016/s16_ramy_final.pptx

Quilez, I. (2013). *Noise -- simplex -- 2D*. Retrieved from Shadertoy:
https://www.shadertoy.com/view/Msf3WH

Turanszki, J. (2020, January 5). *Tile-based optimisation for
post-processing*. Retrieved from Blog:
https://wickedengine.net/2020/01/05/tile-based-optimization-for-post-processing/

# Update history

-   April 2020: Preview release

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
