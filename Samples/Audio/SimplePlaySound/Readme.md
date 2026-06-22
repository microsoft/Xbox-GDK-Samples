# Simple Play Sound Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

# Description

This sample demonstrates how to play a wav file using XAudio2 on the
Xbox One.

![](./media/image1.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample has no controls other than exiting via the View button. It
automatically advances through the sample wav files as each completes.

# Implementation notes

This sample demonstrates how to play PCM, ADPCM, xWMA, and XMA2 format
wav files. It uses helper code in the *ATG Tool Kit* files
**WAVFileReader.h/.cpp**. This implements a simple wav file parser,
along with code for computing the play time of the supported sound
formats.

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
