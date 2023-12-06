  ![](./media/image1.png)

#   SimpleDirectStorageCombo Sample

*This sample is compatible with the Microsoft Game Development Kit (October
2023)*

# Description

This sample shows how to use the Circular mode of the PIXBeginCapture API
to be always recording performance data into a circular buffer, and then save
the buffer to a capture file on demand.

# Building the sample

This sample supports the following platforms

-   Gaming.Scarlett.xbox.x64
-   Gaming.XboxOne.xbox.x64

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample will automatically start capturing in a circular buffer. 
Every 1000 frames, or when the 'a' button is pressed, the sample will 
save the capture to a file and then restart the capture. If the 'b' button
is pressed the buffer is flushed but the capture is discarded.

# Implementation notes

On Xbox, the API produces a 'pevt' file that must be converted to a Timing Capture
via the PIX UI.

The Xbox version of PIXEndCapture is asynchronous and returns E_PENDING until the
capture is fully stopped.

# Update history

Initial release October 2023

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
