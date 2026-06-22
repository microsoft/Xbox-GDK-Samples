# Simple XAPU Sample

*This sample is compatible with the Microsoft Game Development Kit
Preview (November 2019)*

# Description

This sample demonstrates how to stream an Opus OGG file using XAudio2 on
Scarlett.

![](./media/image1.png)

# Building the sample

This sample only works on Project Scarlett.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample has no controls other than exiting via the View button.

# Implementation notes

This sample demonstrates how to stream Opus files using basic OGG
parsing.

For other examples of doing streaming with XAudio2, see
[GitHub](https://github.com/walbourn/directx-sdk-samples/tree/master/XAudio2):

-   **XAudio2AsyncStream** which prepares the .WAV data on disk to
    support Win32 non-buffered overlapped I/O

-   **XAudio2MFStream** which uses Media Foundation Source Reader to
    decompress the data from an WMA file.

# Known issues

This sample does not support OGG metadata frames.

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
