# Advanced Spatial Sound Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This sample demonstrates how use ISpatialAudioClient to playback both
static and dynamic positional audio using Windows Sonic technologies.
The static bed plays on startup and dynamic sounds that follow random
paths can be added and removed

![](./media/image1.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

| Action                                               |  Controller    |
|------------------------------------------------------|---------------|
| Start/Stop Playback                                  |  A button      |
| Add a dynamic sound                                  |  DPad Up       |
| Remove a dynamic sound                               |  DPad Down     |
| Exit                                                 |  View button   |

# Implementation notes

This sample demonstrates how to use ISpatialAudioClient to play static
and dynamic positional sound using spatial technologies. Once
ISpatialAudioClient has been initialized and started, it uses the
callback to request buffer frames.

# Update history

Initial release March 2019
