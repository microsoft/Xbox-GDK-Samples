  ![ATG Xbox and Windows logos](./media/image1.png)

#   Unity SimplePLM Sample for Xbox

This sample is compatible with:

- [Microsoft GDKX](https://www.microsoft.com/en-us/software-download/gdk) October 2023 Update 4 & Later

- [Unity Editor 2022.3.28f1](https://forum.unity.com/threads/unity-2022-3-28f1-6bae5ce6b222.1591389/) & Later

- [Unity GameCore v1.2.0](https://forum.unity.com/threads/game-core-package-1-2-0-84c9720d1886.1548836/) & Later

- [Unity GXDKInput v1.0.1](https://forum.unity.com/threads/game-core-package-1-2-0-84c9720d1886.1548836/) & Later

*If targeting older versions of the GDK and Unity Editor, use the March 2024 version of **GDKX Unity Samples** available from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk):*
![Image showing download option for older version of Unity Samples](./media/UnityGDKSamplesDownload.png)

#

# Description

This sample shows the behavior of the PLM (Process lifetime management) events and the events that are
related to PLM. The sample will print to the screen and to debug output
a timestamp, function name, and any additional data relevant
to that function for events related to PLM. This sample can be used to
understand the behavior of PLM events.

The sample can also perform operations that cause PLM-related
transitions to demonstrate what events and states are affected. These
include launching into a fullscreen SystemOS experience (Settings) and
Showing the AccountPicker TCUI.

The code in `Assets/Sample/Scripts/Xbox/XboxPLM.cs` manages the sample's PLM logic.

![PLM sample screenshot](./media/sample_ss.png)

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

# Using the sample

The user can cause an application to suspend by making the app not visible (launching settings will do this), the app will suspend after 10 minutes. To resume a suspended application, the user simply needs to make the application visible again. Alternately, the user can use the Xbox One Manager to perform Suspend and Resume operations.

Another option is to use the Xbapp.exe tool that is installed with the
XDK Tools which will allow you to suspend and resume an app with the
following commands:

Xbapp.exe suspend 41336MicrosoftATG.SimplePLM_1.1.0.0_neutral\_\_dspnxghe87tn0

Xbapp.exe resume 41336MicrosoftATG.SimplePLM_1.1.0.0_neutral\_\_dspnxghe87tn0

# Known Issues

The sample was developed & tested against the packages & versions in
this document. It is highly recommended to use the most recent releases
for the GameCore plugin & GXDK Input package released by Unity.

# Privacy Statement

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update History
| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of sample and README. Includes build requirements, usage details, notes and issues. | May 2023 | 1.0
| Updated the sample to run on Unity 2022.3.28f1 and the latest (legacy) Unity GameCore packages. | June 2024 | 1.1