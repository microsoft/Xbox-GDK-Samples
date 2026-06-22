//--------------------------------------------------------------------------------------
// PlaceholderAllocations.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include "pch.h"
#include "PlaceholderAllocations.h"

using namespace ATG;

#ifdef _DEBUG
void PlaceholderRegion::ValidateAllocationBlock(const AllocationBlock& block) const
{
    XMEM_BASIC_INFORMATION xmemInfo;
    XMemVirtualQuery(GetCurrentProcess(), block.baseAddress, &xmemInfo);
    if (xmemInfo.AllocationBase != block.baseAddress)
        DebugBreak();
    if (xmemInfo.RegionSize != block.blockSize)
        DebugBreak();
    if (block.freeBlock)
    {
        if ((xmemInfo.State & MEM_RESERVE_PLACEHOLDER) == 0)
            DebugBreak();
    }
    else
    {
        if ((xmemInfo.State & MEM_RESERVE_PLACEHOLDER) != 0)
            DebugBreak();
        if ((xmemInfo.State & (MEM_RESERVE + MEM_COMMIT)) == 0)
            DebugBreak();
    }
}

void PlaceholderRegion::ValidateAllocationList(bool fullValidate) const
{
    if (m_allocations.size() == 0)
        DebugBreak();
    if (m_allocations[0].blockStart() != m_regionAddress)
        DebugBreak();
    auto iter = m_allocations.begin();
    char *lastBlockEnd = iter->blockEnd();
    bool lastFreeValue = iter->freeBlock;
    char *regionEnd = m_regionAddress + m_regionSize;
    if (fullValidate)
        ValidateAllocationBlock(*iter);
    iter++;
    for (; iter != m_allocations.end(); ++iter)
    {
        if (fullValidate)
            ValidateAllocationBlock(*iter);

        if (iter->blockStart() != lastBlockEnd)
            DebugBreak();
        if (iter->blockEnd() > regionEnd)
            DebugBreak();
        if (iter->freeBlock && (iter->freeBlock == lastFreeValue))
            DebugBreak();
        lastBlockEnd = iter->blockEnd();
        lastFreeValue = iter->freeBlock;
    }
    if (lastBlockEnd != regionEnd)
        DebugBreak();
}
#endif

// Attempts to correct the place holder region after a failure to allocate a block within the region
void PlaceholderRegion::ForceRelease(void *baseAddress)
{
    ValidateAllocationList(false);
    assert(baseAddress);

    char *newBaseAddress(reinterpret_cast<char *>(baseAddress));
    assert(newBaseAddress >= m_regionAddress);

    // query for the details on the memory region for the supplied baseAddress
    XMEM_BASIC_INFORMATION xmemInfoBase;
    XMemVirtualQuery(GetCurrentProcess(), baseAddress, &xmemInfoBase);
    newBaseAddress = reinterpret_cast<char *>(xmemInfoBase.AllocationBase);
    size_t allocationSize = xmemInfoBase.RegionSize;
    assert((newBaseAddress + allocationSize) < (m_regionAddress + m_regionSize));

    // Free the region back to the place holder using MEM_PRESERVE_PLACEHOLDER
    // The return value is explicitly ignored because this implementation doesn't handle a failure here
    // If this function fails something catastrophic has happened and is probably a bug in the user of PlaceholderRegion
    #pragma warning(suppress: 6333 28160) // MEM_PRESERVE_PLACEHOLDER requires non-zero size with MEM_RELEASE
    VirtualFree(baseAddress, allocationSize, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);

    // query for the details of the region before and after the previously released block
    // this is to determine if the place holder region needs to be coalesced
    char *placeHolderBaseAddress(newBaseAddress);
    char *placeHolderAddressHigh(newBaseAddress + allocationSize);
    if (placeHolderBaseAddress != m_regionAddress)
    {
        XMEM_BASIC_INFORMATION xmemInfoLow;
        XMemVirtualQuery(GetCurrentProcess(), placeHolderBaseAddress - 1, &xmemInfoLow);
        if (xmemInfoLow.State & MEM_RESERVE)
            placeHolderBaseAddress = reinterpret_cast<char *>(xmemInfoLow.AllocationBase);
    }

    if (placeHolderAddressHigh != (m_regionAddress + m_regionSize))
    {
        XMEM_BASIC_INFORMATION xmemInfoHigh;
        XMemVirtualQuery(GetCurrentProcess(), placeHolderAddressHigh + 1, &xmemInfoHigh);

        if (xmemInfoHigh.State & MEM_RESERVE)
            placeHolderAddressHigh = ((reinterpret_cast<char *>(xmemInfoHigh.AllocationBase)) + xmemInfoHigh.RegionSize);
    }

    // Coalesce the newly released block back into the overall place holder region.
    uint64_t placeHolderSize(static_cast<uint64_t>(placeHolderAddressHigh - placeHolderBaseAddress));
    #pragma warning(suppress: 6333 28160) // MEM_COALESCE_PLACEHOLDERS requires non-zero size with MEM_RELEASE
    VirtualFree(reinterpret_cast<void *>(placeHolderBaseAddress), placeHolderSize, MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS);
    ValidateAllocationList(true);
}

