  ![ATG Xbox and Windows logos](./media/image1.png)

#   Unity Achievements for Desktop

This sample is compatible with:

- [Microsoft GDK](https://github.com/microsoft/GDK/releases) or [GDKX](https://www.microsoft.com/en-us/software-download/gdk) - October 2023 Update 4 & Later

- [Unity Editor](https://unity.com/releases/editor/archive) - 2022.3.28f1 & Later

- [GDK Unity
    Package](https://github.com/microsoft/gdk-unity-package/releases) - March 2024 Update 1 & Later

*If targeting older versions of the GDK and Unity Editor, use the March 2024 version of **GDKX Unity Samples** available from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk):*

![Image of older version of Unity Samples download option](./media/UnityGDKSamplesDownload.png)

#

# Description

The Unity Achievements for Desktop sample demonstrates the usage of Xbox
Live Achievements using the Unity game engine for desktop applications.
You can update & retrieve specific achievements as well as retrieve
information on all achievements available for the title.

![Graphical user interface, application Description automatically generated](./media/image3.png)

# Noteable Code Files

**XboxManager.cs**: Contains initialization of the Xbox GDK & Xbox Live Services APIs,
along with signing in a user & querying various information, such as Sandbox
& Title ID.

**XboxAchievements.cs**: Contains queries & updates for the Xbox
Live achievements APIs.

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
this document. Using any newer versions of [GDK Unity
Package](https://github.com/microsoft/gdk-unity-package) or the Unity
Editor may result in build failures & incompatibilities.

# Privacy Statement

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update History
| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of sample and README. Includes build requirements, usage details, notes and issues. | September 2022 | 1.0
| Added Notable Code section. | May 2023 | 1.0
| Updated the sample to run on Unity 2022.3.28f1 and the latest (legacy) GDK Unity Package on GitHub. Future versions of this sample will use the new Microsoft GDK Packages (com.unity.microsoft.gdk and com.unity.microsoft.gdk.tools) available in Package Manager. | June 2024 | 1.1