# Simple Cloud Aware Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

This sample demonstrates how to detect game streaming clients, change
onscreen control layouts, and read touch points.

![A screenshot of a computer Description automatically generated with medium confidence](./media/image1.png)

# Using the sample

Ensure that Game Streaming is enabled before launching the sample. You
can do this in Dev Home in the Streaming section, or run xbgamestream
startlistening from Gaming command prompt.

Connect to the console running the sample using any compatible client
app (such as the Xbox Game Streaming Test App). Once you are connected,
the sample should change to reflect presence of a streaming client.

The sample includes a sample layout that can be loaded by running in
Gaming command prompt:

tak serve \--takx-file sample-layouts.takx

Press buttons to see them displayed and move the thumbsticks and
triggers to see their readings. Press A (or the A equivalent in the
overlay) to switch to a new overlay. If the client is touch enabled,
touch the screen to see the touchpoints being read.

# Implementation notes

This sample demonstrates how to use the cloud aware API for xCloud.

Layouts are from sample layout GitHub:
<https://github.com/microsoft/xbox-game-streaming-tools/tree/main/touch-adaptation-kit/samples>

# Version History

May 2021: Initial sample

March 2022: Update to correct initialization code

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
