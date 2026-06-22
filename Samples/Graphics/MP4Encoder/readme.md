  ![](./media/image1.png)

#   MP4Encoder Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# 

# Description

This sample shows how the Media Foundation APIs can be used to encode
the back buffer of a game into a MP4 video file using the H264 codec.

![A picture containing chart Description automatically generated](./media/image3.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

This sample uses the following controls.

| Action                                       |  Gamepad               |
|----------------------------------------------|-----------------------|
| Stop encoding                                |  A                     |
| Exit                                         |  View Button           |

# Implementation notes

This sample uses a compute shader to convert its back buffer to a NV12
texture which can be used for video encoding. The NV12 texture contains
two parts, a full resolution plane 0 with the luminance data (Y) and a
half resolution plane 1 with the chroma data (UV). The compute shader
does a simple bilinear downsample to calculate chroma.

The Media Foundation APIs are used to utilize the Xbox hardware encoder
to encode the NV12 texture into a MP4 video file with a H264 video
stream. Two files are written to the devkit's system scratch drive, one
with the raw H264 video stream and an MP4 file containing the H264 video
stream. These files can be found using: `xbdir
xd:\MP4EncoderSampleOutput.mp4`

A title can use the Xbox hardware encoder to encode at a maximum of
1080p @ 30Hz, because the encoder is also used at a system level for
Game DVR. Even though it can only encode at 30Hz, the sample renders at
60Hz. Therefore, the sample creates a separate thread for encoding to
avoid stalling rendering.

To easily view the MP4 file, you can either use xbcp to copy the file or
use the file explorer feature from the Xbox Manager tool.

![Graphical user interface, application Description automatically generated](./media/image4.png)

![Graphical user interface Description automatically generated](./media/image5.png)

![Graphical user interface, text, application Description automatically generated](./media/image6.png)

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
