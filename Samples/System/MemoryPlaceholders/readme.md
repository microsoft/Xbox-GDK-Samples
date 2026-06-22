  ![](./media/image1.png)

#   Memory Placeholders Sample

*This sample is compatible with the April 2021 GXDK version. There are
specific changes in the March 2022 GXDK version accounted for as well.*

# 

# Description

This sample demonstrates how to use memory place holders, which are a
way to reserve virtual address space ahead of time. There are two main
benefits to reserving a virtual address range this way:

-   The primary benefit is that it allows you to change the cache
    behavior flags and the page size for memory committed within the
    range.

-   The secondary benefit is that it can allow for faster memory
    allocation because the base virtual address range has already been
    located.

One important note though is that if the range is to be used for
graphics allocations it needs to be created with the XMEM_GRAPHICS flag
to guarantee it's created in an address range the GPU can handle.

The sample creates a place holder that can handle graphics allocations
shortly after the D3D device has been created.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Press the corresponding button on the controller for each demonstration.
It will report success or failure.

# Implementation notes

**Place Holder Allocations**

An implementation for an allocator that handles a place holder region is
included, you are free to use this in your titles. Note: The
recommendation if this implementation is directly used in your title is
to create additional error detection to match your needs.

Main functions for PlaceholderRegion class and their behaviors:

-   SetupRegion

    -   Reserves the initial placeholder region using XMemVirtualAlloc
        and the MEM_RESERVE_PLACEHOLDER flag.

-   FindOpenSpace

    -   A list of the used and free regions is kept. This function walks
        the list looking for an open spot to satisfy an allocation.

-   Allocate

    -   Allocates at the requested location in the place holder region
        or finds an open space that can satisfy the page size and
        allocation size.

    -   Splits the place holder to remove the newly allocated block out
        of the place holder using the MEM_REPLACE_PLACEHOLDER flag.

    -   Commits the memory if requested. The call can later commit the
        memory using the existing methods.

-   Release

    -   Decommits the memory using the MEM_PRESERVE_PLACEHOLDER flag

    -   Determines if there are adjacent regions within the place holder
        that are free. If so merges them using the
        MEM_COALESCE_PLACEHOLDERS flag.

**Title Reserved Memory**

The memory system in the GXDK reserved the 4TB-8TB virtual address
ranges for title allocations. The XMemVirtualAlloc, VirtualAlloc,
XMemAlloc, etc. functions would not allocate anything in this range
unless explicitly requested by the title. The benefit of this reserved
address range is that a title can use the same fixed memory addresses
from one run to the next run, they could even be stored on disk for
later reuse. Before the March 2022 GXDK this region was only guaranteed
to be available right at title startup, there was a chance that later
allocations could occur in this range.

Starting in the March 2022 GXDK this region is allocated as a place
holder reservation. This guarantees that the range will be available to
the title throughout its lifetime. Allocations will only happen in this
range when explicitly requested by the title.

The sample handles the difference between the March 2022 GXDK and
previous GXDK releases. For GXDK releases prior to the March 2022 GXDK
release the allocation/release functionality is routed directly to
XMemVirtualAlloc/VirtualFree.

In the March 2022 GXDK the title reserved virtual address range is
allocated as a place holder, so the sample uses the PlaceHolderRegion
class supplied in the sample to manage the region.

# Update history

Initial release March 2022

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
