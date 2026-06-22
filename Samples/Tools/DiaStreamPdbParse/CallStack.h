//--------------------------------------------------------------------------------------
// CallStack.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace CallStack
{
    // Capture the backtrace for the current thread
    // Parameters:
    //  addresses -- receives the return addresses from each stack frame in the current call stack
    //  modules  -- receives the module handle corresponding to the return address for each stack frame
    //  framesToCapture -- indicates the number of frames to capture
    // Return value:
    //  Returns the number of frames that were captured
    // Note: the caller must allocate enough space in addresses and modules to accomodate the number of
    //       frames specified by framesToCapture
    size_t CaptureBackTraceFromCurrentThread(void** addresses, HMODULE *modules, size_t framesToCapture);

    // Capture the backtrace starting with the provided context record.
    // Parameters:
    //  contextRecord: address of a context record (register data)
    //  addresses -- receives the return addresses from each stack frame in the current call stack
    //  modules  -- receives the module handle corresponding to the return address for each stack frame
    //  framesToCapture -- indicates the number of frames to capture
    // Return value:
    //  Returns the number of frames that were captured
    // Note: the caller must allocate enough space in addresses and modules to accomodate the number of
    //       frames specified by framesToCapture
    size_t CaptureBackTraceFromContext(CONTEXT *contextRecord, void** addresses, HMODULE* modules, size_t framesToCapture);
} // namespace CallStack
