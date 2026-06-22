  ![](./media/image1.png)

#   MP4Reader Sample (DirectX 12)

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# 

# Description

This sample shows how the Media Foundation Source Reader can be used to
read a MP4 file which contains an H264 or HEVC video stream, and decode
it using hardware acceleration. The sample also has a code path to
illustrate software decoding. It demonstrates decoding the audio stream
using XAudio2 and WASAPI. This sample is particularly useful for titles
that need to integrate H264/HEVC video decoding into their existing
movie playback pipeline.

This sample is not intended to demonstrate a movie playback solution, as
it does not support synchronized playback of video based on the video
source frame-rate.

![A picture containing outdoor Description automatically generated](./media/image3.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

This sample uses the following controls.

| Action                                        |  Gamepad          |
|-----------------------------------------------|-------------------|
| Toggle between Software and Hardware decoding |  X Button         |
| Toggle Audio (will restart the stream)        |  Y Button         |
| Toggle Looping on and off                     |  A Button         |
| Toggle loop method (seek vs restart media)    |  B Button         |
| Exit                                          |  View Button      |

# Implementation notes

This sample reads an MPEG 4 file, which contains a 1920x1080 H264 video stream, 
from the Media\\Videos folder, and renders the decoded frames to
the screen as soon as they are produced. By commenting the defined
INPUT_FILE_PATH macro and uncommenting the one below, a UHD HEVC video
stream can be ran instead. This last one is only supported on Xbox
Series X|S (and without Software Decoding).

By default the sample uses hardware accelerated decoding, but software
decoding can be enabled by pressing X on the gamepad. Changing from one mode to the
other will restart the current stream.

The Sample also supports and defaults to looping the video. The A button can be pressed
to toggle looping on and off. Two looping mechanisms are available: Seek will restart the media by positioning the reader at the start of the stream. Media restart will, as the name suggest, attempt to restart the video and audio player.

The audio streams are decoded using Microsoft Media Foundation and can
be configured to render audio with XAudio2 or WASAPI. This is controlled
by modifying which preprocessor define is set at the top of MP4Reader.h:
```cpp
//
// Use one of these two definitions to see how the two different technologies perform.
//

//#define USE_XAUDIO2
#define USE_WASAPI
```

vs.

```cpp
//
// Use one of these two definitions to see how the two different technologies perform.
//

#define USE_XAUDIO2
//#define USE_WASAPI
```

# Known issues

None

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
