# Cloud Variable Replacement Sample

*This sample is compatible with the Microsoft Game Development Kit
(October 2022)*

# Description

This sample demonstrates how to alter the state of the touch adaptation
kit from within a game.

![Text Description automatically generated](./media/image1.jpeg)

# Using the sample

Ensure that Game Streaming is enabled before launching the sample.
Also ensure that on the streaming client (such as the Xbox Game 
Streaming Test App) that Settings > Developer > Touch adaptation > 
Enable sideload is enabled. Connect to the console running the sample 
using the streaming client app. Once you are connected, the sample 
should change to reflect presence of a streaming client.
Ensure the "sample-layouts" bundle is loaded by running the following
command in a Gaming command prompt:

```
tak serve --takxconfig sample-layouts\takxconfig.json
```

Press dpad left and right to change the opacity of the B button on the
TAK. Press dpad up to toggle the visibility of the Y button and dpad
down to toggle the enabled state of the A button.

# Implementation notes

This sample demonstrates how to use the cloud aware API for xCloud.

For more information, please see the following documentation:
https://learn.microsoft.com/gaming/gdk/_content/gc/system/overviews/game-streaming/building-touch-layouts/game-streaming-touch-changing-layouts-game-state

# Version History

April 2024:
- Updated sample to use the `OnClient` APIs to allow for per-client control of the on screen touch layout.
- Added a `takxconfig.json` to `sample-layouts`.

July 2021: Initial sample

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
