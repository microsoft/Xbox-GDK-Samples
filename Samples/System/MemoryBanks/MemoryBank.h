//--------------------------------------------------------------------------------------
// MemoryBank.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <xmem.h>
#include "TitleReservedMemory.h"

// Creates and manages a memory bank.
// Can either support:
//     A basic single memory bank
//     A shared memory bank where multiple virtual pages are mapped to the same physical page
//     A bank that supports page swapping or rotating, where virtual addresses are moved between multiple physical addresses
class MemoryBank
{
private:
    enum class BankType
    {
        UNDEFINED,
        BASIC_BANK,
        SHARED_BANK,
        ROTATE_BANK,
    };

    // Mapping physical pages to virtual pages requires either 64k or 4MB pages for both
    static constexpr size_t c_SIZE_PER_ALLOCATED_PHYSICAL_PAGE = (64 * 1024);
    static constexpr size_t c_SIZE_THRESHOLD_2MB_PHYSICAL_PAGE_ALLOCATION = (2 * 1024 * 1024);
    static constexpr size_t c_NUM_PHYSICAL_PAGES_PER_2MB = (2 * 1024 * 1024) / c_SIZE_PER_ALLOCATED_PHYSICAL_PAGE;

    // A bank represents a virtual memory address range and not the backing physical pages
    BankType m_bankType;
    void **m_banks;								// virtual address of each bank in this group
    size_t m_numberOfBanks;						// The number of virtual address banks in the m_banks array
    size_t m_bankSize;							// The size of each bank in the m_banks array

    // Used for mapping physical pages
    size_t m_numberOfPhysicalBanks;				// The number of physical memory arrays, this does not have to equal the number of virtual address banks
    uintptr_t **m_physicalPageArray;			// A physical memory bank is a list of 64k pages from XMemAllocateTitlePhysicalPages
    size_t m_numberOfPhysicalPagesPerBank;		// The number of 64k pages for each entry of the m_physicalPageArray
    uintptr_t **m_physicalPageArray2MB = nullptr;		// A list of the first physical page for every 2MB cluster, used when mapping into 2MB page reserved memory
    size_t m_numberOfEntriesPerBank2MB = 0;         // How many entries are in each m_physicalPageArray2MB slot

    // When mapping physical pages into memory allocated as 2MB pages, only the first physical page of each 2MB cluster is used
    void CreatePageArray2MB()
    {
        m_physicalPageArray2MB = new uintptr_t*[m_numberOfPhysicalBanks];
        m_numberOfEntriesPerBank2MB = m_numberOfPhysicalPagesPerBank / c_NUM_PHYSICAL_PAGES_PER_2MB;
        for (uint32_t i = 0; i < m_numberOfPhysicalBanks; i++)
        {
            m_physicalPageArray2MB[i] = new uintptr_t[m_numberOfEntriesPerBank2MB];
            for (uint32_t j = 0; j < m_numberOfEntriesPerBank2MB; j++)
            {
                m_physicalPageArray2MB[i][j] = m_physicalPageArray[i][j*c_NUM_PHYSICAL_PAGES_PER_2MB];
            }
        }
    }

public:
    MemoryBank(const MemoryBank&) = delete;
    MemoryBank& operator=(const MemoryBank&) = delete;
    MemoryBank(MemoryBank&& rhs) = default;

    MemoryBank() :m_bankType(BankType::UNDEFINED), m_banks(nullptr), m_numberOfBanks(0), m_bankSize(0), m_numberOfPhysicalBanks(0), m_physicalPageArray(nullptr), m_numberOfPhysicalPagesPerBank(0) {}
    ~MemoryBank() { ReleaseBank(); }

