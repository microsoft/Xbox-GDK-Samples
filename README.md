# Xbox-GDK-Samples

This repo contains game development samples written by the Microsoft Xbox Advanced Technology Group using the Microsoft Game Development Kit (GDK).

* [Kits](/Kits) contains support code used by the samples
* [Media](/Media) contains media files used by the samples
* [Samples](gdk-samples-list.md) contains samples
  * [Audio](gdk-samples-list.md#Audio)
  * [Graphics](gdk-samples-list.md#Graphics)
  * [Handheld](gdk-samples-list.md#Handheld)
  * [IntroGraphics](gdk-samples-list.md#IntroGraphics)
  * [Live](gdk-samples-list.md#Live)
  * [System](gdk-samples-list.md#System)
  * [Tools](gdk-samples-list.md#Tools)
  * [Unity](gdk-samples-list.md#Unity)
  * [xCloud](gdk-samples-list.md#xCloud)

# Requirements

* Visual Studio 2022 or later
* __October 2025__ Microsoft Game Development Kit ([GDK](https://aka.ms/gdk))
* -or- Microsoft Game Development Kit with Xbox Extensions ([GDKX](https://aka.ms/gdkx))

https://aka.ms/gdkdl
 
https://github.com/microsoft/Xbox-GDK-Samples
 
## ATTN: Samples Now Require the October 2025 GDK

### What is the new GDK layout?

With the release of the October 2025 (2510) GDK comes the new GDK layout. The new layout is an updated directory structure for the GDK installation that replaces the **Gaming.Desktop.x64** build target with classic **x64**. It also reorganizes the resources available in the GDK install, requiring updated path and dependency configurations.
The old layout (pre-2510) will be deprecated some time in 2026. To get ahead of this, the samples were proactively converted to use the new layout and removing support for the old one.
 
### What does this mean for developers?
 
Going forward, the samples distributions will only use the new GDK layout, requiring a 2510 GDK or later installation. If developers can’t upgrade, older sample releases can be used instead.

*Note that the 2510 GDK includes the option to install the old layout next to the new one to prevent immediate breaks for existing titles. Unity samples currently still use the old layout, so be sure to use this option if you are developing with Unity.*
 
### Why not keep both layouts for the samples?
1. Clarity: Supporting both layouts would require samples to have both an **x64** and a **Gaming.Desktop.x64** configuration, which would negatively affect the clarity of sample configurations.
2. Planning ahead: Since the old layout will be deprecated some time in 2026, doing a full conversion to the new layout is a better long-term solution.

# Opening samples

To open most samples, open the Visual Studio `.sln` file included in the sample's directory.

For Unity samples, use Unity Hub's '+ New project' option and select the Unity sample's directory that contains the `Assets` directory.

Each sample has a readme markdown file which provides directions on how to run it and other important information.

## Privacy Statement

When compiling and running a sample, the file name of the sample executable will be sent to Microsoft to help track sample usage. To opt-out of this data collection, you can remove the block of code in ``Main.cpp`` labeled _Sample Usage Telemetry_.

For more information about Microsoft's privacy policies in general, see the [Microsoft Privacy Statement](https://privacy.microsoft.com/privacystatement/).

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft trademarks or logos is subject to and must follow [Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general). Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship. Any use of third-party trademarks or logos are subject to those third-party's policies.

## Other Samples

For more ATG samples, see [DirectML Samples](https://github.com/microsoft/DirectML), [PlayFab-Samples](https://github.com/PlayFab/PlayFab-Samples), [Xbox-ATG-Samples](https://github.com/microsoft/Xbox-ATG-Samples), and [Xbox-LIVE-Samples](https://github.com/microsoft/xbox-live-samples).

## Samples list by category

[Samples List](gdk-samples-list.md)
