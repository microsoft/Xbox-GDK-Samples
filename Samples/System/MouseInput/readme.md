  ![](./media/image1.png)

#   Mouse Input Sample

*This sample is compatible with the Microsoft Game Development Kit
(August 2020)*

# Description

This sample demonstrates how to implement mouse controls with GameInput.
Two different mouse controls are demonstrated: relative mouse control
and clip cursor mouse control capturing the windows mouse to stay within
the window of the app or game.

# Using the sample

This sample uses mouse control for interaction with the UI and game
modes. Press the right and left mouse buttons to exit the different game
modes and return to the UI.

# Implementation notes

Mouse is used in two ways in this sample: low latency raw deltas using
GameInput and absolute position calculated by the OS.

## Relative Mode (First person shooter)

This mode is sometimes referred to as "mouse-look" or "mouse-move"
camera mode. Some apps or games might prefer the mouse to be used as a
general input device based on the relative movement of the mouse instead
of the mouse location. This is commonly seen in first person shooter
games.

To enter this mode in the sample, select the first person shooter tile
using the left mouse button. Press both the left and right mouse button
to exit.

## Edge Cursor Mode

In this mode, the mouse is constrained to the bounds of the
application's window and will scroll when approaching those bounds.
Rather than using GameInput for low latency input, we are getting the
processed absolute position calculated by the OS. To enter this mode in
the sample, select the Real-time Strategy tile using the left mouse
button. Press both the left and right mouse button to exit.

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
