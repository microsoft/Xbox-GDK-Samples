  ![](./media/image1.png)

#   EnumerateThread Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

The [ToolHelp](https://docs.microsoft.com/windows/win32/api/tlhelp32/)
APIs provide to enumerate all the threads running within the title
process. While these APIs are available in the OneCore libraries they
are currently not available in the WINAPI_PARTITION_GAMES, this requires
some additional code to make them available to the title code. The
current work around is to redefine WINAPI_FAMILY_PARTITION before
including TlHelp32.h with the following code.

// Note: The tool help APIs are not defined in the games partition
currently. This is a workaround to force the ToolHelp APIs to be
available for the title to call. On Xbox consoles the title will also
need to link against the onecore_apiset.lib library. At that point the
ToolHelp APIs can be used without issue.

> #undef WINAPI_FAMILY_PARTITION
>
> #define WINAPI_FAMILY_PARTITION(Partitions) 1
>
> #include \<TlHelp32.h\>
>
> #undef WINAPI_FAMILY_PARTITION
>
> #define WINAPI_FAMILY_PARTITION(Partitions) (Partitions)

# Using the sample

The sample will create five background threads and then enumerate and
list all the threads running in the process along with their name and
priority.

# Update history

Initial release August 2022

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
