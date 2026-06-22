//--------------------------------------------------------------------------------------
// CallStack.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "CallStack.h"

// Capture the backtrace for the current thread
// Note: the caller must allocate enough space in addresses and modules to accomodate the number of frames specified by framesToCapture
size_t CallStack::CaptureBackTraceFromCurrentThread(void** addresses, HMODULE* modules, size_t framesToCapture)
{
    static const uint32_t FRAMES_TO_SKIP = 1;
    // Since this function doesn't exit until it finishes it's safe for it to make a copy of the current stack, the stack won't change
    // With FRAMES_TO_SKIP set to one the actual call to RtlCaptureStackBackTrace won't be in the backtrace
    // This function will have already performed the unwinding of the stack into individual blocks
    size_t numCapturedFrames = RtlCaptureStackBackTrace(FRAMES_TO_SKIP, static_cast<DWORD>(framesToCapture), addresses, nullptr);

    for (uint32_t i = 0; i < numCapturedFrames; ++i)
    {
        uint64_t baseAddress = 0;
        RUNTIME_FUNCTION* runtimeFunction = ::RtlLookupFunctionEntry(reinterpret_cast<uint64_t>(addresses[i]), &baseAddress, nullptr);

        if (runtimeFunction == nullptr)
        {
            // If we don't have a RUNTIME_FUNCTION, then we've encountered a leaf function.  Adjust the stack data appropriately.
            if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(addresses[i]), &modules[i]))
            {
                modules[i] = 0;
            }
        }
        else
        {
            modules[i] = reinterpret_cast<HMODULE>(baseAddress);
        }
    }

    return numCapturedFrames;
}

// Capture the backtrace starting with the provided context record.
// Note: the caller must allocate enough space in addresses and modules to accomodate the number of frames specified by framesToCapture
size_t CallStack::CaptureBackTraceFromContext(CONTEXT* contextRecord, void** addresses, HMODULE* modules, size_t framesToCapture)
{
    if (contextRecord == nullptr)
    {
        return 0;
    }

    CONTEXT ctx = *contextRecord;

    uint64_t baseAddress = 0;

    KNONVOLATILE_CONTEXT_POINTERS NvContext = {};
    void* handlerData = nullptr;
    uint64_t establisherFrame = 0;

    size_t numCapturedFrames = 0;
#ifndef _M_ARM64
    do
    {
        RUNTIME_FUNCTION* runtimeFunction = RtlLookupFunctionEntry(ctx.Rip, &baseAddress, nullptr);

        modules[numCapturedFrames] = (HMODULE)baseAddress;
        addresses[numCapturedFrames] = reinterpret_cast<void*>(ctx.Rip);
        ++numCapturedFrames;

        if (runtimeFunction)
        {
            // Using the current frame data unwind the stack one level to point at the next function in the call stack
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, baseAddress, ctx.Rip, runtimeFunction, &ctx, &handlerData, &establisherFrame, &NvContext);
        }
        else
        {
            // If we don't have a RUNTIME_FUNCTION, then we've encountered a leaf function.  Adjust the stack appropriately.
            ctx.Rip = *reinterpret_cast<uint64_t*>(ctx.Rsp);
            ctx.Rsp += 8;
        }
    } while (numCapturedFrames < framesToCapture && ctx.Rip != 0);
#else
    do
    {
        RUNTIME_FUNCTION* runtimeFunction = RtlLookupFunctionEntry(ctx.Pc, &baseAddress, nullptr);

        modules[numCapturedFrames] = (HMODULE)baseAddress;
        addresses[numCapturedFrames] = reinterpret_cast<void*>(ctx.Pc);
        ++numCapturedFrames;

        if (runtimeFunction)
        {
            // Using the current frame data unwind the stack one level to point at the next function in the call stack
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, baseAddress, ctx.Pc, runtimeFunction, &ctx, &handlerData, &establisherFrame, &NvContext);
        }
        else
        {
            ctx.Pc = *reinterpret_cast<uint64_t*>(ctx.Sp);
            ctx.Sp += 8;
        }
    } while (numCapturedFrames < framesToCapture && ctx.Pc != 0);
#endif

    return numCapturedFrames;
}
