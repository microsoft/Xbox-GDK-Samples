# Simple Device and SwapChain Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

This sample demonstrates how to create a Direct3D 12 device and PresentX
swap chain for an Xbox title.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using an Xbox One X|S devkit, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample has no controls other than exiting.

# Implementation notes

While the Xbox title Direct3D setup is very similar to other Microsoft
platforms, this sample demonstrates a few key differences:

-   Using **D3D12XboxCreateDevice** instead of the standard
    D3D12CreateDevice

-   Using 4k for Xbox Series X / Xbox One X, 1440p for Xbox Series S,
    and 1080p for Xbox One

-   Instead of using DXGI for presentation, this uses the new
    **PresentX** API

For more information on best practices for Direct3D 12 device creation
and swapchains, see [Anatomy of Direct3D 12 Create
Device](https://walbourn.github.io/anatomy-of-direct3d-12-create-device/)
and [The Care and Feeding of Modern Swap
Chains](https://walbourn.github.io/care-and-feeding-of-modern-swapchains/).

For details on the use of the loop timer, see
[StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer).

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update History

October 2018 -- Initial version for Microsoft GDK

October 2019 -- Switched to XSystemGetDeviceType for console detection

October 2021 -- Updated to use 1440p on Xbox Series S

August 2022 -- Improved PresentX best practice for where to wait for
origin event.
