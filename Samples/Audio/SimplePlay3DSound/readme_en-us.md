# Simple Play 3D Sound Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

# Description

This sample demonstrates how use XAudio2 and X3DAudio to playback
positional audio on Xbox One. The listener is static (represented by the
white triangle) and emitter (represented by the black triangle) can be
moved in 3D space, though the view is top down. The circle around the
emitter represents the end of the attenuation curve with the lines
showing the inner and outer bounds of the emitter's cone. For detail on
these terms, please see [Common Audio
Concepts](https://msdn.microsoft.com/en-us/library/windows/desktop/ee415692%28v=vs.85%29.aspx)

![](./media/image1.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

| Action                              |  Gamepad                        |
|-------------------------------------|--------------------------------|
| Move the emitter                    |  Left thumbstick                |
| Rotate emitter                      |  Right thumbstick               |
| Adjust emitter height               |  Left/Right shoulder buttons    |
| Reset emitter location              |  Left/Right thumstick           |
| Change reverb type                  |  DPad Up/Down                   |

# Implementation notes

This sample demonstrates how to use XAudio2 with X3DAudio to play
positional sound. Once XAudio2 has been initialized, a submix channel is
added for reverb and a wav file is played in an infinite loop. Each
update uses the current position of the emitter calculate the X3DAudio
DSP settings to account for position and direction.

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
