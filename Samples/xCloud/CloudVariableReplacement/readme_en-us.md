# Cloud Variable Replacement Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

This sample demonstrates how to alter the state of the touch adaptation
kit from within a game.

![Text Description automatically generated](./media/image1.jpeg)

# Using the sample

Ensure that Game Streaming is enabled before launching the sample.
Connect to the console running the sample using any compatible client
app (such as the Xbox Game Streaming Test App). Once you are connected,
the sample should change to reflect presence of a streaming client.
Ensure the "standard-controller" TAK is loaded.

Press the trigger buttons to change the opacity of the B button on the
TAK. Press dpad up to toggle the visibility of the Y button and dpad
down to toggle the enabled state of the A button.

# Implementation notes

This sample demonstrates how to use the cloud aware API for xCloud.

Layouts are in the sample-layout directory and are from sample layout
GitHub:
<https://github.com/microsoft/xbox-game-streaming-tools/tree/master/touch-adaptation-kit/touch-adaptation-bundles>

# Version History

July 2021: Initial sample

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
