# Simple MultiExecutable Package Sample
_This sample is compatible with the Microsoft Game Development Kit (March 2022)_
![image](SampleImage.png)


### Description
A sample that demonstrates how to set up a solution with multiple executables. This implementation is done by creating multiple projects, and setting them up in a way that allows them to be packaged and run together. 

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If running on PC, set the active solution platform to `Gaming.Xbox.Desktop.x64`.

### Running the sample.

#### Method 1, Run from Visual Studio
- You can run the sample simply by clicking F5. The default experience will be the first to load.

#### Method 2, Run from Package creation
- You can also generate a package for this sample, and run it from there.
- To generate the package.
	1. Build the project.
	2. Run GenConsoleXVCPackage.bat or GenDesktopMSIXVCPackage.bat within a terminal that supports Makepkg.
		1. The console package files can be found in the .\\DefaultExperience\\$Target\\Layout\\Image\\.
		2. The desktop package files can be found in .\\Gaming.Desktop.x64\\Layout\\Image
- Installing and running the package.
	1. If running on Xbox, copy the .xvc file into your devkit through Xbox Manager. You can also install the package with xbapp install.
	2. If running on desktop, install the .MSIXVC file through WDAPP install.


## Update history

**Initial Release:** Microsoft Game Development Kit (June 2023)

June 2023: Initial release

## Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