    // Creates a single basic memory bank with the option to create it at a system defined address or an address of choice
    // There is no underlying tracking of the physical pages since this is a basic memory bank created through XMemVirtualAlloc
    bool CommitBank(size_t bankSize, uintptr_t baseAddressDesired = 0)
    {
        assert(!m_banks);
        assert(!m_physicalPageArray);
        try
        {
            // The basic memory bank only supports one bank, there is no sharing or rotating
            m_numberOfBanks = 1;
            m_banks = new void *[m_numberOfBanks]{};
            m_bankSize = bankSize;

            // If a specific address is desired then the address range needs to be reserved first, it's not possible to reserve and commit in the same call
            // If the address range is in the reserved 4TB-8TB range then directly commit it
            if ((baseAddressDesired < ATG::c_LOW_FIXED_RESERVED_ADDRESS) || (baseAddressDesired > ATG::c_HIGH_FIXED_RESERVED_ADDRESS))
            {
                m_banks[0] = XMemVirtualAlloc(reinterpret_cast<void *> (baseAddressDesired), bankSize, MEM_RESERVE, XMEM_CPU, PAGE_READWRITE);
                if (m_banks[0] == nullptr)
                    throw std::logic_error("Failed to reserve virtual memory when creating basic bank");
                // Once the address is reserved it can be committed. This can also be done in one step if desired with the request to reserve
                m_banks[0] = XMemVirtualAlloc(m_banks[0], bankSize, MEM_COMMIT, XMEM_CPU, PAGE_READWRITE);
            }
            else
            {
                m_banks[0] = ATG::AllocateFixedAddress(reinterpret_cast<void *>(baseAddressDesired), bankSize);
            }

            if (m_banks[0] == nullptr)
                throw std::logic_error("Failed to commit virtual memory when creating basic bank");
        }
        catch (const std::exception& /*except*/)
        {
            ReleaseBank();
            return false;
        }
        m_bankType = BankType::BASIC_BANK;
        return true;
    }

