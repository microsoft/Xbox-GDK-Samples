//--------------------------------------------------------------------------------------
// TitleReservedMemory.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include "pch.h"
#include "TitleReservedMemory.h"
#include "PlaceholderAllocations.h"

    // Starting in the 2203 GXDK the 4TB-8TB address space that is reserved for the was changed to a PLACE_HOLDER reservation which means a different method is needed to allocate
#if _GXDK_VER >= 0x55F00C2F /* GXDK Edition April 2022, 220300 */
namespace
{
    ATG::PlaceholderRegion g_titleReservedPlaceholder(false, reinterpret_cast<void *> (ATG::c_LOW_FIXED_RESERVED_ADDRESS), ATG::c_HIGH_FIXED_RESERVED_ADDRESS - ATG::c_LOW_FIXED_RESERVED_ADDRESS);
}

void *ATG::AllocateTitleReserved(void *baseAddress, size_t allocationSize, uint32_t allocationType, uint64_t xmemFlags, uint32_t pageProtection)
{
    return g_titleReservedPlaceholder.Allocate(baseAddress, allocationSize, allocationType, xmemFlags, pageProtection);
}

bool ATG::ReleaseTitleReserved(void *baseAddress)
{
    return g_titleReservedPlaceholder.Release(baseAddress);
}
#endif
