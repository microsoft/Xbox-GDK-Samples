//--------------------------------------------------------------------------------------
// SharedMemory.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "pch.h"
#include "SharedMemory.h"

// The shared memory block is used for communication between the title process and the secondary process that create crash dumps
// It is a single block of memory that is mapped into both the title process as well as the secondary process
namespace SharedMemory
{
    constexpr uint32_t c_maxBaseDumpName = MAX_PATH;
    constexpr uint32_t c_maxApplicationName = MAX_PATH;
    struct SharedMemoryBlock
    {
        bool shutdown;
        DWORD threadId;
        uint32_t miniDumpType;
        EXCEPTION_POINTERS *pointers;
        wchar_t baseDumpName[c_maxBaseDumpName];
        wchar_t applicationName[c_maxApplicationName];

        void SetBaseDumpName(const std::wstring& name) { wcscpy_s(baseDumpName, name.c_str()); }
        void SetApplicationName(const std::wstring& name) { wcscpy_s(applicationName, name.c_str()); }
    };
    extern HANDLE g_sharedStartEvent;
    extern HANDLE g_sharedFinishedEvent;
    extern SharedMemoryBlock *g_sharedMemory;

    void CleanupSharedMemory();
    SharedMemory::SharedMemoryBlock *InitSharedMemory();
}
