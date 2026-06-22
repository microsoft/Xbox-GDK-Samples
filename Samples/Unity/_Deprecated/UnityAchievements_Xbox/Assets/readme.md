  ![ATG Xbox and Windows logos](./media/image1.png)

This sample is compatible with:

- [Microsoft GDKX](https://www.microsoft.com/en-us/software-download/gdk) October 2023 Update 4 & Later

- [Unity Editor 2022.3.28f1](https://forum.unity.com/threads/unity-2022-3-28f1-6bae5ce6b222.1591389/) & Later

- [Unity GameCore v1.2.0](https://forum.unity.com/threads/game-core-package-1-2-0-84c9720d1886.1548836/) & Later

- [Unity GXDKInput v1.0.1](https://forum.unity.com/threads/game-core-package-1-2-0-84c9720d1886.1548836/) & Later

*If targeting older versions of the GDK and Unity Editor, use the March 2024 version of **GDKX Unity Samples** available from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk):*
![Image showing download option for older version of Unity Samples](./media/UnityGDKSamplesDownload.png)

#
# Description

The Unity Achievements for Xbox sample demonstrates the usage of Xbox
Live Achievements using the Unity game engine. You can update & retrieve
specific achievements as well as retrieve information on all
achievements available for the title.

![Graphical user interface, application Description automatically generated](./media/image3.png)

# Noteable Code Files

**XboxManager.cs**: Contains initialization of the Xbox GDK & Xbox Live Services APIs,
along with signing in a user & querying various information, such as Sandbox
& Title ID.

**XboxAchievements.cs**: Contains queries & updates for the Xbox
Live achievements APIs.

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

You will need an Xbox Live test account signed in to execute updates and
retrieval commands.

The console's sandbox **must** be set to XDKS.1.

There are two configured achievements available for this sample:

-   '*My First Achievement*'

-   '*A Second Achievement*'

Both achievement sections act in the same manner for updates &
retrievals.

*Update commands:*

-   '*Update Achievement 1: 25%*' -- Sets '*My First Achievement*'
    progression to 25% complete.

-   '*Update Achievement 1: 100%*' -- Sets '*My First Achievement*'
    progression to 100% complete.

-   '*Update Achievement 2: 25%*' -- Sets '*A Second Achievement*'
    progression to 25% complete.

-   '*Update Achievement 2: 100%*' -- Sets '*A Second Achievement*'
    progression to 100% complete.

**IMPORTANT:** Once an achievement has a progression value, the
achievement cannot be updated to a value less than or equal to the
current progression value. In the sample, this will provide a warning
stating that the given achievement has not been modified.

*Retrieval commands:*

-   '*Get Achievement 1'* -- Returns the current state of '*My First
    Achievement*'

-   '*Get Achievement 2'* -- Returns the current state of '*A Second
    Achievement*'

-   '*Get All Achievements'* -- queries Xbox Live Services for all
    achievements associated with the current title ID. The sample's
    console will provide some of the additional information that can be
    retrieved from an achievement.

*Additional Commands:*

-   *'Sign In'* -- opens the Xbox user selection.

-   '*Clear Logs'* -- Clears the console of all existing logs.

-   '*Close' --* Closes the sample.

# Sample Setup in Partner Center

Achievements can be found in '*Gameplay Settings'* underneath the '*Xbox
Services'* section in your title's overview bar. Achievements can be
added by selecting the '*New Achievement'* button.

![](./media/image6.png)

**IMPORTANT:** When using the Xbox Live Service APIs in the sample, it's
important to note that the sample APIs reference the IDs of the
achievements, which can be seen in the 'ID' column of the image above.

Once inside, Achievements can be customized depending on your needs.
Achievements can have:

-   Unique names

-   Custom lock & unlock descriptions

-   Custom icons

-   Launch dependent status

-   Xbox gamer scores

-   Visibility Status (public/secret)

-   External unlock rewards

-   External achievement linking

![Text Description automatically generated](./media/image7.png)

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
| Initial draft of sample and README. Includes build requirements, usage details, notes and issues. | September 2022 | 1.0
| Added Notable Code section. | May 2023 | 1.0
| Updated the sample to run on Unity 2022.3.28f1 and the latest (legacy) Unity GameCore packages. | June 2024 | 1.1