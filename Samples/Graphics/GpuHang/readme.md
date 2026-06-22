  ![](./media/image1.png)

#   GpuHang Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

The GpuHang sample is a testbed which reproduces a variety of commonly
encountered GpuHangs, producing xhit file dumps for postmortem
investigation. Examining these xhit files can teach us where to look for
information in debugging hangs which occur naturally.

# Building the sample

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

## Screenshot

![](./media/image3.png)

| Action                                 |  Gamepad                     |
|----------------------------------------|-----------------------------|
| Trigger selected hang                  |  A button                    |
| Take a PIX capture                     |  X button                    |
| Navigate list                          |  Left joystick               |
| Exit                                   |  View Button                 |

# Implementation notes

Each selection from the list on the left represents a potential hang
type. The selection runs without hanging until the user presses 'A'.
That action triggers a slight change in sample operation which
introduces a hang. At this point, the sample will usually terminate and
produce an xhit file which can be examined in PIX.

When the selection is not in hang mode, a press of 'X' will trigger a
PIX GPU capture. This capture can be useful in determining what the
normal, non-hanging sequence of operations is.

ATG encourages users to add their own hangs to the sample. To add a new
hang, simply add a new .cpp file to the Visual Studio project. The new
file should contain a class which derives from Hang, and which
implements the required virtual methods of IHang. Use the existing hangs
as a model. Add any required HLSL files to the project as well.

# Known issues

By design, the sample hangs the GPU, which often terminates the
executable.

The D3D12 validation layer catches some of the errors which lead to
hangs in this sample. For demonstration purposes, the relevant
validation checks are disabled in the sample. In some cases, the
validation checks cannot be cleanly disabled. The corresponding hangs
are marked in the list with a notation of "(\~V)" to indicate that the
examples won't work properly in validated builds.

# Update history

Initial release April 2019. Added Xbox Series X|S support in March 2020.
