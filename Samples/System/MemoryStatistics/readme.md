  ![](./media/image1.png)

#   Memory Statistics Sample

# Description

This sample demonstrates querying memory usage at runtime in your Xbox
game. The primary API used is XMemGetWorkingSetStatistics to grab a
snapshot of resources used by the game. The sample demonstrates grabbing
snapshots of current memory usage, or comparing before and after
snapshots of individual functions to determine memory used by those
components.

# Building the sample

The sample is a Visual Studio 2017 project, but it can also be loaded
and used in more recent versions of the development environment.

If you are using an Xbox One devkit, you should use the
*Gaming.Xbox.XboxOne.x64* configuration.

If you are working with Project Scarlett devkit, you should use the
*Gaming.Xbox.Scarlett.x64* configuration to run the sample in native
mode. Alternatively, you can use the *Gaming.Xbox.XboxOne.x64*
configuration to run the sample in back compat mode.

# Using the sample

This sample displays information about the memory usage in the sample.
It initially displays a single model in 3D space. More models can be
created at runtime to see how they affect resource usage.

![](./media/image3.png)

| Action                      |  Gamepad            |  Keyboard         |
|-----------------------------|--------------------|------------------|
| Increase the number of teapots |  DPad Right  |  Right arrow |
| Decrease the number of teapots |  DPad Left  |  Left arrow |
| Display information about how resource usage at any point during runtime compares to immediately after initialization. |  Y button  |  P |
| Exit                        |  View Button        |  Esc              |

If resource usage displayed by the Y button/P key consistently increases
during gameplay this could indicate a memory leak or inefficient
resource management.

# Implementation notes

This sample shows three ways the **XMemGetWorkingSetStatistics** API can
be used to track memory information.

-   It takes a capture of resource usage at the end of the
    **Initialize** function to compare at any time with resource usage
    at runtime.

-   It takes a capture before and after creating a new teapot to
    determine how much memory is required for any single operation.

-   It also tracks total memory usage to view.

# Update history

-   Initial release -- August 2020

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
