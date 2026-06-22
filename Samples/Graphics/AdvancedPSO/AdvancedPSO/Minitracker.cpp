//-----------------------------------------------------------------------------
// Minitracker.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "pch.h"
#include "Minitracker.h"

XMEM_SET_ALLOCATION_HOOKS(MyAlloc, MyFree)

void* __stdcall MyAlloc(_In_ SIZE_T dwSize, _In_ ULONGLONG dwAttributes)
{
    void* alloc = XMemAllocDefault(dwSize, dwAttributes);

    if (DX::Minitracker::s_enabled)
    {
        XALLOC_ATTRIBUTES xAttr;
        xAttr.Attributes = dwAttributes;

        {
            std::lock_guard<std::mutex> lock(DX::Minitracker::s_critSection);
            // this may alloc via CRT
            DX::Minitracker::s_allocs.insert(std::make_pair(alloc, DX::Minitracker::Entry{ dwSize, xAttr, GetCurrentThreadId()}));
        }
    }

    return alloc;
}

// hook XMemAlloc frees
void __stdcall MyFree(_In_ void* lpAddress, _In_ ULONGLONG dwAttributes)
{
    XMemFreeDefault(lpAddress, dwAttributes);

    if (DX::Minitracker::s_enabled)
    {
        std::lock_guard<std::mutex> lock(DX::Minitracker::s_critSection);

        auto it = DX::Minitracker::s_allocs.find(lpAddress);
        if (it != DX::Minitracker::s_allocs.end()) DX::Minitracker::s_allocs.erase(it);
    }
}

namespace DX
{
    bool                                                Minitracker::s_enabled;
    std::unordered_map< void*, Minitracker::Entry >     Minitracker::s_allocs;
    std::mutex                                          Minitracker::s_critSection;
}
