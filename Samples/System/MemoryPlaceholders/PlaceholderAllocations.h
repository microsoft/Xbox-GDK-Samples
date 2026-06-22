//--------------------------------------------------------------------------------------
// PlaceholderAllocations.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <xmem.h>

namespace ATG
{
    class PlaceholderRegion
    {
    public:
        static constexpr size_t c_defaultRegionSize = 1ULL * 1024 * 1024 * 1024;
        static constexpr size_t c_2MB = 2ULL * 1024 * 1024;
        static constexpr size_t c_64k = 64ULL * 1024;
        static constexpr size_t c_4k = 4ULL * 1024;
        static constexpr size_t c_2MBRoundValue = (c_2MB - 1);
        static constexpr size_t c_64kRoundValue = (c_64k - 1);
        static constexpr size_t c_4kRoundValue = (c_4k - 1);
    private:
        struct AllocationBlock
        {
        public:
            char* baseAddress;
            size_t blockSize;
            bool freeBlock;
            AllocationBlock(char* newBaseAddress = nullptr, size_t newBlockSize = 0, bool newFreeBlock = true) :baseAddress(newBaseAddress), blockSize(newBlockSize), freeBlock(newFreeBlock) {}
            char* blockStart() const { return baseAddress; }
            char* blockEnd() const { return baseAddress + blockSize; }
        };
        typedef std::vector<AllocationBlock> AllocationList;
        bool m_graphicsValid;
        char* m_regionAddress;
        size_t m_regionSize;
        AllocationList m_allocations;
#ifdef _DEBUG
        void ValidateAllocationList(bool fullValidate = false) const;
        void ValidateAllocationBlock(const AllocationBlock& block) const;
#else
        void ValidateAllocationList(bool /*fullValidate*/ = false) const {}
        void ValidateAllocationBlock(const AllocationBlock& /*block*/) const {}
#endif
        void ForceRelease(void* baseAddress);

        void* FindOpenSpace(size_t sizeNeeded, size_t roundValue,bool& exactFit) const
        {
            for (auto& block : m_allocations)
            {
                if (!block.freeBlock)
                    continue;
                char* alignedAddress = reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(block.blockStart() + roundValue) & ~roundValue);
                uint64_t blockSize;
                if (block.blockSize <= roundValue)
                    blockSize = 0;
                else
                    blockSize = block.blockSize - (alignedAddress - block.blockStart());
                exactFit = block.blockSize == sizeNeeded;
                if (blockSize >= sizeNeeded)
                    return alignedAddress;
            }
            return nullptr;
        }

        AllocationList::iterator FindAllocationBlock(char* baseAddress)
        {
            for (auto iter = m_allocations.begin(); iter != m_allocations.end(); ++iter)
            {
                if ((baseAddress >= iter->blockStart()) && (baseAddress < iter->blockEnd()))
                    return iter;
            }
            return m_allocations.end();
        }

        void SplitBlock(AllocationList::iterator block, char* baseAddress, size_t size)
        {
            assert(block->freeBlock);

            assert(block->blockSize >= size);
            if (block->blockSize == size)
            {
                block->freeBlock = false;
            }
            else
            {
                if (block->blockStart() < baseAddress)
                {
                    AllocationBlock leftBlock(block->blockStart(), static_cast<size_t>(baseAddress - block->blockStart()), true);
                    *block = AllocationBlock(baseAddress, static_cast<size_t>(block->blockEnd() - baseAddress), true);
                    block = m_allocations.insert(block, leftBlock);
                    block++;
                }
                if (block->blockEnd() > (baseAddress + size))
                {
                    *block = AllocationBlock(block->blockStart() + size, static_cast<size_t>(block->blockEnd() - (block->blockStart() + size)), true);
                }
                m_allocations.insert(block, AllocationBlock(baseAddress, size, false));
            }
            ValidateAllocationList();
        }

        void InsertAllocationBlock(char* baseAddress, size_t size)
        {
            ValidateAllocationList();
            auto location = FindAllocationBlock(baseAddress);
            assert(location != m_allocations.end());
            assert(location->freeBlock == true);
            SplitBlock(location, baseAddress, size);
            ValidateAllocationList();
        }

    public:
        explicit PlaceholderRegion(bool needGraphics = false, void* baseAddress = nullptr, size_t requestedAllocationSize = 0)  noexcept :
            m_graphicsValid(needGraphics)
            , m_regionAddress(static_cast<char*> (baseAddress))
            , m_regionSize(requestedAllocationSize)
        {
            if ((m_regionSize != 0) && (m_regionAddress != nullptr))
                SetupRegion(m_graphicsValid, m_regionAddress, m_regionSize);
        }
        ~PlaceholderRegion() {}

        bool SetupRegion(bool needGraphics = false, void* baseAddress = nullptr, size_t requestedAllocationSize = c_defaultRegionSize)
        {
            // The initial place holder reservation is always 2MB aligned.
            m_regionSize = (requestedAllocationSize + c_2MBRoundValue) & ~c_2MBRoundValue;
            m_graphicsValid = needGraphics;
            m_regionAddress = static_cast<char*> (baseAddress);
            if (!m_regionAddress)
            {
                m_regionAddress = static_cast<char*> (XMemVirtualAlloc(NULL, m_regionSize, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, m_graphicsValid ? XMEM_GRAPHICS : XMEM_CPU, PAGE_NOACCESS));
            }
            else
            {
                // All graphics allocations are allocated in a range less than 1TB.
                if (m_graphicsValid && (reinterpret_cast<uintptr_t> (m_regionAddress) >= (1ULL * 1024 * 1024 * 1024 * 1024)))
                {
                    m_regionAddress = nullptr;
                    m_regionSize = 0;
                }
            }
            if (!m_regionAddress)
            {
                m_regionSize = 0;
                return false;
            }
            m_allocations.reserve(1024);
            m_allocations.push_back(AllocationBlock(m_regionAddress, m_regionSize, true));
            return true;
        }

        void* Allocate(void* baseAddress, size_t allocationSize, uint32_t allocationType, uint64_t xmemFlags, uint32_t pageProtection);
        bool Release(void* baseAddress);
    };
}
