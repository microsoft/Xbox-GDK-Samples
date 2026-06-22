  ![ATG Xbox and Windows logos](./media/image1.png)

# Unity Asset Workflow for Xbox Series | Xbox One

This sample is compatible with:

- [Microsoft GDKX](https://www.microsoft.com/en-us/software-download/gdk) June 2024 & Later

- [Unity 2021.3.45f2](https://discussions.unity.com/t/xbox-unity-2021-3-45f2-88f88f591b2e/1688118) & Later

*If developing with the legacy (now deprecated) Unity GameCore packages instead of the Microsoft GDK packages listed above, use the October 2024 version of **GDKX Unity Samples** available from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk) (set file type to 'GDK' and build/version to 'Microsoft October 2024 GDKX Unity Samples)*.

# Description

This sample demonstrates usage of Xbox console resources and tooling APIs available to the developer which can be utilized to facilitate quick iteration of game assets.
Reference documentation for XTF can be found: [Xbox Tools Framework (API contents)](https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/reference/tools/xtf/atoc-xbox-tools-framework).

The sample uses the app title's TEMP drive partition (limited to 1.5 GB in total size) to store bundles being updated in real-time.
The TEMP drive is denoted by the "T:" drive designation and is specific to the title.
Unlike the app\'s partition ("G:") and the shared scratch partition ("D:"), the TEMP drive is only accessible while the title is active and is cleaned up when the title is terminated.

# Building the Sample

This sample was updated and tested using the June 2024 GDKX, and Unity 2021.3.45f2 with the Xbox build add-ons from Unity.
The sample has minimal dependencies beyond what can be found with the GDK installation itself and Unity's built-in packages.
The sample consists of only one scene named "SampleScene.unity" found within the ***Assets/Scenes*** folder.

Provided that you have necessary support to build a Unity engine player for Scarlett (Series X|S) and/or XboxOne, you should be able to build and run directly from within the Unity editing environment with the existing configuration of the project.

![Unity Build Settings with target set to Scarlett](./media/image3.png)

# Running the sample

If you have managed to successfully build and run the sample from within the Unity editing environment to your Scarlett or XboxOne development
kit, you should see an initial screen that shows a rotating can in the middle of the screen with the familiar Xbox logo on it, like so:

![Sample running on console](./media/image4.png)

The text at the top indicates which version of the assets that the sample player is using to render the Xbox soda can.
At the start, the assets being used are bundles which were built into the player and located in the \<runtime_root\>***/StreamingAssets*** folder.

The sample provides a simple 'tools' window for building and editing assets.
Opening the Xbox Iteration tool window can be done through the Unity editor interface:

![Xbox tools available in the Unity Editor](./media/image5.png)

The Xbox Iteration tools window provides two buttons.
The first is a convenience button which will perform the same function as Unity's built-in "Build and Run" button with the additional pre-build step of
ensuring that any currently running title on the development console is suspended and terminated.

![Xbox Iteration tools window](./media/image6.png)

The scene shown in the previous image can be found at \<assets_root\>***/Scenes/SampleScene.unity***.
Expanding the "SampleScene" root hierarchy node will show the interface elements and some other global objects such as the **AssetManager** and the **SampleMain**, but not the soda can asset itself.
The soda can asset is dynamically loaded at runtime (and placed under the "AssetContainer" game object node) by means of asset bundles using Unity's [Addressables](https://docs.unity3d.com/Manual/com.unity.addressables.html) system.
A detailed explanation of this system is outside the scope of this document, please read Unity's available documentation for further details.
The sample uses an "Iteration" Addressables profile:

![Sample's Addressables Profiles](./media/image7.png)

... that is configured to load bundles from the sample title's "T:" or TEMP drive as described at the start of this document within the "sample-assets" folder.

The assets, which comprise the Xbox logo soda can, are defined together within the **Addressables Groups** to form a dynamically loaded prefab that encapsulates the needed assets such as the 3d cylinder model, the texture, the shader, and the material:

![Sample's Addressables Groups](./media/image8.png)

The sample is configured to have each asset be in its own bundle to allow for maximum flexibility in modifying individual assets without having to copy over excess updated asset data.
Only the affected bundles and asset catalog files are copied over through the available XTF (Xbox Tooling Framework) C# APIs.

*While the sample app is running on the "default" connected development kit*, modify the shader used to render the can (found at <assets_root>***/Assets/basic_shader.shader***) to use a multiply in the fragment shader  rather than an add, like so:

![Fragment shader edits](./media/image9.png)

... and then press the "Build and Deploy Assets (Xtf)" button found on the Xbox Iteration tools pane (Windows > Xbox > Xbox Iteration Tools).
The changed bundles will be rebuilt and copied over into the running title's TEMP drive, and the asset update detection code found within the **AssetManager** will see the change, and notify **SampleMain** to reload the prefab to show the changed assets:

![Running sample with fragment shader edits applied](./media/image10.png)

As we would expect, the can now renders a much darker shade since the shader's "MainColor" property is being multiplied with the "MainTex"
texture pixel color rather than being added.

If we decide to change the color of the texture to the yellow one, another update will be sent to the console and the rendered can will become yellow tinted.

The **XboxManager.exe** tool, found within the ***/Microsoft GDK/bin*** folder of your GDK install, can be used to inspect the files for the running title.
Select the 'Browse Console Files' icon at the top > Running Title > gdk > Drives > Temp, to see the files available in the T drive:

![Using Xbox Manager to inspect files for a running title](./media/image11.png)

... and here you can see the sample-assets\updates folder has the timestamp of the last update, while the prefab_assets_assets\assets folder contains the latest version of the asset bundle on the drive.
The folder presented in the banner for the running sample shows the updated asset catalog path which reflects the changes.
The asset bundle folder structure mirrors that of the \<runtime_root\>***/StreamingAssets*** folder for the player's platform target.

# Implementation notes

This sample has purposely chosen a "local" method of updating assets at runtime for the purposes of demonstrating the drive resources available to a title, as well as making use of some very basic XTF functionality that can be used by developers for making programmatic use of their dev kits.

- The runtime code is under "Scripts" whereas the tool code is under "Editor"

- As mentioned, the simple *AssetManager.cs* does the change detection and loading

- The *SampleMain.cs* object handles the refreshes

- The *XboxIterationTools.cs* code shows the XTF API usage for copying files

**Important** - The versions for packages are locked to the particular implementation of this sample using the versions of the GDK and the Unity Engine mentioned
at the top of this document.

# Known issues

When building for Xbox Series, the build target reported by `UnityEditor.EditorUserBuildSettings.activeBuildTarget` is different than what `Application.platform` returns during runtime.
This can cause streaming assets to fail to load when running the sample.
A workaround is included in AssetManager.cs (lines 101-106), which switches 'GameCoreXboxSeries' to 'GameCoreScarlett' when creating the streamingAssetsBundlePath.

# Trademarks

This sample may contain trademarks or logos for projects, products, or services.
Authorized use of Microsoft trademarks or logos is subject to and must follow [Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this sample must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.

# Privacy Statement

This sample adheres to general Microsoft privacy guidelines regarding the distribution of sample source code, documentation, or other material, for the sole private and individual usage by the prospective developer of the APIs referenced within.

For more information about Microsoft's privacy policies in general, see the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update history

| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of README and sample. Includes build requirements, usage details, notes and issues. Sample targets June QFE 4 2021 GDKX and Unity Editor 2021.2.15f1|  March 24, 2022  |  1.0 |
| Sample updated to target October 2023 Update 4 GDKX and Unity Editor 2022.3.28f1. | June 2024 | 1.1 |
| Sample updated to target June 2024 GDKX and Unity Editor 2021.3.45f2. | October 2025 | 1.2 |
