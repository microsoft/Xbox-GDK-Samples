# Simple Spatial Sound Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

# Description

This sample demonstrates how use ISpatialAudioClient to playback static
audio with height channels using Windows Sonic technologies. There are
two file choices for playback with keyboard controls to start/stop,
pause/play, and file select.

![](./media/image1.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

| Action                                    |  Controller               |
|-------------------------------------------|--------------------------|
| Start/Stop                                |  A button                 |
| Cycle Next File                           |  B button                 |
| Exit                                      |  View button              |

# Implementation notes

This sample demonstrates how to use ISpatialAudioClient to play static
bed sound using spatial technologies. Once ISpatialAudioClient has been
initialized and started, it uses the callback to request buffer frames.

# Update history

Initial release March 2019
