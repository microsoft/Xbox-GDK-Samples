  ![](./media/image1.png)

#   Streaming Installation with Intelligent Delivery Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

#  

# Description

This sample demonstrates the streaming installation APIs. Once the
launch chunk is installed, the sample can be run to visualize the
installation progress of the remaining chunks.

It also demonstrates the intelligent installation features including
localized chunks, hardware specific chunks, and custom tags. Different
content will be installed depending on the target SKU of the target
hardware (Durango/Xbox One X for the Xbox One Family, Lockhart/Anaconda
for the Project Scarlett Family) and the language/locale settings of the
console.

# Building the sample

If using an Xbox One devkit, set the active solution platform to
Gaming.Xbox.XboxOne.x64.

If using Project Scarlett, set the active solution platform to
Gaming.Xbox.Scarlett.x64.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Unlike most GDK samples, the StreamingInstall sample is not meant to be
built and deployed from Visual Studio. Although you can deploy and run
directly from Visual Studio this circumvents the installation tools and
process this sample is meant to demonstrate. The steps to use this
sample are as follows:

1)  Build the sample from Visual Studio **without** deploying and
    executing it

2)  Generate content for the installation package

3)  Create the installation package

4)  Install the package and run the sample

With the new Project Scarlett console generation, a separate XVC package
is created based on the target device family. As a result, the different
.bat and .config files here have both an XboxOne version and a Scarlett
version depending on the device family you are testing. Regardless of
the device family, the XPackage code remains unchanged with GDK. In the
MicrosoftGame.config file, the attribute
"TargetDeviceFamily=\"\[XboxOne/Scarlett\]\"" is specified based on the
target family.

This section details each step to provide more information about the
tools and process. The first step is simply to build the sample from
Visual Studio. This will create the binaries and directory layout that
will eventually be used in creating the package. Once the sample has
been built, run **CreateInstallPackage\_\[XboxOne/Scarlett\].bat** from
the command line. The batch file will create several large files to act
as content files to monitor installation progress in real time and stage
the proper MicrosoftGame.config with the target device family set.

Once the binaries and the content are in the build output directory the
next step is to create the installation package using the **MakePkg**
tool installed with the XDK. Open an **Xbox VS 2017 Gaming Command
Prompt** and navigate to the directory that contains
**StreamingInstall.vcxproj**. You can run
**CreateXVC\_\[XboxOne/Scarlett\].bat** from the command prompt you
opened or run the command directly:

XboxOne Device Family:

> *makepkg.exe pack /v /f Chunks_XboxOne.xml /d
> \".\\Gaming.Xbox.x64\\Layout\\Image\\Loose\" /pd
> \".\\Gaming.Xbox.x64\\Layout\\Image\"*

Scarlett Device Family:

> *makepkg.exe pack /v /f Chunks_Scarlett.xml /d
> \".\\Gaming.Scarlett.x64\\Layout\\Image\\Loose\" /pd
> \".\\Gaming.Scarlett.x64\\Layout\\Image\"*

The options used in this command are as follows:

| Opt ion |  Details |
|-----|----------------------------------------------------------------|
| /v  |  Specifies verbose output for error reporting.                  |
| /f  |  Specifies the layout file to be used for creating the installation package. Details about the layout file included in this sample, **Chunks\_\[XboxOne/Scarlett\].xml**, can befound below in this section.                                   |
| /d  | Specifies the source directory for the content of the package. |
| /pd  | Specifies the output directory for the installation packagefile.                                                          |

Since the package has been filled with several gigabytes of filler
content it can take up to several minutes to execute. Once it has
finished executing, several files will be created in the location
specified after the **/pd** option, the
**".\\Gaming.\[Xbox/Scarlett\].x64\\Layout\\Image"** directory in this
case. The actual installation package file will be given a name that is
the **Package Family Name** of the package and not file extension.

The final step is to install the package onto your development console.
There are a number of options to determine what content you want to
install depending on what scenarios you would like to test. If you want
to test your streaming installation implementation, then the following
command can be used:

> xbapp install \[/l\] \[/w\] \<package name\>

| Option         |  Details                                             |
|----------------|-----------------------------------------------------|
| \<package name\> |  The installation package created from the MakePkg command.                                            |
| /l  |  Specifies that only the launch chunk should be installed. Once the launch chunk is installed the game can be run, but only the content enumerated in the chunk marked "Launch" in the layout file will be available.                                       |
| /w  |  Specifies that the install will keep the pipe open. This allows for testing dynamic installation of OnDemand and extra Language chunks which are not initially installed.                                |
| \<full package name\>  |  The name of the game after it has been installed on the console. This should not be confused with the package family name. You can get a list of installed packages on your console by running the **xbapp list** command from the command prompt.     |

Using /l and /w, you can test different scenarios of installing the
game. Using just /l, the system will only install the initial chunk.
Using /l and /w, the initial chunk will be installed by the system and
the rest will be installed after launching the application. If you don't
specify either of those flags, then the entire base package is installed
minus OnDemand and non-default language chunks.

Some chunks are installed based on the user's system and language
automatically. For example, if you deploy to a Durango console with the
XboxOne family package with the language set to "en-US", then chunks
tagged with the language "fr" or device "Xbox-Scorpio" will not be
installed. With the Scarlett family package, either the Lockhart or the
Anaconda content will be installed based on the target device (or debug
setting on a devkit).

You can force installation of other chunks using **xbapp install** with
some of the optional arguments that can be passed to the command.

| Option   |  Details                                                   |
|----------|-----------------------------------------------------------|
| /L anguages  |  Specifies which languages will be installed in a semi-colon delineated list of languages. This can be used to test localized content for multiple languages without the need to rerun the installation command after changing the console settings.                                     |
| /Devices  |  Specifies which hardware specific chunks should be installed. This is a semi-colon delineated list of the different hardware SKUs to include.                       |
| /A llChunks |  Specifies that all chunks should be installed regardless of console hardware or settings.                          |

This is not an exhaustive list of the options that can be used with the
**xbapp install** command. Refer to the GDK documentation for a
comprehensive list of options.

This sample uses the following controls:

| Action                             |  Gamepad Control                |    |
|------------------------------------|--------------------------------|---|
| Navigating the grid menu           |  D-Pad                          |    |
| Selecting an item from the grid menu. Attempt to install a missing chunk if available. Makes the current grid item install first if multiple are pending. |  A  |  |
| Closing an open item and returning to the grid menu |  B  |  |
| Uninstall selected optional chunk if possible |  X  |  |
| Exiting the sample                 |  View                           |    |

# Update history

April 2019, first release of the sample.

March 2020, update sample to separate separate configurations for
XboxOne and Scarlett device families.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# 
