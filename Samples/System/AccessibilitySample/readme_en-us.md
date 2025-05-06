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

This sample supports both Xbox and Desktop.

Privacy:

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, remove ATG_ENABLE_TELEMETRY from the
C/C++ / Preprocessor / Preprocessor Definitions list in the project's
settings.

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Third Party Notice

This sample demonstrates using the ImGui library which is available under the MIT license.

For more information, see https://github.com/ocornut/imgui
