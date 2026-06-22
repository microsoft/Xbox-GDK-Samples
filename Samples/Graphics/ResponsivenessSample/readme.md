  ![](./media/image1.png)

#   Responsiveness Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

The sample provides a variety of examples to show how DLI and display
technologies can improve responsiveness. **Note that you will need a
display with HDMI 2.1, VRR, and support for a 120Hz refresh rate to take
full advantage of this sample. Unless noted otherwise, ensure your
console is set to run at 120Hz.** There are key concepts to understand
before running this sample:

- 120Hz Refresh Rate: At this point in time, most displays only
    support a 60Hz refresh rate. More displays are supporting 120Hz
    refresh rate. Xbox consoles support this mode and it can be
    enabled/disabled in your display settings.

- Variable Refresh Rate (VRR): A display technology that allows the
    source device to push updates to the display at any frequency in the
    display's supported range.

- Dynamic Latency Input (DLI): A new technology in Xbox Series X|S
    that schedules controller update times to minimize latency between
    when a button is pressed and when a game queries for input state.

Here are the different modes:

**60 FPS Happy: A happy 60 fps title with consistent frame rate**

A game running at a consistent 60 frames per second.

**55 FPS Stutter @ 60 Hz Transmission: Up to 16ms of stutter**

**Ensure your console is set to 60Hz!** A game running at 55 frames per
second with the console disabling VRR. While 55fps might seem like a
high frame rate, notice how dramatic stuttering can be when rendering
just barely misses the VBLANK.

**55 FPS Variable Refresh Rate: HDMI Transmission will vary to match the frame rate**

**Ensure your console is set to 60Hz!** A game running at 55 frames per
second with VRR enabled. While there is still some stutter, notice how
much smoother the game appears with VRR.

**55 FPS @ 120 Hz Transmission: Significantly reduced 8.3ms of stutter**

A game running at 55 frames per second with VRR disabled, but refreshing
at 120Hz. While there is still some stutter, notice how much smoother
the game appears because the time of the stutter is only half that of a
60Hz refresh.

**120 FPS Title**

A game running at a consistent 120 frames per second.

**60 FPS Title**

A game running at a consistent 60 frames per second.

**30 FPS Title**

A game running at a consistent 30 frames per second.

**40 FPS Title @ 120 Hz Transmission: Every frame is transmitted 3 \* 8.3ms = 25ms (40fps)**

A game running at a consistent 40 frames per second. While traditionally
this was an odd framerate, there will be no stutter when running on a
120Hz display. This is an excellent option for games that cannot
maintain 60fps when a player has a display that supports 120Hz.

**Triple Buffering w/ Period 3: 3 frames of latency worse case**

A game that is triple buffered, thus adds 3 frames of latency. Tap the B
button to introduce frame delays and notice how little the game is
impacted in this case.

**Double Buffering: 2 frames of latency**

A game that is double buffered, thus adds 2 frames of latency. Tap the B
button to introduce frame delays and notice how dramatically the game is
impacted in this case.

**Double Buffering: When frame rate is missed, the penality is severe**

A game that is double buffered, thus adds 2 frames of latency, and is
commonly missing its frame time. Notice how dramatically the game is
impacted in this case.

**PresentX() allows Triple Buffering w/ Period 2: Only 2 frames of latency and smooth triple buffering**

A game running with PresentX's new triple buffer. Notice that it feels
the same as the double buffered example, yet tapping the B button
introduces stutter like it is triple buffered. That is because it is
prioritizing latency and will be double buffered unless frame times get
to long, at which point it will introduce a third buffer to minimize
stutter until it is safe enough to return to a double buffer.

**Super-responsive: Double-Buffering /w Period 1: Only 1 frame of latency (minus 1 ms offset)**

Example of near-minimal latency. Period of 1 means the title must build
frame from beginning to end within one frame interval. The frame origin
can even be scheduled later than the vsync by a title-chosen amount.
These settings would be extremely ambitious for a real game, and the
mode is included primarily for demonstration purposes.

**Ultra-responsive: Double-Buffering /w Period 1: Only 1 frame of latency at 120 Hz (minus 1 ms offset)**

Example of near-minimal latency at 120 Hz. Period of 1 means the title
must build frame from beginning to end within one frame interval. The
frame origin can even be scheduled later than the vsync by a
title-chosen amount. These settings would be extremely ambitious for a
real game, and the mode is included primarily for demonstration
purposes.

**DLI Off**

DLI has been manually disabled.

**DLI On**

DLI has been manually enabled.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using a Xbox Series X|S devkit, set the active solution platform to
Gaming.Xbox.Scarlett.x64

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Left/Right on the Dpad: Select the mode

Left stick: Move target reticule

X/A: Fire weapon

B: Delay the GPU, forcing missed frames

# Known issues

\[none\]

# Update history

Initial release June 2020

Aug 2021: Add 60FPS Plus mode

Oct 2022: Add super-responsive and ultra-responsive modes

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
