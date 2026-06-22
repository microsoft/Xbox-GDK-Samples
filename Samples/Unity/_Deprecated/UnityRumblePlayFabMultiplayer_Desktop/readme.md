  ![](./media/image1.png)

#   UnityRumble PlayFab Multiplayer Sample

*\* This sample has been developed with Unity 2021.3.0f1 and the
Microsoft June 2021 GDKX Update 4 (10.0.19041.7032) on Win 10.*

# 

# Description

This sample is a port of the UnityRumble sample that is commonly
included on the Microsoft GDK download portal page. This sample differs
from the vanilla UnityRumble sample in that it uses PlayFab Multiplayer
for sessions and matchmaking instead of using Multiplayer Manager.

It is a simple multiplayer game which demonstrates Xbox Live, PlayFab,
PlayFab Party, and PlayFab Multiplayer APIs that a developer would use
to perform the following functionalities:

-   Login into Xbox Live services & PlayFab services

-   Retrieve Xbox Live friend profile information

-   Start a PlayFab Multiplayer lobby

-   Invite Xbox Live friends to join your multiplayer lobby

-   Join a friend's PlayFab Multiplayer lobby

-   Matchmake Xbox Live users into a PlayFab Multiplayer lobby

-   Entering a PlayFab Party network which supports voice chat

-   Engage in gameplay using reliable and unreliable messaging

-   Use the Multiplayer Activity APIs for handling invites

# Building the Sample

This sample was developed and tested against the June 2021 Microsoft GDK
installation, and Unity version 2021.3.0f1. The sample depends on
several SDKS, all of which have snapshots provided in the SDKs folder.

**IMPORTANT:** When importing the gdk-pc unitypackage file, make sure to
not overwrite the MicrosoftGame.config file located in the
"Assets\\GDK-Tools\\ProjectMetadata" folder. You can avoid overwriting
this file by deselecting the checkbox for this folder during the import
process.\
\
![Graphical user interface, text, application Description automatically generated](./media/image3.png)

**IMPORTANT:** Once you have imported the playfab-multiplayer
unitypackage file, you must run the setup batch script located in the
"Assets\\PlayFabMultiplayerSDK\\Setup\\GameCore" folder. This batch
script will copy the correct versions of dlls that PlayFab Multiplayer
has dependencies on.

# Running the sample

To run the sample, first open the sample scene located in
"Assets/Sample/Scenes". Next you must set the PlayFab Title Id. You can
do this in the Unity editor by setting the titleID in the
PlayFabSharedSettings. To do this, first click on then PlayFab menu
option and then click on MakePlayFabSharedSettings. From there enter the
following titleId 78E25 and save your changes.

![Graphical user interface, text, application, chat or text message Description automatically generated](./media/image4.png)

![](./media/image5.png)

Next, open the "GDK -- PC Build and Run" menu. From the menu select
"GDK" -\> "PC" -\> "Build and Run". Ensure that the "Define
MICROSOFT_GAME_CORE" checkbox is checked and then select "Build and Run"
from the bottom right.

![Graphical user interface, application, Teams Description automatically generated](./media/image6.png)

![Text Description automatically generated](./media/image7.png)

# Important!

The SDK snapshots provided with the sample are purely meant to serve as
a convenience for running the sample and **should not** be treated as
"official" SDK versions suitable for shipping a published title with.
Always use the latest QFE your development is comfortable with for your
title's particular SDK versions.

# Known issues

The sample was developed and tested against the SDK snapshots provided
with the sample. Integrating a newer version of the Microsoft Unity GDK
plugin, using Unity's GameCore package, or integrating a different
version of the PlayFab, PlayFab Party, PlayFab Multiplayer, or Unity
plugin may expose incompatibilities with APIs that may be deprecated, or
have altered behavioral characteristics.

# Update history

| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of README for the sample. Includes build requirements, usage details, and notes and issues. |  November 3, 2021  |  1.0 |

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