    // Creates a shared memory bank. Each bank shares the same backing physical pages
    // Shared banks can be created with adjacent virtual addresses.
    //   This is very useful for creating ring buffers without the need to break memory copies across boundaries.
    bool CommitSharedBanks(size_t bankSize, size_t numberOfBanks = 1, bool adjacentBanks = false, uintptr_t baseAddressDesired = 0)
    {
        try
        {
            // Only create one set of physical pages
            m_numberOfPhysicalBanks = 1;
            m_numberOfPhysicalPagesPerBank = bankSize / c_SIZE_PER_ALLOCATED_PHYSICAL_PAGE;
            m_physicalPageArray = new uintptr_t *[1];
            m_physicalPageArray[0] = new uintptr_t[m_numberOfPhysicalPagesPerBank];

            size_t actualPagesAllocated = m_numberOfPhysicalPagesPerBank;
            // The first step is to allocate the physical pages for the bank.
            // Physical pages need to be allocated as either 64k or 4MB contiguous pages. However the OS will return them as an array of 64k page addresses
            if (!XMemAllocatePhysicalPages(unsigned(bankSize < c_SIZE_THRESHOLD_2MB_PHYSICAL_PAGE_ALLOCATION ? MEM_64K_PAGES : MEM_2MB_PAGES),
                &actualPagesAllocated, m_physicalPageArray[0]))
            {
                delete[] m_physicalPageArray;
                m_physicalPageArray = nullptr;
                throw std::logic_error("Failed to allocate physical pages when creating shared banks");
            }
            // It's possible when allocating physical pages that a smaller amount is returned. Consider this a failure for this sample
            if (actualPagesAllocated < m_numberOfPhysicalPagesPerBank)
            {
                XMemFreePhysicalPages(actualPagesAllocated, m_physicalPageArray[0]);
                m_physicalPageArray[0] = nullptr;
                throw std::logic_error("Failed to allocate the requested number of physical pages when creating shared banks");
            }

            CreatePageArray2MB();

            m_numberOfBanks = numberOfBanks;
            m_bankSize = actualPagesAllocated * c_SIZE_PER_ALLOCATED_PHYSICAL_PAGE;
            m_banks = new void *[m_numberOfBanks]{};

            // If the virtual banks have been requested to be adjacent then a single virtual address range needs to be reserved
            if (adjacentBanks)
            {
                void *baseVirtualAddress;
                // Reserve the full virtual address range, it can be reserved at a known location or let the OS decide
                // Note we have to use XMemVirtualAlloc so the new XMEM_MAPPABLE flag can be set
                baseVirtualAddress = XMemVirtualAlloc(reinterpret_cast<void *> (baseAddressDesired),
                                                      m_bankSize*m_numberOfBanks,
                                                      unsigned(MEM_RESERVE | (bankSize < c_SIZE_THRESHOLD_2MB_PHYSICAL_PAGE_ALLOCATION ? MEM_64K_PAGES : MEM_2MB_PAGES)),
                                                      XMEM_MAPPABLE | XMEM_CPU,
                                                      PAGE_READWRITE);
                if (baseVirtualAddress == nullptr)
                    throw std::logic_error("Failed to reserve virtual memory when creating basic bank");
                for (uint32_t i = 0; i < m_numberOfBanks; i++)
                {
                    void *bankVirtualAddress = (reinterpret_cast<char*>(baseVirtualAddress)) + (m_bankSize * i);
                    // Map each of the physical pages into a section of the virtual address range, physical pages can be mapped multiple times
                    if (bankSize < c_SIZE_THRESHOLD_2MB_PHYSICAL_PAGE_ALLOCATION)
                    {
                        m_banks[i] = XMemMapPhysicalPages(bankVirtualAddress, m_numberOfPhysicalPagesPerBank, m_physicalPageArray[0]);
                    }
                    else
                    {
                        m_banks[i] = XMemMapPhysicalPages(bankVirtualAddress, m_numberOfEntriesPerBank2MB, m_physicalPageArray2MB[0]);
                    }
                    if (m_banks[i] == nullptr)
                    {
                        throw std::logic_error("Failed to map the physical pages when creating adjacent shared banks");
                    }
                }
            }
            else    // Adjacent virtual address ranges were not requested, let the OS decide the virtual address to use for each bank
            {
                for (uint32_t i = 0; i < m_numberOfBanks; i++)
                {
                    void *baseAddress = XMemVirtualAlloc(nullptr,
                                                         m_bankSize,
                                                         unsigned(MEM_RESERVE | (bankSize < c_SIZE_THRESHOLD_2MB_PHYSICAL_PAGE_ALLOCATION ? MEM_64K_PAGES : MEM_2MB_PAGES)),
                                                         XMEM_MAPPABLE | XMEM_CPU,
                                                         PAGE_READWRITE);
                    if (baseAddress == nullptr)
                        throw std::logic_error("Failed to reserve virtual memory when creating shared banks");
                    if (bankSize < c_SIZE_THRESHOLD_2MB_PHYSICAL_PAGE_ALLOCATION)
                    {
                        m_banks[i] = XMemMapPhysicalPages(baseAddress, m_numberOfPhysicalPagesPerBank, m_physicalPageArray[0]);
                    }
                    else
                    {
                        m_banks[i] = XMemMapPhysicalPages(baseAddress, m_numberOfEntriesPerBank2MB, m_physicalPageArray2MB[0]);
                    }
                    if (m_banks[i] == nullptr)
                    {
                        throw std::logic_error("Failed to map the physical pages when creating shared banks");
                    }
                }
            }
        }
        catch (const std::exception& /*except*/)
        {
            ReleaseBank();
            return false;
        }
        m_bankType = BankType::SHARED_BANK;
        return true;
    }

