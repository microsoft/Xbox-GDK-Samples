  ![](./media/image1.png)

#   Memory Banks Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

# 

# Description

This sample demonstrates several different methods available for
managing memory banks.

-   Random -- Baseline memory bank allocated through VirtualAlloc.

-   Fixed -- Demonstrates how to allocate memory at a specific memory
    address.

-   Read-Only -- Demonstrates how to convert memory to read-only.

-   Bank Switching -- Demonstrates how to create a chain of memory banks
    that can be rotated.

-   Shared -- Demonstrates how to create multiple memory banks that
    share the same physical location.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Press the corresponding button on the controller for each demonstration.
It will report success or failure.

# Implementation notes

## Random

This demonstration serves as the baseline for the other demonstrations.
It allocates a bank of memory using VirtualAlloc and then reads a binary
tree from disk into the memory bank. The data from the disk needs to be
fixed up. Internally the binary tree stores indexes into the memory bank
its allocated within. All the internal pointers for the binary tree need
to be updated based on the index since the base address changes between
runs of the program.

## Fixed

This demonstration works the same way as the random address
demonstration. The main difference is that the base address is stored in
the data file. This allows the program to recreate the same bank at the
same memory location for each run of the program. This means the
internal indexes are not needed and there is no need for pointer fixup.
The data can be loaded from disk much faster with almost zero CPU
overhead.

## Bank Switching

This demonstration shows how to create multiple memory banks that can
have their backing physical blocks swapped. This swaps the virtual
address for two blocks of physical memory. More than two memory banks
could be used and then rotated between. Several uses for this are in the
creation of logging data, frame data used to save replays, and other
places memory is moved between buffers. This allows the removal of the
memory move operation and results in dramatically improved performance.

## Shared

This demonstration shows how to create multiple memory banks that all
point to the same physical block. If two virtual banks are created
adjacent to each other this allows a ring buffer that doesn't require
multiple memory copy operations for boundary copies. As data is written
across the end of the first virtual bank it will automatically wrap to
the start of the physical block.

Another use is to create multiple virtual pointers with different
permissions, such as read-write and read-only. However mixing
permissions that adjust cache usage is not allowed. For example,
write-combine and cacheable.

## Read-Only

This demonstration shows how to create pages that are both read-write
and read-only pages. It does this using the shared demonstration where
one bank is marked read-write and another bank is marked read-only. A
prime use for this pattern is to track down random memory corruption.
For example, static data used by a title. The file loading system could
use the read-write address for creating the data. The rest of the title
uses the read-only pointer for access. If there is a memory corruption
issue it will cause an exception at the exact location in source causing
the problem.

# Update history

Initial release April 2019

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
