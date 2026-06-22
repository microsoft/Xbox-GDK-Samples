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
    static constexpr uintptr_t c_LOW_FIXED_RESERVED_ADDRESS = 4ULL * 1024 * 1024 * 1024 * 1024;
    static constexpr uintptr_t c_HIGH_FIXED_RESERVED_ADDRESS = 8ULL * 1024 * 1024 * 1024 * 1024;

    // Starting in the 2203 GXDK the 4TB-8TB address space that is reserved for the was changed to a PLACE_HOLDER reservation which means a different method is needed to allocate
#if _GXDK_VER < 0x55F00C2F /* GXDK Edition 220300 */
    inline void *AllocateFixedAddress(void *baseAddress, size_t allocationSize)
    {
        return XMemVirtualAlloc(baseAddress, allocationSize, MEM_COMMIT, XMEM_CPU, PAGE_READWRITE);
    }

    inline bool ReleaseFixedAddress(void *baseAddress, size_t allocationSize)
    {
        return VirtualFree(baseAddress, allocationSize, MEM_RELEASE);
    }
#else
    inline void *AllocateFixedAddress(void *baseAddress, size_t requestedAllocationSize)
    {
        size_t allocationSize = (requestedAllocationSize + 65535) & 65536;

        uintptr_t baseAddressAsUINT(reinterpret_cast<uintptr_t>(baseAddress));
        if (baseAddressAsUINT < c_LOW_FIXED_RESERVED_ADDRESS)
            return nullptr;
        if ((baseAddressAsUINT + allocationSize) > c_HIGH_FIXED_RESERVED_ADDRESS)
            return nullptr;
        XMEM_BASIC_INFORMATION xmeminfo;
        XMemVirtualQuery(GetCurrentProcess(), baseAddress, &xmeminfo);
        if ((xmeminfo.State & MEM_RESERVE) == 0)
            return nullptr;


#pragma warning(suppress: 6333 28160) // MEM_PRESERVE_PLACEHOLDER requires non-zero size with MEM_RELEASE
        if (!VirtualFree(baseAddress, allocationSize, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER))
            return nullptr;
        if (!XMemVirtualAlloc(baseAddress, allocationSize, MEM_REPLACE_PLACEHOLDER | MEM_RESERVE, XMEM_CPU, PAGE_READWRITE))
            return nullptr;
        if (!XMemVirtualAlloc(baseAddress, allocationSize, MEM_COMMIT, XMEM_CPU, PAGE_READWRITE))
            return nullptr;

        return baseAddress;
    }

    inline bool ReleaseFixedAddress(void *baseAddress, size_t allocationSize)
    {
        if (!baseAddress)
            return true;
        XMEM_BASIC_INFORMATION xmemInfo;
        XMemVirtualQuery(GetCurrentProcess(), baseAddress, &xmemInfo);
        if (xmemInfo.AllocationBase != baseAddress)
            XMemVirtualQuery(GetCurrentProcess(), xmemInfo.AllocationBase, &xmemInfo);
        if (allocationSize == 0)
            allocationSize = xmemInfo.RegionSize;

#pragma warning(suppress: 6333 28160) // MEM_PRESERVE_PLACEHOLDER requires non-zero size with MEM_RELEASE
        if (!VirtualFree(baseAddress, allocationSize, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER))
            return false;

        char *newPlaceholderBaseAddress(reinterpret_cast<char *>(xmemInfo.AllocationBase));
        char *newPlaceholderHighAddress((reinterpret_cast<char*>(xmemInfo.AllocationBase)) + allocationSize);

        char *baseAddressLow = newPlaceholderBaseAddress - 1;
        if (baseAddressLow >= reinterpret_cast<char *>(c_LOW_FIXED_RESERVED_ADDRESS))
        {
            XMEM_BASIC_INFORMATION xmemInfoLow;
            XMemVirtualQuery(GetCurrentProcess(), baseAddressLow, &xmemInfoLow);
            if (xmemInfoLow.State & MEM_RESERVE)
                newPlaceholderBaseAddress = reinterpret_cast<char *>(xmemInfoLow.AllocationBase);
        }

        char *baseAddressHigh = newPlaceholderHighAddress + 1;
        if (baseAddressHigh < reinterpret_cast<char *>(c_HIGH_FIXED_RESERVED_ADDRESS))
        {
            XMEM_BASIC_INFORMATION xmemInfoHigh;
            XMemVirtualQuery(GetCurrentProcess(), baseAddressHigh, &xmemInfoHigh);

            if (xmemInfoHigh.State & MEM_RESERVE)
                newPlaceholderHighAddress = ((reinterpret_cast<char *>(xmemInfoHigh.AllocationBase)) + xmemInfoHigh.RegionSize);
        }

        uint64_t newPlaceholderSize(static_cast<uint64_t>(newPlaceholderHighAddress - newPlaceholderBaseAddress));


#pragma warning(suppress: 6333 28160) // MEM_COALESCE_PLACEHOLDERS requires non-zero size with MEM_RELEASE
        if (!VirtualFree(reinterpret_cast<void *>(newPlaceholderBaseAddress), newPlaceholderSize, MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS))
            return false;
        return true;
    }
#endif
}
