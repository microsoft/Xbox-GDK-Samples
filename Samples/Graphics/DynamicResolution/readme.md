  ![](./media/image1.png)

#   Dynamic Resolution Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022).*

It does not currently support the Xbox One X console mode.

# Description

This sample demonstrates how to maintain frame rate by modulating
resolution in response to changing performance conditions.

# ![](./media/image2.jpeg)

# Building the sample

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

This sample uses the following controls.

| Action                                 |  Gamepad                     |
|----------------------------------------|-----------------------------|
| Navigate onscreen menu                 |  Left Thumbstick/D-Pad       |
| Toggle dynamic resolution on/off       |  Y Button                    |
| Pause camera                           |  X Button                    |
| Hide or show side panel                |  A Button                    |
| Exit the sample                        |  View Button                 |

# Implementation notes

## Algorithm

The goal of the sample is to make three determinations:

1.  At the current resolution, will the title drop frames in the
    immediate future?

2.  Will reducing resolution help avoid frame drops (i.e. is performance
    constrained by GPU load)?

3.  Could the title maintain frame rate at a higher resolution?

To test this goal, the sample produces a number of synthetic scenarios
which are meant to model different sorts of frame rate instability in
games:

1.  Transient spike in GPU load

2.  Prolonged overrun in GPU load

3.  Prolonged overrun in CPU load

The desired outcomes for these scenarios are (respectively):

1.  Reduce resolution, then raise resolution after spike passes

2.  Reduce resolution by the minimum amount necessary to maintain frame
    rate

3.  Do nothing (resolution does not affect CPU load)

The core algorithm of the sample resides in the function
`Sample::DetermineResolution`. Much of the other code in the sample is
dedicated to the frame rate simulation and to careful measurement and
graphing. That latter code need not be ported when implementing dynamic
resolution in a game engine.

The sample waits to modify resolution until the camera is in motion.
Resolution changes are noticeable in a static scene, but are usually
unnoticeable in a moving scene. For this reason, it's a good idea to
keep stationary elements like UI at a fixed resolution. The sample
achieves this goal by rendering the main scene to the background display
plane, and UI elements to the foreground display plane. The background
plane supports dynamically changing resolution in response to processor
load. The foreground plane renders at a fixed resolution of 1920x1080.

We recommend that when possible, titles maintain a fixed vertical
resolution and only reduce horizontal resolution. The reasons for this
recommendation are twofold:

1.  The hardware scaler takes more taps in the x-direction than in the
    y-direction, and thus does a better job of upscaling horizontally.

2.  Most game motion (camera and objects) tends to favor the
    x-direction, and therefore horizontal resolution is less noticeable.

## Aliased render targets

To allow an arbitrary number of resolutions without increasing memory
usage, the sample aliases all scene render and depth targets,
respectively, into the same memory. For example, when the sample starts
up, it creates a 1920x1080 render target. If the resolution changes to
1728x1080, a new *placed resource* is created at the same memory
allocation as the original render target. This will be used as the new,
smaller render target until the resolution changes again. Because only
one resolution is active at a time, the 1920x1080 and 1728x1080 render
targets can share the memory without conflicts. Depth targets work the
same way.

The **ResolutionData** struct holds the resources (descriptor handles
and D3D resources) needed to render at a particular resolution. The
**ResolutionSet** class manages a set of ResolutionData structs, and is
responsible for creating new placed resources if necessary when the
resolution changes. The sample only uses a single ResolutionSet to
render its scene, but a game could use more than one if it has multiple
render targets, for example in a deferred renderer.

Note that the sample performs a texture copy from its current render
target into the upper-left corner of the fixed-size swap chain. This is
only because the sample is simple enough to render everything in one
pass. In a real game, the final full-screen pass can render directly to
the upper-left of the swap chain. In either case,
`D3D12XBOX_PRESENT_PLANE_PARAMETERS::pSrcViewRects` is used when calling
`ID3D12CommandQueue::PresentX` to indicate which portion of the swap chain
to display.

## Analyzing results

The sample draws several graphs and indicators to help analyze
performance, instantaneously and over time:

**Graphs:**

-   Pixel count (%) -- the percentage of total pixels relative to the
    ideal resolution of 1920x1080.

-   Frame time (ms) -- the time between consecutive 'flips' of the swap
    chain. An individual dropped frame will show up as a spike in this
    graph.

-   Vsync margin (ms) -- the time between when a frame was ready and
    when it was scheduled to be displayed. An individual dropped frame
    will cause this graph to drop below the 0 ms baseline.

**Indicators:**

-   Average FPS -- the average frame rate over a sliding window.

-   Tear -- the vertical point on the screen at which tearing occurred
    (lags by a couple of frames). The top of the screen represents no
    tearing.

To understand how the dynamic resolution method affects observed
performance, watch the two indicators over several seconds, and then
toggle the method on/off using the B Button. You can also focus on the
rotating scene and attempt to pick out tears and dropped frames
directly.

**Title Performance Overlay:**

> For more info regarding this, refer to the GDK docs, specifically the section titled **Title Performance Overlay**.

> Only available on Xbox Series X|S. It’s not available on the Xbox One family of consoles

This tool is designed to provide an easy (visual) way to locate short duration, transient frame rate dips. Even single frame hiccups. These types of hiccups can be easy to miss when just playing a game. 

When active, the overlay will look like this:

# ![](./media/image3.png)

- The green line represents the average framerate of the title (over the past 60 frames). The goal is for this line to be as stable as possible.
- The yellow line shows the percentage of time the GPU was busy vs idle in the frame (GPU Busy %).
- The green and red lines appear whenever tearing happened in the frame, and the ratio between these lines represents where (vertically in the display) the tear happened for each frame.
- The blue bar at the bottom represents the frame time. Target for this sample is 16.66 milliseconds.
- The purple bar shows Dynamic Resolution Scaling. This value is the percentage of rendered resolution versus the maximum front buffer resolution. Here, this bar appears when the system lowers the render resolution in response to system performance issues.

To enable this overlay, you can do it by ticking the *Enable title performance overlay* checkbox on Xbox Manager, under the Debug tab (in console settings). 

# ![](./media/image4.png)

It can also be enabled via pix (in PIX System Monitor, by using the *Show FPS Overlay* button) or via the Xbox gaming command prompt, by writing the following command:
> xbconfig TitlePerformanceOverlay=true 

# Known issues

When building the Gaming.Xbox.XboxOne.x64 configuration, this sample
will not run on the Xbox One X console mode due to its use of multiple
display planes. You can run it on an Xbox One X development kit by
changing the console mode setting to Xbox One or Xbox One S. You can
also run the Gaming.Xbox.Scarlett.x64 configuration on a Xbox Series X|S
development kit.

When running the sample in the Debug build configuration, additional CPU
overhead may make dynamic resolution less effective, especially on Xbox
One console modes. Run in the Profile or Release configurations for best
results.

The dynamic resolution method requires careful tuning for each title.
Depending upon the title's performance profile, including both typical
and extreme conditions, various constants may need to be adjusted up or
down.

The sample may appear to briefly hang the console upon abnormal exit.
This behavior is due to the 'wait' mechanism used to enforce specific
processor loads for the purposes of experiment. The dynamic resolution
method will not cause such behavior in games.

# Update history

**June 2020:** Initial release

**April 2021:** Updated GPU load simulation technique

# 

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