void *PlaceholderRegion::Allocate(void *baseAddress, size_t allocationSize, uint32_t allocationType, uint64_t xmemFlags, uint32_t pageProtection)
{
    ValidateAllocationList(true);

    // Graphics allocations can only be made in a place holder region that was created with the XMEM_GRAPHICS flag
    if (!m_graphicsValid && ((xmemFlags & XMEM_GRAPHICS) == XMEM_GRAPHICS))
        return nullptr;

    size_t roundValue = c_4kRoundValue;
    if ((allocationType & MEM_64K_PAGES) == MEM_64K_PAGES)
        roundValue = c_64kRoundValue;
    else if ((allocationType & MEM_2MB_PAGES) == MEM_2MB_PAGES)
        roundValue = c_2MBRoundValue;
    allocationSize = (allocationSize + roundValue) & ~roundValue;

    if (m_regionAddress == nullptr)
        return nullptr;
    if (allocationSize > m_regionSize)
        return nullptr;
    char *baseAddressAsCharPtr = static_cast<char *> (baseAddress);
    if (baseAddress && ((baseAddressAsCharPtr < m_regionAddress) || ((baseAddressAsCharPtr + allocationSize) >= (m_regionAddress + m_regionSize))))
        return nullptr;

    bool exactFit(false);
    if (!baseAddress)
    {
        baseAddress = FindOpenSpace(allocationSize, roundValue,exactFit);
        if (!baseAddress)
            return nullptr;
    }
    else
    {
        XMEM_BASIC_INFORMATION xmemInfoBase;
        XMemVirtualQuery(GetCurrentProcess(), baseAddress, &xmemInfoBase);
        exactFit = xmemInfoBase.RegionSize == allocationSize;
    }

    // The memory block being allocated from the place holder region must first be released using VirtualFree and the MEM_PRESERVE_PLACEHOLDER flag
    // This splits the place holder region into multiple pieces, the new block and the blocks that still exist within the place holder region
    // If it's an exact fit then it's an error to release it and preserve the placeholder, there isn't any leftover memory to preserve as a placeholder
        #pragma warning(suppress: 6333 28160) // MEM_PRESERVE_PLACEHOLDER requires non-zero size with MEM_RELEASE
        if (!exactFit && !VirtualFree(baseAddress, allocationSize, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER))
        {
            return nullptr;
        }

    // After the block has been split out of the place holder region it can be reserved as a normal memory block
    // This allows various flags such as the cache flags, the page size, the page protection, etc. to be set on the new memory block
    if (!XMemVirtualAlloc(baseAddress, allocationSize, (allocationType & ~MEM_COMMIT) | MEM_REPLACE_PLACEHOLDER | MEM_RESERVE, xmemFlags, pageProtection))
    {
        ForceRelease(baseAddress);
        return nullptr;
    }

    // After the new block has been reserved it's treated as any other memory block that was reserved through XMemVirtualAlloc/VirtualAlloc
    // For example it needs to be committed using XMemVirtualAlloc with the MEM_COMMIT flag
    if (allocationType & MEM_COMMIT)
    {
        if (!XMemVirtualAlloc(baseAddress, allocationSize, allocationType, xmemFlags, pageProtection))
        {
            ForceRelease(baseAddress);
            return nullptr;
        }
    }
    InsertAllocationBlock(static_cast<char *> (baseAddress), allocationSize);
    ValidateAllocationList(true);
    return baseAddress;
}

// When a memory block allocated out of a place holder region is no longer needed it needs to be released back to the place holder region
bool PlaceholderRegion::Release(void *baseAddress)
{
    ValidateAllocationList(true);
    if (!baseAddress)
        return true;

    // It's not possible to release just a sub-portion of the block back to a place holder, it's all or nothing
    // Because of this the true location and size of the block are used from the tracking structure
    auto matchingBlock = FindAllocationBlock(static_cast<char *> (baseAddress));

    assert(matchingBlock != m_allocations.end());
    assert(matchingBlock->freeBlock == false);

    // When releasing the block the MEM_PRESERVE_PLACEHOLDER flag needs to be used
    #pragma warning(suppress: 6333 28160) // MEM_PRESERVE_PLACEHOLDER requires non-zero size with MEM_RELEASE
    if (!VirtualFree(matchingBlock->blockStart(), matchingBlock->blockSize, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER))
        return false;

    // If the neighboring blocks in the place holder region are still belong to the place holder region then all the blocks need to be coalesced back together
    auto prevBlock = m_allocations.end();
    auto nextBlock = matchingBlock + 1;
    AllocationBlock coalescedBlock(matchingBlock->baseAddress, matchingBlock->blockSize, true);
    if (matchingBlock != m_allocations.begin())
    {
        prevBlock = matchingBlock - 1;
        if (prevBlock->freeBlock)
        {
            coalescedBlock.baseAddress = prevBlock->baseAddress;
            coalescedBlock.blockSize += prevBlock->blockSize;
        }
        else
        {
            prevBlock = m_allocations.end();
        }
    }
    if (nextBlock != m_allocations.end())
    {
        if (nextBlock->freeBlock)
        {
            coalescedBlock.blockSize += nextBlock->blockSize;
        }
        else
        {
            nextBlock = m_allocations.end();
        }
    }

    // If there is an adjacent free block in the place holder region then coalesce them using the MEM_COALESCE_PLACEHOLDERS flag
    if ((prevBlock != m_allocations.end()) || (nextBlock != m_allocations.end()))
    {
        #pragma warning(suppress: 6333 28160) // MEM_COALESCE_PLACEHOLDERS requires non-zero size with MEM_RELEASE
        if (!VirtualFree(reinterpret_cast<void *>(coalescedBlock.blockStart()), coalescedBlock.blockSize, MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS))
        {
            DebugBreak();
        }
    }

    *matchingBlock = coalescedBlock;
    if (prevBlock != m_allocations.end())
    {
        matchingBlock = m_allocations.erase(prevBlock);
    }
    nextBlock = matchingBlock + 1;
    if ((nextBlock != m_allocations.end()) && (nextBlock->freeBlock))
    {
        m_allocations.erase(nextBlock);
    }
    ValidateAllocationList(true);
    return true;
}