    // Creates a set of banks that can be rotated or bank swapped, virtual addresses can be swapped between each physical page array
    // This is very useful to remove memory copies in certain patterns
    bool CommitRotateBanks(size_t bankSize, size_t numberOfBanks)
    {
        try
        {
            m_numberOfBanks = numberOfBanks;
            m_numberOfPhysicalBanks = numberOfBanks;
            m_numberOfPhysicalPagesPerBank = bankSize / c_SIZE_PER_ALLOCATED_PHYSICAL_PAGE;
            m_physicalPageArray = new uintptr_t *[m_numberOfPhysicalBanks];
            memset(m_physicalPageArray, 0, sizeof(uintptr_t) * m_numberOfPhysicalBanks);

            for (uint32_t i = 0; i < m_numberOfBanks; i++)
            {
                size_t actualPagesAllocated = m_numberOfPhysicalPagesPerBank;
                m_physicalPageArray[i] = new uintptr_t[m_numberOfPhysicalPagesPerBank];
                // The first step is to allocate the physical pages for the bank.
                // Physical pages need to be allocated as either 64k or 4MB contiguous pages. However the OS will return them as an array of 64k page addresses
                if (!XMemAllocatePhysicalPages(unsigned(bankSize < c_SIZE_THRESHOLD_2MB_PHYSICAL_PAGE_ALLOCATION ? MEM_64K_PAGES : MEM_2MB_PAGES),
                    &actualPagesAllocated, m_physicalPageArray[i]))
                {
                    throw std::logic_error("Failed to allocate physical pages when creating rotating banks");
                }
                // It's possible when allocating physical pages that a smaller amount is returned. Consider this a failure for this sample
                if (actualPagesAllocated < m_numberOfPhysicalPagesPerBank)
                {
                    XMemFreePhysicalPages(actualPagesAllocated, m_physicalPageArray[i]);
                    m_physicalPageArray[i] = nullptr;
                    throw std::logic_error("Failed to allocate requested number of physical pages when creating rotating banks");
                }
                m_bankSize = actualPagesAllocated * c_SIZE_PER_ALLOCATED_PHYSICAL_PAGE;
            }
            CreatePageArray2MB();
            m_banks = new void *[m_numberOfBanks]{};
            for (uint32_t i = 0; i < m_numberOfBanks; i++)
            {
                // Allocate a virtual address range for each physical page array
                auto baseAddress = XMemVirtualAlloc(nullptr,
                                                    m_bankSize,
                                                    unsigned(MEM_RESERVE | (bankSize < c_SIZE_THRESHOLD_2MB_PHYSICAL_PAGE_ALLOCATION ? MEM_64K_PAGES : MEM_2MB_PAGES)),
                                                    XMEM_MAPPABLE | XMEM_CPU,
                                                    PAGE_READWRITE);
                if (baseAddress == nullptr)
                    throw std::logic_error("Failed to reserve virtual memory when creating rotating banks");

                if (bankSize < c_SIZE_THRESHOLD_2MB_PHYSICAL_PAGE_ALLOCATION)
                {
                    m_banks[i] = XMemMapPhysicalPages(baseAddress, m_numberOfPhysicalPagesPerBank, m_physicalPageArray[i]);
                }
                else
                {
                    m_banks[i] = XMemMapPhysicalPages(baseAddress, m_numberOfEntriesPerBank2MB, m_physicalPageArray2MB[i]);
                }

                if (m_banks[i] == nullptr)
                    throw std::logic_error("Failed to map the physical pages when creating rotating banks");
            }
        }
        catch (const std::exception& /*except*/)
        {
            ReleaseBank();
            return false;
        }
        m_bankType = BankType::ROTATE_BANK;
        return true;
    }

    void ReleaseBank()
    {
        if (m_banks)
        {
            for (size_t i = 0; i < m_numberOfBanks; i++)
            {
#pragma warning(push)
#pragma warning(disable : 6001) // m_banks is value-initialized with {} on allocation
                if (m_banks[i])  // it's possible for this to fail for adjacent pages, however they are still all released with the first array entry
#pragma warning(pop)
                {
                    uintptr_t bankAddress = reinterpret_cast<uintptr_t> (m_banks[i]);
                    if ((bankAddress < ATG::c_LOW_FIXED_RESERVED_ADDRESS) || (bankAddress > ATG::c_HIGH_FIXED_RESERVED_ADDRESS))
#pragma warning(suppress : 28183) // m_banks[i] is verified non-null above
                        VirtualFree(m_banks[i], 0, MEM_RELEASE);
                    else
                        ATG::ReleaseFixedAddress(m_banks[i], 0);
                }
            }
            delete[] m_banks;
            m_banks = nullptr;
        }

        if (m_physicalPageArray)
        {
            for (size_t i = 0; i < m_numberOfPhysicalBanks; i++)
            {
                if (m_physicalPageArray[i])
                    XMemFreePhysicalPages(m_numberOfPhysicalPagesPerBank, m_physicalPageArray[i]);
                delete[] m_physicalPageArray[i];
                delete[] m_physicalPageArray2MB[i];
            }
            delete[] m_physicalPageArray;
            m_physicalPageArray = nullptr;
            delete[] m_physicalPageArray2MB;
            m_physicalPageArray2MB = nullptr;
        }
    }

