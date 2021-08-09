//--------------------------------------------------------------------------------------
// SharedMemory.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SharedMemory.h"

namespace SharedMemory
{
    uint32_t c_sizeSharedMemory = sizeof(SharedMemoryBlock);
    const wchar_t *g_sharedStartEventName = L"OutOfProcStartExceptionEvent";
    const wchar_t *g_sharedFinishedEventName = L"OutOfProcFinishedExceptionEvent";
    const wchar_t *g_sharedMemoryName = L"OutOfProcSharedMemory";
    HANDLE g_sharedFileMappingHandle = nullptr;
    HANDLE g_sharedStartEvent;
    HANDLE g_sharedFinishedEvent;
    SharedMemoryBlock *g_sharedMemory;

    SharedMemoryBlock *InitSharedMemory()
    {
        bool needInitialMemoryClear = false;

        // These Events have names which means if an Event has already been created with the same name then CreateEvent will return a handle to that Event
        // This allows the communication between processes, they are each referencing the same Event object in the kernel
        g_sharedStartEvent = CreateEvent(nullptr, FALSE, FALSE, g_sharedStartEventName);
        g_sharedFinishedEvent = CreateEvent(nullptr, FALSE, FALSE, g_sharedFinishedEventName);

        // A shared memory is created through a file mapping object that is not backed by a file including the page file
        // The main reason no backing file is chosen for this shared memory is because that is required on the console, having a backing file used for read/write access is not allowed

        // First attempt to open the file mapping object by name, this will only succeed if the object has already been created
        // Doing the operations in this order removes the need for synchronization between systems for which system will create the object
        // The OS is already handling the synchronization on the mapping object creation
        g_sharedFileMappingHandle = OpenFileMappingW(
            FILE_MAP_ALL_ACCESS,
            FALSE,
            g_sharedMemoryName);

        // If the attempt to open the object fails then attempt to create the file mapping object
        // Note: There is still technically a race condition with this code where the open fails, another thread creates the object, then the original thread will fail to create
        // For the purposes of this sample extra protection has not been created to handle this race condition, it won't happen in the examples shown in the sample
        if (g_sharedFileMappingHandle == nullptr)
        {
            needInitialMemoryClear = true;
            g_sharedFileMappingHandle = CreateFileMappingW(
                INVALID_HANDLE_VALUE,
                NULL,
                PAGE_READWRITE,
                0,
                c_sizeSharedMemory,
                g_sharedMemoryName);
        }

        if (g_sharedFileMappingHandle == nullptr)
            return nullptr;

        // Once the file mapping object has been created the memory can now be mapped into the process
        g_sharedMemory = (SharedMemoryBlock *)MapViewOfFile(g_sharedFileMappingHandle,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            c_sizeSharedMemory);

        if (!g_sharedMemory)
        {
            CleanupSharedMemory();
            return nullptr;
        }

        if (needInitialMemoryClear)
            memset(g_sharedMemory, 0, c_sizeSharedMemory);

        return g_sharedMemory;
    }

    // Cleaning up the shared memory object is straight forward with ummapping the view and closing the associated handles
    // The objects ref-counted through the handles and they will stay allocated until the last handle is closed.
    void CleanupSharedMemory()
    {
        if (g_sharedMemory != nullptr)
        {
            UnmapViewOfFile(g_sharedMemory);
            g_sharedMemory = nullptr;
        }

        if (g_sharedFileMappingHandle != nullptr)
        {
            CloseHandle(g_sharedFileMappingHandle);
            g_sharedFileMappingHandle = nullptr;
        }
        CloseHandle(g_sharedStartEvent);
        CloseHandle(g_sharedFinishedEvent);
        g_sharedStartEvent = INVALID_HANDLE_VALUE;
        g_sharedFinishedEvent = INVALID_HANDLE_VALUE;
    }
}
