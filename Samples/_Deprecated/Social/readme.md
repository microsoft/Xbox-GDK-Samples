  ![](./media/image1.png)

#   Social Sample

*This sample is compatible with the Microsoft GDKX (June 2020)*

# 

# Description

This sample demonstrates the Social Manager C-API provided by the
Microsoft Gaming SDK (GDK).

This sample includes scenarios for:

-   Adding users and creating groups

-   Retrieving social groups based on filters

-   Responding to Social Manager events

# Building the sample

-   If using an Xbox One devkit, set the active solution platform to
    **Gaming.Xbox.XboxOne.x64**.

-   If using an Xbox Series X|S devkit, set the active solution platform
    to **Gaming.Xbox.Scarlett.x64**.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample is controlled using a standard gamepad or keyboard. The
bottom of the screen displays the input legend with all available
actions.

## Sample Screen

![Text Description automatically generated](./media/image3.png)

| Action                      |  Gamepad                                |
|-----------------------------|----------------------------------------|
| Sign in user                |  Menu button / Tab key                  |
| Refresh UI for current filter |  A Button / F5 key |
| Change social group viewed  |  LB and RB shoulder buttons / Left or Right arrow keys                       |
| Exit                        |  View Button / ESC key                  |

# Implementation notes

The code that directly interfaces with Social Manager API is
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
