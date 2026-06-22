  ![](./media/image1.png)

# Unity Simple MPSD for Desktop

This sample is compatible with:

- [Microsoft GDK](https://github.com/microsoft/GDK/releases) or [GDKX](https://www.microsoft.com/en-us/software-download/gdk) - October 2023 Update 4 & Later

- [Unity Editor](https://unity.com/releases/editor/archive) - 2022.3.28f1 & Later

- [GDK Unity Package](https://github.com/microsoft/gdk-unity-package/releases) - March 2024 Update 1 & Later

*If targeting older versions of the GDK and Unity Editor, use the March 2024 version of the **GDKX Unity Samples** available from the GDK Download site:*

![Image of older version of Unity Samples download option](./media/UnityGDKSamplesDownload.png)

# Description

This sample is a port of the C++ SimpleCrossGenMPSD sample that is commonly
included in the **Microsoft GDK (Samples only) compatible with all GDK versions** package that is available from the Microsoft GDK download site.

It is a simple multiplayer session sample app which demonstrates Xbox Live, raw MPSD, and SmartMatch APIs that a developer would use to perform the following functionalities:

- Login into Xbox Live services

- Retrieve Xbox Live profile information

- Start an Xbox Live multiplayer session using MPSD

- Invite Xbox Live friends to join your multiplayer session

- Join a friend's Xbox Live multiplayer session

- Start and cancel a SmartMatch multiplayer session

- Leave a multiplayer session

# Notable Code Files

The sample code, found under the "Assets\Sample\Scripts" folder, is broken down primarily into UI/View related code, and core logic.
Within the Logic folder, the code is further separated between Xbox Live/Multiplayer specific logic and other generic logic bits.

- Xbox Live login and profile support are found in ..\Logic\XboxLive\XboxLiveLogic.cs and XboxLiveSocialLogic.cs

- MPSD usage is found in ..\Logic\XboxLive\XboxSessionManager.cs

- UX/View functions for login are in Assets\Sample\Scripts\Views

# Building the Sample

**IMPORTANT** This sample **requires** the [GDK Unity Package](https://github.com/microsoft/gdk-unity-package/releases).
Both the *'GDK-APIs'* & *'GDK-Tools'* provided in the package must be included in order to successfully build and run the sample.

Use the following steps to import the package:

1. Download the GDK Unity Package from GitHub.

2. In Unity, use **Assets > Import Package > Custom Package** and select the GDK Unity package on your PC.

3. This sample already contains a **GDK-Tools\ProjectMetadata** folder which holds a pre-configured '*MicrosoftGame.Config*' file.
Uncheck the 'ProjectMetadata' folder to preserve the sample's configuration:

![Image showing package import](./media/packageImport.png)

**Note:** If you accidentally overwrite this file, you can copy the contents of MicrosoftGameConfig.mgc into MicrosoftGame.Config (Assets\GDK-Tools\ProjectMetadata) to restore the sample's configuration.

4. Select the '*Import*' button.
After the package import completes, your '*Project'* folder should mirror the image below:

![Image showing GDK package assets](./media/projectAssets.png)

5. Open **GDK > PC > Build and Run** and check the 'Define MICROSOFT_GAME_CORE' checkbox.
Use this page when you want to generate a packaged version of your game for testing or to upload to Partner Center.

![Image showing Build and Run option for PC](./media/gdk_pcBuildSettings.png)

6. Open **File > Build Settings** and ensure the target platform is set to 'Windows'.
Use '*Build*' or '*Build and Run*' to build the sample executable.

![Image showing Unity Build Settings](./media/buildSettings.png)

7. To run the sample within the Unity Editor, select **GDK > PC > Update Editor Game Config** and then press the '*Play*' button. For best visual results, set the Game window's display to **Full HD (1920x1080)**.

For more information, see the [*Unity End-to-End Guide*](https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/get-started-with-pc-dev/get-started-with-unity-pc/gdk-unity-end-to-end-guide) in the GDK documentation.

# Running the Sample

If you have managed to successfully build, possibly package, and side load the app onto your development Windows 10 PC, you should see an app icon within the Windows 10 Start menu that is named "UnitySimpleMPSD."
Clicking on the app icon should launch the sample into a 1920x1080 window
that is non-resizable.

To successfully run, it is important that the PC is set to use the "XDKS.1" development sandbox (or your own sandbox) which the title has been configured and deployed to.
A GDK tool named "XBLPCSandbox" (run from a Gaming VS 2019 Command Prompt") can assist with switching the active sandbox. You will also need a batch of test users who can login to the sandbox (all @xboxtest.com accounts have access to XDKS.1).

The next sections will cover the sample functionality, and explain how the sample is expected to behave in a correctly working GDK development environment.

## Sample Main Screen

![Graphical user interface, application Description automatically generated](./media/image9.png)

The above screenshot shows the expected main screen when the sample is launched.
The title will immediately launch a user login flow for Xbox Live.
If any of the login steps fail, an error message will be presented to the user, and the sample app will cease to function any further.

Common failure situations might include: the user does not have
access to the current sandbox, internet connectivity is currently down,
or Xbox Live services are experiencing a service disruption of some sort (which could possibly be the result of not running a configured Web
services proxy).

## Sample Main Menu

![Graphical user interface, text Description automatically generated](./media/image10.png)

Once a test user has been successfully signed into Xbox Live, the sample's main menu screen will appear.
The three main functionalities presented are:

1. Match my user with other user(s) running the title using SmartMatch.

2. Have my user host a game lobby session, and invite other users to join it.

3. Directly join a session associated with an invitation that was already received and accepted by the test user.

A failure to perform any of the three capabilities above will result in an error message displayed in the sample's console log UI and in the user remaining at the main menu screen in the previous enabled state.

## Finding a SmartMatch Session

![Text Description automatically generated](./media/image11.png)

When choosing the "Start Matchmaking" button option from the main menu, the sample will immediately attempt to matchmake through a matchmaking ticket managed by the Multiplayer Session and Matchmaking APIs.
The matchmake ticket will generally lead to one of two common results:

1. The matchmake ticket could be fulfilled and the matchmake session was created, and all of the members that were match-made together were placed into the new session.
Two or more users must be using Matchmaking for a match to be found.

2. The matchmake ticket could not be fulfilled and timed out.
If that is the result, a message is shown in the log window, and the available main menu options are re-enabled.

A matchmaking ticket can also be cancelled so long as it is still active.
If the matchmaking ticket is cancelled, then the main menu options are re-enabled as if the matchmake request failed.

## The Session "Lobby"

![Graphical user interface, text Description automatically generated](./media/image12.png)

Once the test user has either: successfully been match-made, successfully joined a friend's game lobby through the "Join Session" main menu options, or successfully started to host their own session, the user now has the ability to leave the session.

The console log window UI will show the members that have subsequently joined or left the lobby session.
The "Leave" option allows the user to abort being in the lobby, and if chosen by the user, will result in that member being removed from the lobby member list.

## Host Invite Friend into Lobby

![Graphical user interface, application Description automatically generated](./media/image13.png)

If the test user is hosting the game lobby through the "Host Session" main menu option, then the "Invite Friend" button will be enabled on the sample menu.
Choosing the "Invite Friend" button will present the user with a Shell UI screen that presents a list of friends that can be sent invites to join the current game session.

Behind the scenes, the invite APIs are invoked to handle the selection of friends and the server-side invitation protocol mechanics.
If the invitation is not cancelled, then the recipient users, whether in the app or not, will receive a Shell UI notification that they have been given an invitation and they can either accept that invitation, or dismiss it.
If it is accepted, and the user is not currently in a session, the "Join Session" button is enabled on the main menu.

## Invite from Friend Received Screens

![Graphical user interface, application, Teams Description automatically generated](./media/image14.png)

![Graphical user interface, text Description automatically generated](./media/image15.png)

As mentioned previously, when accepting an invitation given by a host user, the main menu screen will display a "Join Session" when the invite is accepted.

# Partner Center Configuration

**App overview of the "SimpleCrossGenMPSD" title on Partner Center**

![A screenshot of a computer Description automatically generated with medium confidence](./media/image6.png)

The above screenshot shows the current state of the "SimpleCrossGenMPSD" game title on Partner Center.
The title has been configured and deployed to the XDKS.1 sandbox. There is a total of six multiplayer session template(s) configured for the title: three simple and three cross-gen, along with two SmartMatch hopper(s).

**Multiplayer session templates**

![A screenshot of a computer Description automatically generated with medium confidence](./media/image7.png)

In a common Multiplayer Manager scenario (consult the GDK documentation for further details) we maintain at least 3 session templates: one for the lobby, one for hosted gameplay (the same one *could* be used for matchmade gameplay), and one dedicated for matchmaking.

With the lobby template, we limit the number of participants that a lobby can have to 8.
The invite protocol specifies that the invitation should be given to the game application.
Also, connectivity is required of the members, and this is managed through the RTA (real time activity) service.

We have also not elected to make the session be searchable through the absence of the "searchable" property, and the sample does not demonstrate any search capability.
Finally, we also do not specify the number of members needed to start the lobby session, but the sample implicitly expects it to be one.

**Session templates**

```JSON
{
   "constants": {
        "system": {
            "version": 1,
            "maxMembersCount": 8,
            "visibility": "open",
            "inviteProtocol": "game",
            "capabilities": {
                "connectivity": true,
                "connectionRequiredForActiveMembers": true,
                "gameplay" : true,
                "crossPlay": true,
                "userAuthorizationStyle": true
            },
        },
        "custom": {}
    }
}
```

**SmartMatch Hopper**

![A screenshot of a computer Description automatically generated with medium confidence](./media/image8.png)

# Packaging

If a package has previously been submitted to Partner Center (as of June 2024, this sample does not have a package uploaded for XDKS.1), the ContentId override must be present when creating a new package via **GDK > PC > Build and Run**.
The ContentId override should match the ContentId of the Store-installed package.

Follow these steps to find and set the ContentId override:

1. Uninstall any previous instance of the game.

2. Download the store package fully for the sandbox you are targeting (run `ms-windows-store://pdp/?productid=YourStoreId` and click 'Install').

3. With the package installed in step 2 above, obtain the ContentId from the registry (`Computer\HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Store\ContentId`).

4. Add the ContentId override to the MicrosoftGame.Config file.

    - Open **Assets\GDK-Tools\ProjectMetadata\MicrosoftGame.Config**.

    - Set the ContentId override to the value obtained in step 3 above.
    The value shown below is for another product in XDKS.1:

```xml
  <DevelopmentOnly>
    <ContentIdOverride>0B04E5FB-17E2-4CD7-9501-BB308FA9FC76</ContentIdOverride>
  </DevelopmentOnly>
```

5. Uninstall the store package, use **GDK > PC > Build and Run > Build** to generate a new package with the ContentId.

# Known Issues

The sample was developed and tested against the versions listed at the top of this document.
Integrating a newer version of the Microsoft Unity GDK plugin, using Unity's GameCore package, or integrating a different version of the native GDK or Unity plugin may expose incompatibilities with APIs that may be deprecated, or have altered behavioral characteristics.

To use the GDK Unity package, be sure to define "MICROSOFT_GAME_CORE" as mentioned in [Building the sample](#building-the-sample).

To send a joinable invite from PC, the game must be registered using ```wdapp register {full path to the directory containing the MicrosoftGame.Config, game EXE and Store image files}``` or by installing a packaged build of your game (see [Packaging](#packaging) for more information).

On PC, game invites will appear in the Xbox App. However, at the time of this writing, notifications in the Xbox App were only refreshed at launch. If you're expecting a notification but don't see one, close the Xbox App and then reopen it to get the latest game invites.

# Update History

| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of README and sample. Includes build requirements, usage details, notes and issues. |  September 2021  |  1.0 |
| Updated the sample to run on Unity 2022.3.28f1 and the latest (legacy) GDK Unity Package on GitHub. Future versions of this sample will use the new Microsoft GDK Packages (com.unity.microsoft.gdk and com.unity.microsoft.gdk.tools) available in Package Manager. | June 2024 | 1.1 |

# Privacy Statement

This sample adheres to general Microsoft privacy guidelines regarding
the distribution of sample source code, documentation, or other
material, for the sole private and individual usage by the prospective
developer of the APIs referenced within.

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
