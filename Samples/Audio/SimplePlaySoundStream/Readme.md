# Simple Play Sound Stream Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

# Description

This sample demonstrates how to stream a wav file using XAudio2 on the
Xbox One.

![](./media/image1.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample has no controls other than exiting via the View button.

# Implementation notes

This sample demonstrates how to stream wav files using its own WAV file
parser.

For other examples of doing streaming with XAudio2, see
[GitHub](https://github.com/walbourn/directx-sdk-samples/tree/master/XAudio2):

-   **XAudio2AsyncStream** which prepares the .WAV data on disk to
    support Win32 non-buffered overlapped I/O

-   **XAudio2MFStream** which uses Media Foundation Source Reader to
    decompress the data from an WMA file.

-   *DirectX Tool Kit's* **SoundStreamInstance** which implements
    non-buffered overlapped I/O for all XAaudio2 formats.

# Known issues

This sample does not support streaming xWMA .wav files.

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
