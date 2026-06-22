  ![](./media/image1.png)

# Unity Simple MPSD for Xbox

This sample is compatible with:

- [Microsoft GXDK](https://www.microsoft.com/en-us/software-download/gdk) - October 2023 Update 4 & Later

- [Unity Editor 2022.3.28f1](https://forum.unity.com/threads/unity-2022-3-28f1-6bae5ce6b222.1591389/) & Later

- [Unity GameCore v1.2.0](https://forum.unity.com/threads/game-core-package-1-2-0-84c9720d1886.1548836/) & Later

- [Unity GKDKInput v1.0.1](https://forum.unity.com/threads/game-core-package-1-2-0-84c9720d1886.1548836/) & Later

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

**IMPORTANT:** This sample **requires** the *GameCore* and *GXDKInput* packages provided by Unity, which can be found in the [Unity
Forums](https://forum.unity.com/threads/unity-for-game-core-downloads.837508/).
However, these packages are currently under **NDA** status.
You will need to join [ID@Xbox](https://www.xbox.com/en-us/Developers/id) to gain access.

After opening the project, you **must** add the following components via Unity's **Package Manager** to resolve the errors:

- com.unity.gamecore-1.2.0 (or later)

- com.unity.inputsystem.gxdk-1.0.1 (or later)

![Image showing the Game Core and Input packages in Package Manager](./media/packageManager.png)

You will also need to switch to the target Xbox platform in Unity's
**Build Settings** page.
Afterwards, you can use '*Build and Run*' to deploy to the default console that you set via [**Xbox Manager**](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xbom).

- Game Core -- Xbox One

- Game Core -- Xbox Series

![Image showing build settings for Xbox Series](./media/buildSettings.png)

For more information, see **Set up Unity for Xbox development** in Unity's Game Core documentation (Help > Unity Manual GameCoreScarlettSupport).

# Running the Sample

If you have managed to successfully build and deploy the sample, you should see "UnitySimpleMPSD_Xbox.exe" added to your 'Games and apps' list on the console.

The console's sandbox **must** be set to XDKS.1 for the sample to work with the default configuration.
All @xboxtest.com accounts have access to this sandbox.
If you want to run the sample as your own title within your development sandbox, then you can use the '*Store Association*' wizard to alter the Microsoft Game configuration.

You will also need a batch of test users who can login to the sandbox (all @xboxtest.com accounts have access to XDKS.1).

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
Two or more users must be searching for a match in order for this to occur.

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

# Known Issues

The sample was developed and tested against the versions listed at the top of this document.
Integrating a newer version of Unity's GameCore package, or integrating a different version of the native GDK or Unity plugin may expose incompatibilities with APIs that may be deprecated, or have altered behavioral characteristics.

To use the Unity GameCore package, be sure to define "UNITY_GAMECORE" for Xbox One and Xbox Series under Edit > Project Settings > Player > Other Settings.

# Update History

| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of README and sample. Includes build requirements, usage details, notes and issues. |  September 2021  |  1.0 |
| Updated the sample to run on Unity 2022.3.28f1 and the latest (legacy) Unity GameCore packages. Future versions of this sample will use the new Microsoft GDK Packages (com.unity.microsoft.gdk and com.unity.microsoft.gdk.tools) available in Package Manager. | June 2024 | 1.1 |

# Privacy Statement

This sample adheres to general Microsoft privacy guidelines regarding
the distribution of sample source code, documentation, or other
material, for the sole private and individual usage by the prospective
developer of the APIs referenced within.

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
