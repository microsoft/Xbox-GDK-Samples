![ATG Xbox and Windows logos](./media/image1.png)

# Unity Simple PLM

This sample is a port of the [C++ SimplePLM sample](https://github.com/microsoft/Xbox-GDK-Samples/tree/main/Samples/System/SimplePLM) provided by the Xbox ATG team. It is compatible with:

- [Microsoft GDK](https://github.com/microsoft/GDK/releases/tag/June_2024) or [Microsoft GDKX](https://www.microsoft.com/en-us/software-download/gdk) June 2024 (10.0.25398.4271) & Later

- [Unity Editor](https://unity.com/releases/editor/archive) 6000.0.23f1, 2022.3.49f1, 2021.3.45f2 & Later

- [Microsoft GDK API](https://docs.unity3d.com/Packages/com.unity.microsoft.gdk@1.4/manual/index.html) 1.2.3 & Later (available via Unity's Package Manager)

- [Microsoft GDK Tools](https://docs.unity3d.com/Packages/com.unity.microsoft.gdk.tools@1.4/manual/index.html) 1.2.3 & Later (available via Unity's Package Manager)

**Note:** When buidling for console, you must use a version of the Unity Editor that supports your target GDKX version.
See [**Building for console**](#building-for-console) for more information.

*If developing with the legacy (now deprecated) Unity GameCore packages instead of the Microsoft GDK packages listed above, use the October 2024 version of **GDKX Unity Samples** available from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk) (set file type to 'GDK' and build/version to 'Microsoft October 2024 GDKX Unity Samples)*.

#

# Description

The Unity Simple PLM sample shows how to respond to events that are related to PLM.
For more information about game life-cycle states on console, see the [Xbox Game Life Cycle](https://learn.microsoft.com/en-us/gaming/gdk/docs/gdk-dev/console-dev/overviews/xbox-game-life-cycle) documentation.
This sample can be used to understand and test the behavior of PLM events.
Responding to PLM events is necessary on console, but this sample also responds to focus changed event on PC to demonstrate similar behavior.

![Simple PLM sample UI](./media/image3.png)

# Noteable Code Files

**XboxManager.cs**: Contains initialization of the Xbox GDK and Xbox Live Services APIs, along with signing in a user and querying various information, such as Sandbox and Title ID.

**GamePLM.cs**: Contains handlers for focus changed events on PC and resource constrained / suspended / resumed events on console.

**SimplePLMSceneManager.cs**: Controls sample UI flow and calls XGameUiShowPlayerProfileCardAsync, XGameUiShowMessageDialog, and XLaunchUri to demonstate different TCUI options that can cause a game to lose focus (PC) or be constrained (console).

# Building the Sample

For general guidance on using the GDK with Unity, see [Unity GDK integration for PC: get started](https://learn.microsoft.com/en-us/gaming/gdk/docs/gdk-dev/pc-dev/tutorials/get-started-with-unity-pc/gdk-unity-end-to-end-guide).

The following steps are provided to help troubleshoot common issues when building the sample:

1. After opening the project, you may need to add the following components via Unity's **Package Manager** to resolve any errors:

     - Microsoft GDK API (com.unity.microsoft.gdk) - version 1.2.3 (or later).

     - Microsoft GDK Tools (com.unity.microsoft.gdk.tools) - version 1.2.3 (or later).

     - Input System (com.unity.inputsystem) - version 1.7.0 (or later).
     Switching to the new Input System will require an Editor restart.

2. Set the sample's Input Action Asset to be used as the project-wide input actions (Assets\SimplePLM\GdkSimplePLMInputManager.inputactions).

3. Activate the sample's GDK Settings Asset (double-click on Assets\SimplePLM\GdkSimplePLM.gdksettings.asset).

4. Open 'Edit > Project Settings > Microsoft GDK' to confirm 'GdkSimplePLM' is set as the Microsoft Game Config:

    ![Image of GDK Settings for In-Game Store](./media/gdkSettingsAsset.png)

5. Open 'File > Build Settings' to confirm that the sample scene (Assets\SimplePLM\SimplePLM.unity) is included in the 'Scenes In Build' section.

## Building for PC

On the **Build Settings** page, set Platform to 'Windows, Mac, Linux', target platform to 'Windows' and architecture to 'Intel 64-bit'.
Use 'Build' to build the project, or 'Build and Run' to build and deploy to your development PC.

![Image of 'Build Settings' in Unity](./media/buildSettings.png)

## Building for Console

There are additional package requirements when building for console.
The following packages are available from the [Unity Xbox Forums](https://discussions.unity.com/t/unity-for-game-core-downloads/778704).
However, they are under **NDA** status and you will need to join [ID@Xbox](https://www.xbox.com/en-us/Developers/id) to gain access.

- Unity Game Core Series (Scarlett) Add-on (version depends on Unity Editor and GDKX combo)

- Unity Game Core Xbox One Add-on (version depends on Unity Editor and GDKX combo)

- [Microsoft GDK Tools for Xbox](https://discussions.unity.com/t/microsoft-gdk-tools-xbox-1-0-2-is-now-available/1531731) (com.unity.microsoft.gdk.tools.xbox) - version 1.0.2 (or later)

- [GXDK Input System](https://discussions.unity.com/t/gxdk-input-system-1-0-2-bc4cf1f9/1587298) (com.unity.inputsystem.gxdk) - version 1.0.2 (or later)

To build for console, switch Unity's build target on the **Build Settings** page to 'Xbox One' or 'Xbox Series'.
Afterwards, you can use '*Build and Run*' to deploy to the default console that you set via [Xbox Manager](https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/tools-console/xbox-tools-and-apis/xbom/xbom).

# Running the Sample

The sample can perform operations that demonstrate PLM transitions and what events / game states are affected.
The following options will trigger a TCUI to appear, which will result in a PLM event on the device:

| Button                                |  Action                      |
|---------------------------------------|------------------------------|
| Show Player Profile Card | Shows the PlayerProfile card, which causes the game to lose focus on PC or be constrained with partial visibility on console |
| Show Message Dialog | Shows a customizable message dialog that causes the game to lose focus on PC, or become contrained with limited visibility on console |
| Launch Settings | Shows the Settings app, which causes the game to lose focus on PC or become constrained without visibility on console |
| Sign In |  Shows the AccountPicker, which causes the game to lose focus on PC or be constrained with partial visibility (console) |

On console, suspend can be triggered by making the game fully not visible ('Launch Settings' will do this) and then waiting ~10 minutes for the game to be suspended.

You can also test console suspend, constrain and resume operations from within Visual Studio, the Xbox Manager, or via the Xbapp.exe tool installed with the GDK:

- Xbapp.exe suspend SimplePLM_1.2.0.0_x64\_\_zjr0dfhgjwvde

- Xbapp.exe resume SimplePLM_1.2.0.0_x64\_\_zjr0dfhgjwvde

For more details, see [Debugging tools and features](https://learn.microsoft.com/en-us/gaming/gdk/docs/gdk-dev/console-dev/overviews/xbox-game-life-cycle#debugging-tools-and-features-).

# Known Issues

The sample was developed and tested against the packages and versions listed in this document.
Using different versions of the GDK, Microsoft GDK API package, or the Unity Editor may result in build failures and incompatibilities.

# Trademarks

This sample may contain trademarks or logos for projects, products, or services.
Authorized use of Microsoft trademarks or logos is subject to and must follow [Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this sample must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.

# Privacy Statement

This sample adheres to general Microsoft privacy guidelines regarding the distribution of sample source code, documentation, or other material, for the sole private and individual usage by the prospective developer of the APIs referenced within.

For more information about Microsoft's privacy policies in general, see the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update History

| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of sample and README. Includes build requirements, usage details, notes and issues. | September 2022 | 1.0 |
| Added Notable Code section. | May 2023 | 1.0 |
| Updated the sample to run on Unity 2022.3.28f1 and the latest (legacy) GDK Unity Package on GitHub. Future versions of this sample will use the new Microsoft GDK Packages (com.unity.microsoft.gdk and com.unity.microsoft.gdk.tools) available in Package Manager. | June 2024 | 1.1 |
| Sample now targets both PC and Xbox consoles using the new Microsoft GDK API and Tools packages (com.unity.microsoft.gdk, com.microsoft.gdk.tools, com.unity.microsoft.gdk.tools.xbox) available via Unity's Package Manager. | June 2025 | 1.2 |
