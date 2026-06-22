  ![](./media/image1.png)

#   Social Sample (Windows PC)

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

# 

# Description

This sample demonstrates the Social Manager C-API provided by the
Microsoft Gaming SDK (GDK).

This sample includes scenarios for:

-   Adding users and creating groups

-   Retrieving social groups based on filters

-   Responding to Social Manager events

# Using the sample

The sample is controlled using a standard gamepad or keyboard. The
bottom of the screen displays the input legend with available actions.

## Sample Screen

![A screenshot of a computer Description automatically generated](./media/image3.png)

| Action                      |  Gamepad                                |
|-----------------------------|----------------------------------------|
| Sign in user                |  Menu button / Tab key                  |
| Refresh UI for current filter |  A Button / F5 key |
| Change social group viewed  |  LB and RB shoulder buttons / Left or Right arrow keys                       |
| Exit                        |  View Button / ESC key                  |

# Implementation notes

The code that directly interfaces with Social Manager C-API is
encapsulated into the SocialManagerIntegration.cpp file.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update history

**Updated:** *July 2021*

**Initial Release:** *September 2019*