    // Swap the virtual addresses used by two physical blocks
    bool SwapBanks(size_t bankIndex1, size_t bankIndex2)
    {
        assert(m_bankType == BankType::ROTATE_BANK);
        assert(bankIndex1 <= m_numberOfBanks);
        assert(bankIndex2 <= m_numberOfBanks);
        try
        {
            void *bankAddress1 = m_banks[bankIndex1];
            void *bankAddress2 = m_banks[bankIndex2];

            for (uint32_t i = 0; i < m_numberOfPhysicalPagesPerBank; i++)
            {
                std::swap(m_physicalPageArray[bankIndex1][i], m_physicalPageArray[bankIndex2][i]);
            }
            for (uint32_t i = 0; i < m_numberOfEntriesPerBank2MB; i++)
            {
                std::swap(m_physicalPageArray2MB[bankIndex1][i], m_physicalPageArray2MB[bankIndex2][i]);
            }

            // Remap the two physical blocks with the swapped virtual addresses.
            if (m_bankSize < c_SIZE_THRESHOLD_2MB_PHYSICAL_PAGE_ALLOCATION)
            {
                void *newBank1Address = XMemMapPhysicalPages(bankAddress1, m_numberOfPhysicalPagesPerBank, m_physicalPageArray[bankIndex1]);
                if (newBank1Address != m_banks[bankIndex1])
                    throw std::logic_error("Failed to remap bank 1 in SwapBanks");
                void *newBank2Address = XMemMapPhysicalPages(bankAddress2, m_numberOfPhysicalPagesPerBank, m_physicalPageArray[bankIndex2]);
                if (newBank2Address != m_banks[bankIndex2])
                    throw std::logic_error("Failed to remap bank 2 in SwapBanks");
            }
            else
            {
                void *newBank1Address = XMemMapPhysicalPages(bankAddress1, m_numberOfEntriesPerBank2MB, m_physicalPageArray2MB[bankIndex1]);
                if (newBank1Address != m_banks[bankIndex1])
                    throw std::logic_error("Failed to remap bank 1 in SwapBanks");
                void *newBank2Address = XMemMapPhysicalPages(bankAddress2, m_numberOfEntriesPerBank2MB, m_physicalPageArray2MB[bankIndex2]);
                if (newBank2Address != m_banks[bankIndex2])
                    throw std::logic_error("Failed to remap bank 2 in SwapBanks");
            }
        }
        catch (const std::exception /*except*/)
        {
            ReleaseBank();
            return false;
        }
        return true;
    }

    // In many cases it's useful to convert a block of memory to read-only
    // This is useful for static data that is created once and then doesn't change throughout it's lifetime
    // Any attempt to change the memory will result in an immediate exception, useful to track down potential memory overwrite bugs
    // When using shared banks (multiple virtual addresses to the same physical block) each virtual address can have different protection flags
    // This means when accessing memory through one address it's read/write, however it's read-only through a different address.
    // NOTE: It is not possible to have different cache management flags such as write combine for each virtual address range, any attempt to do so will fail
    bool LockBank(size_t bankIndex = SIZE_MAX)  // SIZE_MAX means lock all banks
    {
        assert(m_banks);
        if (bankIndex == SIZE_MAX)
        {
            bool toret = true;
            for (size_t i = 0; i < m_numberOfBanks; i++)
            {
                DWORD oldProtect;
                if (m_banks[i])
                {
                    // Set this virtual address bank to be readonly
                    if (VirtualProtect(m_banks[i], m_bankSize, PAGE_READONLY, &oldProtect) == 0)
                        toret = false;
                }
            }
            return toret;
        }
        else
        {
            assert(bankIndex <= m_numberOfBanks);
            DWORD oldProtect;
            if (m_banks[bankIndex] == nullptr)
                return false;
            // Set this virtual address bank to be readonly
            return VirtualProtect(m_banks[bankIndex], m_bankSize, PAGE_READONLY, &oldProtect) != 0;
        }
    }

    operator void * () const { assert(m_banks); return m_banks[0]; }
    void *get(size_t bankIndex = 0) const { assert(bankIndex <= m_numberOfBanks); return m_banks[bankIndex]; }
};
