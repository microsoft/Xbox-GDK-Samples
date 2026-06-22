  ![](./media/image1.png)

#   GpuBench Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

GpuBench is ATG's internally developed GPU benchmarking tool. The sample
runs modular tests designed to achieve peak GPU rates, or to illuminate
GPU penalties. GpuBench runs on all hardware in the Xbox One family of
devices.

# Building the sample

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

## Screenshot

![](./media/image3.png)

![](./media/image4.png)

| Action                                 |  Gamepad                     |
|----------------------------------------|-----------------------------|
| Run selected benchmark                 |  A button                    |
| Run all benchmarks                     |  X button                    |
| Navigate list                          |  Left joystick               |
| Navigate output                        |  Right joystick              |
| Show/hide help                         |  Menu button                 |
| Exit                                   |  View Button                 |

The sample also supports these command line arguments (for unattended
operation):

-   "all" -- run all benchmarks

-   Lists such as "buffer compute texture" -- run the specified
    benchmarks

ATG encourages users to add their own benchmarks to the sample. To add a
new benchmark, simply add a new .cpp file to the Visual Studio project.
The new file should contain classes which derive from Benchmark and
Test, and which implement the required virtual methods. Use the existing
benchmarks as a model. Add any required HLSL files to the project as
well.

# Implementation notes

Each benchmark consists of a series of tests. The sample runs each test
run multiple times, in order to measure a duration using GPU timestamps,
and to retrieve some number of GPU hardware counter values. The sample
takes care of properly isolating each test run, with a full
synchronization pause and cache flush.

Benchmarks produce the following output:

-   A text table containing a configurable set of columns, a header row,
    and one results row per test. The table is printed:

    -   On screen

    -   To the standard debugger output

    -   To a text file in the Title Scratch drive called GpuBench.txt
        (e.g xbdir /x/title xd:\\GpuBench.txt)

-   A PIX GPU capture in the Title Persistent Storage drive, named
    according to the benchmark name, containing one run of each test
    (e.g. xbdir /x/title xr:\\Compute.pix3)

The benchmarks use GPU counters to validate that they have actually
exercised the GPU according to their intent. For example, the compute
benchmark uses the SQ_PERF_SEL_INSTS_VALU counter to verify how many
hardware arithmetic operations were performed and check that number
against expectations. The sample adopts the convention of naming columns
with the strings "API" and "GPU" (as in "Bytes API" vs "Bytes GPU") to
designate the expected vs actual values of various quantities.

GpuBench provides wrappers for GPU hardware counter retrieval in the
GpuCounterSet class, and supporting classes. These classes may also be
useful as standalone example uses of the underlying D3D11.X GPU counter
APIs. GPU counters can also be retrieved in PIX, but iteration is faster
with runtime retrieval. Also, the sample supports certain features not
implemented in PIX:

-   Restrict counters by shader stage (e.g. count texture fetches only
    in the pixel shader not the vertex shader)

-   Aggregate counters in non-default ways (e.g. find the standard
    deviation rather than the sum across hardware instances)

Titles can retrieve GPU counters only in development builds and should
not rely on them in shipping builds.

# Known issues

Some benchmarks require GPU counters or hardware features which are only
available in Scorpio-class hardware (and beyond). These benchmarks run
on earlier hardware, but provide more information on the later hardware.

# Update history

Initial release April 2019

February 2020: Updated with Gaming.Xbox.Scarlett.x64 support

June 2021: Added async compute launch rate benchmark
