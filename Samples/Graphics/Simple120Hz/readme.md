# Simple 120 Hz Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

This sample demonstrates the basics of rendering at several refresh
rates with the GXDK -- 30Hz, 40Hz, 60Hz, and 120Hz. New functionality is
demonstrated which specifies preference of HDR or refresh rate when the
display does not support HDR & 120Hz output simultaneously. To
facilitate HDR rendering the app utilizes Xbox Series X|S built in
hardware 3D LUT to produce the SDR image for GameDVR.

![](./media/image1.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Controls

| Action                                 |  Gamepad                     |
|----------------------------------------|-----------------------------|
| Select Refresh Rate                    |  DPad Up/Down                |
| Toggle HDR Mode Preference             |  A Button                    |
| Exit                                   |  View Button                 |

# Implementation notes

This sample demonstrates two crucial GXDK APIs for selecting a framerate
and dictating preference for HDR or 120Hz.

The first is `ID3D12Device::SetFrameIntervalX`, which sets the frame
interval in microseconds and the frame period in number of intervals,
together dictating the duration from frame start to frame presentation.
The selected frame interval must be supported by the display, which is
queried using `IDXGIOutput::GetDisplayModeListX` on Xbox Series X|S, or
`IDXGIOutput::GetDisplayModeList` on Xbox One.

The second is `XDisplayTryEnableHdrMode` which tries to enable the
display's HDR mode. It allows the user to specify a preference over HDR
or 120Hz refresh rate if the display does not support them
simultaneously. The title can change the preference at any point by
calling the function again. Note that this can change the results of
`IDXGIOutput::GetDisplayModeList(X)`, which entails a refresh of the
supported frame intervals.

Rendering for HDR displays entails titles concern themselves with
producing an SDR image suitable for GameDVR, screenshots, broadcasting,
and streaming. This sample demonstrates new Xbox Series X|S
functionality of a hardware 3D LUT to automatically perform the HDR to
SDR color conversion. This saves the title all the CPU, GPU, memory,
and/or bandwidth costs of manually converting or relying on the driver's
software 3D LUT commonly used on Xbox One. This is the default setting
when presenting a single display plane with
`DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020` color space, and omitting the
`D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP` flag on the
backbuffer textures.

# Update history

6/4/2020 -- Sample creation.

March 2022 -- 24Hz support added.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
