# HDR Auto Tone Mapping Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This sample shows how an HDR game can use auto tone mapping by the D3D
driver to produce an SDR image for GameDVR, as opposed to the title
rendering the SDR image itself. A title can use the default auto tone
mapper, or provide its own tone mapper as a 3D lookup table.

![](./media/image1.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample uses the following controls.

| Action                                    |  Gamepad                  |
|-------------------------------------------|--------------------------|
| Change auto tone map method               |  A button                 |
| Switch TV to HDR mode                     |  X button                 |
| Save tone mapper as a .DDS file           |  Y button                 |
| Previous image                            |  Left shoulder button     |
| Next image                                |  Right shoulder button    |

# Implementation notes for Xbox One

HDR games on Xbox One are required to output two swap chains, one with
10-bit HDR10 values presented to the HDR TV, and the other with an SDR
image which can be used for GameDVR, screenshots, streaming and
broadcasting. The SDR image can be rendered by the game, or be
automatically rendered by the D3D driver when auto tone mapping is
enabled by using the swap chain creation flag
D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP. The default
auto tone mapper produces a good SDR image, but will most likely not
have the same look as the game's own tone mapped SDR image. The API
SetHDRToneMapperX() gives a game the opportunity to provide its own tone
mapper so that auto tone mapping by the D3D driver can produce an image
similar to that of the game's own SDR image. It is safe to call the API
every frame, allowing a game to even generate the 3D LUT at runtime to
adjust the tone mapper dynamically for different scenes.

There are three ways to produce the SDR image:

1)  The game renders the SDR image. This is the recommended method,
    since this will result in the best image quality and GPU
    performance. The disadvantage is that it adds extra complexity to
    the render pipeline, i.e. extra engineering hours to implement and
    maintain, and could be undesirable for cross platform games. With
    this method, the game allocates two swap chains, renders the HDR and
    SDR images, then use PresentX to present both images.

2)  Default auto tone mapping by the D3D driver. This method is the
    easiest to implement. The title does not know about a GameDVR image,
    the D3D driver does all the extra work. There is no extra complexity
    to the render pipeline, but the image produced will most likely not
    look the same as the game's SDR image. With this method, the game
    allocates only one swap chain, renders only the HDR image, but still
    use PresentX with one swap chain. The D3D driver will allocate the
    extra swap chain from title memory, and inject a compute shader
    during the Present call to produce the SDR image.

3)  Auto tone mapping using the game's tone mapper. This method is
    similar to the previous method, but the game can specify its own
    tone mapper which the D3D driver will use during auto tone mapping.
    This method gives the best of both worlds, there is no extra
    complexity to the render pipeline, and the game can still control
    the look of the SDR image.

To enable HDR auto tone mapping, the sample uses the
`D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP` resource
creation flag when the swap chain buffer is created.

The sample starts up without switching the TV to HDR mode, since the
goal of the sample is to show the auto tone mapped SDR image, not the
HDR image. Pressing the X button will switch the TV to HDR mode.

The sample shows how to dynamically render a 3D LUT with the game's tone
mapper, how to save it as a .DDS file, and load it again. When pressing
the Y button, the current tone mapper is saved to the title scratch
folder and can be access here:

Refer to the white paper "[HDR on Xbox
One](http://aka.ms/hdr-on-xbox-one)" and the Xfest 2018 presentation
"Xbox One Enhancement for Display Output"

# Implementation notes for Xbox Series X

The display output hardware of Xbox Series X includes a hardware 3D LUT.
This means that there is no GPU or bandwidth cost to the title to
produce the SDR image. A title can still opt into using the legacy Xbox
One auto tone mapping that uses a compute shader on the GPU.

To use the system auto tone mapper using the Xbox Series X hardware 3D
LUT, a title simply renders the HDR10 swap buffer and it should *not*
specify any flags. To use the legacy Xbox One auto tone mapping, the
title *should* specify the flag
`D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP`. Refer to the
code in DeviceResources.cpp

# Known issues

None

# Update history

Initial release August 2019. Added support for Xbox Series X|S in
February 2020.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
