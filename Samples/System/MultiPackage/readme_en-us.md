# Multi Package Sample

_This sample is compatible with the Microsoft Game Development Kit (March 2023)_

![image](media/SampleImage.png)

  
  

### Description

This sample demonstrates how to manage multiple packages with XlaunchURI and custom protocols. The projects MainPackageExperience and AlternatePackageExperience interact with eachother through the XLaunchURI API.

### Project setup
The Microsoft Game config files (.mgc) of a specific project/package contain the custom protocol definitions used to launch that package.

This sample has two projects. The projects must be installed and run as packages in order for this sample to work properly. This is done by:
1. Building the projects.
2. Creating and installing the package for each project through the makepkg.

### Installing and running the packages.

There are 2 packages, so you want to repeat the following steps for each package. The commands below should be run in the ```MainPackageExperience``` and ```AlternatePacakgeExperience``` directories.   

1. You first want to generate the mapping file for the package. This can be done by running makepkg genmap in your build dir.   
	```makepkg genmap /f genChunk.xml /d Gaming.Xbox.Scarlett.x64\Debug```

2. Then generate the package.   
	```makepkg pack /f genChunk.xml /d Gaming.Xbox.Scarlett.x64\Debug /pd <PACKAGE OUTPUT DIRECTORY>```
	
3. Install the .xvc package file that was placed in your \<PACKAGE OUTPUT DIRECTORY\>. Depending on the package, it will either be    
    ```xbapp install 41336MicrosoftATG.MultiPackageMainExperience_1.0.0.0_neutral__dspnxghe87tn0_xs.xvc```   
	or   
	```xbapp install 41336MicrosoftATG.MultiPackageAlternateExperience_1.0.0.0_neutral__dspnxghe87tn0_xs.xvc```

The above commands can be generalized to other platforms and configurations.

>If building packages for desktop, install the .MSIXVC package file with WDAPP install. The install command will look similar to:   
	```wdapp install 41336MicrosoftATG.MultiPackageMainExperience_1.0.0.0_x64__dspnxghe87tn0.msixvc```


## Update history

**Initial Release:** Microsoft Game Development Kit (June 2023)

August 2023: Initial release

## Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
