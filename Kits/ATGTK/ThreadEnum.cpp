//--------------------------------------------------------------------------------------
// ThreadEnum.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
// Note: The tool help APIs are not defined in the games partition currently. This is a workaround to force the toolhelp APIs to be available
#undef WINAPI_FAMILY_PARTITION
#define WINAPI_FAMILY_PARTITION(Partitions) 1

#include <TlHelp32.h>

#undef WINAPI_FAMILY_PARTITION
#define WINAPI_FAMILY_PARTITION(Partitions) (Partitions)

#include "ThreadEnum.h"

void ATG::EnumerateThreads(const ATG::EnumThreadsCallbackFunc& callbackFunc)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());
    THREADENTRY32 threadEntry;
    memset(&threadEntry, 0, sizeof(threadEntry));
    threadEntry.dwSize = sizeof(threadEntry);
    if (Thread32First(snapshot, &threadEntry))
    {
        do
        {
            if (threadEntry.th32OwnerProcessID == GetCurrentProcessId())
            {
                if (!callbackFunc(threadEntry.th32OwnerProcessID, threadEntry.th32ThreadID))
                    break;
            }
        } while (Thread32Next(snapshot, &threadEntry));
    }
    CloseHandle(snapshot);
}

//#endif
