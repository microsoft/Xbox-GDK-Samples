<!-----
---
page_type: sample
languages:
- cpp
products:
- gdk
urlFragment: "accessibilitysample"
extendedZipContent:
- path: LICENSE
  target: LICENSE
- path: Kits
  target: Kits
- path: Media
  target: Media
description: "This sample demonstrates how to implement cross-platform accessibility features for a title."
---
----->

  ![](./media/image1.png)

#   Accessibility Sample (PC, XBOX)

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

![Accessibilty Sample with 2 windows](./AccessibilitySample.png)

# Description

This sample demonstrates the following accessibility features.
-	Text/background luminosity ratios > 4.5:1
-	All Non-Text Contrast (for icons, Focus) Luminosity ratio should be >=3:1
-	Focused/selected UI element are visually differentiated
-	Keyboard navigable UI Elements
-	Reads OS settings (where available) for both high-contrast dark and high-contrast light themes
-	Screen narration. 
-	No text overlap on OS text resize or DPI change.

These features are implemented through a wrapper for ImGUI.

# Sample architecture

![Accessibilty Sample with 2 windows](./AccessibilitySampleArchitecture.jpg)

# Building the sample

This sample supports both Xbox and Desktop. Build with Visual Studio 2022
(or greater) and the Microsoft GDK installed. Supported platforms are x64,
ARM64, Gaming.Xbox.Scarlett.x64, and Gaming.Xbox.XboxOne.x64.

# Using the sample

The sample opens two windows: an "Example UI" window with sample widgets
(buttons, a text field, and a slider) and a "Legend" window that lists the
navigation controls and the accessibility features in effect. Move focus
between widgets to hear them narrated, and toggle "Enable Narration" in the
Legend window to turn narration on or off. The theme (high-contrast dark or
high-contrast light) and the text scaling are read automatically from the OS
settings, so changing those Windows settings while the sample is running
updates the UI.

# Controls

The in-app Legend window lists the keyboard controls:

| Action | Keyboard |
| --- | --- |
| Switch window | Ctrl + Tab |
| Next widget | Tab / Down arrow |
| Previous widget | Shift + Tab / Up arrow |
| Activate widget | Enter |
| Un-focus widget | Esc |
| Horizontal scroll | Left arrow / Right arrow |

Gamepad navigation is also enabled (Dear ImGui gamepad nav): use the D-pad or
left stick to move focus, the A button to activate a widget, and the B button
to go back.

# Implementation notes

The accessibility features are implemented through ImGuiAcc, a wrapper around
Dear ImGui that adds:

- Focus-driven screen narration (the focused widget is announced).
- High-contrast dark and high-contrast light themes sourced from the OS
  settings, with text/background luminosity ratios that meet accessibility
  contrast guidance.
- Text scaling driven by the OS text-size setting, laid out to avoid overlap
  on DPI or text-resize changes.
- Keyboard- and gamepad-navigable widgets with a visually differentiated
  focus indicator.

Input is routed through GameInputManager (built on GameInput), and Dear ImGui
gamepad and keyboard navigation are enabled in Main.cpp.

# Privacy Statement

When compiling and running a sample, the file name of the sample executable
will be sent to Microsoft to help track sample usage. To opt-out of this data
collection, add ATG_DISABLE_TELEMETRY to the C/C++ > Preprocessor >
Preprocessor Definitions list in the project's settings.

For more information about Microsoft's privacy policies in general, see the
[Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Third Party Notice

This sample demonstrates using the ImGui library which is available under the MIT license.

For more information, see https://github.com/ocornut/imgui
