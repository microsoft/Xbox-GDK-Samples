//--------------------------------------------------------------------------------------
// TitleReservedMemory.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <xmem.h>

namespace ATG
{
    // fixed address range that is reserved by the OS ahead of time, titles can directly commit out of it without fear of any other process accessing it
    constexpr uintptr_t c_LOW_FIXED_RESERVED_ADDRESS = 4ULL * 1024 * 1024 * 1024 * 1024;
    constexpr uintptr_t c_HIGH_FIXED_RESERVED_ADDRESS = 8ULL * 1024 * 1024 * 1024 * 1024;

    // Starting in the 2203 GXDK the 4TB-8TB address space that is reserved for the was changed to a PLACE_HOLDER reservation which means a different method is needed to allocate
#if _GXDK_VER < 0x55F00C2F /* GXDK Edition 220300 */
    inline void* AllocateTitleReserved(void* baseAddress, size_t allocationSize, uint32_t allocationType, uint64_t xmemFlags, uint32_t pageProtection)
    {
        return XMemVirtualAlloc(baseAddress, allocationSize, allocationType, xmemFlags, pageProtection);
    }

    inline bool ReleaseTitleReserved(void* baseAddress)
    {
        return VirtualFree(baseAddress, 0, MEM_RELEASE);
    }
#else
    void* AllocateTitleReserved(void* baseAddress, size_t allocationSize, uint32_t allocationType, uint64_t xmemFlags, uint32_t pageProtection);
    bool ReleaseTitleReserved(void* baseAddress);
#endif
}
