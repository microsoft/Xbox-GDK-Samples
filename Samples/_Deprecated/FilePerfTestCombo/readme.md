# FilePerfTestCombo Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This sample works with the [Maximizing File Performance on Xbox
One](https://developer.microsoft.com/games/xbox/docs/gdk/maximizing_file_performance_win32_xbox_one)
and [Maximizing Performance on Project
Scarlett](https://developer.microsoft.com/games/xbox/docs/gdk/maximizing_file_performance_win32_scarlett)
pages in the documentation. All the numbers shown in the documentation
were generated from this sample. The sample allows you to examine the
benchmark methodology as well as make changes to model other
configurations.

# Using the sample

All command line options can be found in FilePerfTestCombo.cpp and the
Sample::ParseCommandLine function.

The first step is to generate the data files needed for the test
configurations.

1)  Make sure you have at least 30 GiB free on the drive for the data
    files.

2)  Select the Release configuration for the x64
    platform.

3)  Set the command line.

```
performfullsetup createpackedfile numiterations 5
```


4)  Build and Run.

    a.  Depending on your hardware this may take several minutes.

If desired data files can be used for the decompression test
configuration using zlib. Change the command line to

```
performzipsetup ratio \[compression ratio\] createpackedfile numiterations
```

The \[compression ratio\] can be any number between 0 and 100. The
default is 50. This represents a compression ratio of 50%, the
compressed files will be 50% of the size of the original files.

> Note: Depending on your hardware this could take more than an hour and 200+GiB of disk space.

The various test configurations are mostly controlled through the
command line, the options are not case sensitive.

-   Test type to run -- These can all be used on the same command line
    to specific multiple test configurations.

    -   DoSync

        -   Use Win32 and synchronous operations.

    -   DoASync

        -   Use Win32 and Overlapped operations.

    -   DoSyncDStorage

        -   Use DirectStorage in a synchronous manner.

    -   DoASyncDStorage

        -   Use DirectStorage in an asynchronous manner.

    -   DoZip

        -   Use DirectStorage and the decompression hardware.

        -   Requires the use of the decompression data.

-   NumIterations \[count\]

    -   How many iterations to perform for each test configuration.

-   Load \[first order\] \[last order\]

    -   First and last order read set to use for test configurations.

    -   Valid options in order.

        -   True_Sequential

        -   Random

        -   Random_Sequential

        -   Backwards

        -   Redundant

-   Size \[first size\] \[last size\]

    -   First and last read size to use for test configurations.

    -   Valid options in order.

        -   size_8k

        -   size_12k

        -   size_16k

        -   size_32k

        -   size_64k

        -   size_128k

        -   size_192k

        -   size_256k

        -   size_512k

        -   size_1024k

        -   size_2048k

        -   size_4096k

        -   size_8192k

        -   size_16384k

        -   size_32768k

-   Depth \[first depth\] \[last depth\]

    -   First and last queue depth to use for Win32 asynchronous test
        configurations.

    -   Valid options in order.

        -   depth_1

        -   depth_2

        -   depth_4

        -   depth_8

        -   depth_12

        -   depth_16

        -   depth_24

        -   depth_32

        -   depth_64

        -   depth_128

        -   depth_256

        -   depth_512

        -   depth_768

        -   depth_1024

        -   depth_2048

        -   depth_4096

-   UsePackedFile

    -   Whether to use a single file or multiple smaller files.

-   DoRealtime

    -   Modifier for the DirectStorage tests to use real-time priority
        queues.

Some example command lines

```
DoASync NumIterations 5 load True_Sequential Random Depth Depth_4 Depth_64 Size Size_8k Size_32768k
```

Perform a Win32 asynchronous test with all the combinations for
true sequential and random locations, using a queue depth
between 4 and 64, and a read size between 8KiB and 32MiB.

```
DoASyncDStorage NumIterations 5 load True_Sequential Random Size Size_8k Size_32768k
```

Perform a DirectStorage asynchronous test with all the
combinations for true sequential and random locations, and a
read size between 8KiB and 32MiB.

```
DoASync DoASyncDStorage NumIterations 5 load True_Sequential Random Depth Depth_4 Depth_64 Size Size_8k Size_32768k
```

Perform a Win32 and a DirectStorage asynchronous test with all
the combinations for true sequential and random locations, using
a queue depth between 4 and 64, and a read size between 8KiB and
32MiB.

> Note: the queue depth is only applied to the Win32 tests since that is the only test type that uses that option.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using a Scarlett devkit, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If using Desktop PC, set the active solution platform to `x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Update history

Initial release November 2020

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
