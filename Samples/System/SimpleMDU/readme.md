  ![](./media/image1.png)

#   SimpleMDU Sample

# Description

This sample demonstrates the basic properties of the Memory
Decompression Unit (MDU).

As described within the *Optimizing Compressed Content* topic within the
GDK documentation, the MDU can be used to decompress data streams as
part of I/O requests through the DirectStorage API, or from an in-memory
source. This sample generates a series of data streams and uses two
DirectStorage queues to continuously decompress from-memory and\\or
from-storage streams. You can change the properties of the data stream
to observe the throughput characteristics of the MDU.

# Building the sample

This sample does not support Xbox One or Desktop. DirectStorage is only
supported in the Gaming.Xbox.Scarlett.x64 configuration.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Upon load, the sample will generate a single data stream which will then
be persisted to disk. That stream is then consumed by both the in-memory
and from-storage queues. You can then generate\\change data streams with
the following controls:

| Dpad L eft\\Right |  Adjust the size of each separately compressed "chunk" within the larger data stream |
|------------|---------------------------------------------------------|
| Dpad Up\\Down |  Adjust the compressibility of the data stream |
| LB\\RB  |  Adjust the memory alignment of the data stream (16 byte or 4KiB)                                                |
| Y          |  Enable\\Disable in-memory decompression                 |
| B          |  Enable\\Disable from-storage decompression              |
| View       |  Exit the sample                                         |

![](./media/image3.jpeg)

# Implementation notes

Within the *Optimizing Compressed Content* topic in the GDK
documentation, MDU decompression options are discussed in detail. The
MDU supports two different compression strategies (zlib & bcpack) that
can be applied separately or in series to a given data stream.

Data streams are typically subdivided into predetermined sizes that can
be offloaded to the MDU decompression hardware, with the upper size
limit being 32MiB for each separately compressed chunk. If using XBTC
for texture compression, the sizes of these individually compressed
streams are determined by the options used at texture import time. If
separately compressing content, then the developer may choose any valid
"chunk" size, though there are performance consideration for using sizes
that are small or large (see documentation for further details).

Depending on the type of content and encoding, and whether lossy or
non-lossy compression is used, typical compression ratios may vary from
1.5:1 up through 4:1. To model this, while keeping the size of the
sample download minimal, the sample generates data across a variety of
compressibility ranges, selectable via controller. Due to the nature of
arbitrary data generation to achieve a range of compression ratios, the
sample only uses the zlib compression strategy removing the need to
generate valid BC encoded texture streams.

Once a stream is selected, the inner loop of the DSQueueHandler simply
attempts to keep the queue topped up with requests. It attempts to keep
three batches of requests in-flight at all times, with each batch
consisting of 32MiB of data requests. This allow easily determining the
MDU throughput characteristics based on the typical data loaded by your
title.

For an example on how to use BCPack compression see the
TextureCompression sample.

The zlib library (version 1.2.11) is subject to this license:
<http://zlib.net/zlib_license.html>

# Update history

Initial release September 2020

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
