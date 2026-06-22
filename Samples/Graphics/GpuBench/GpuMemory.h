//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

enum MemoryType : uint32_t
{
    MEMORY_TYPE_GARLIC,
    MEMORY_TYPE_ONION,
    MEMORY_TYPE_ESRAM,

    MEMORY_TYPE_COUNT
};
constexpr auto g_esramSizeInBytes = 32U * 1024U * 1024U;

#ifdef _GAMING_XBOX_SCARLETT

// Easier to redefine this than to #define it out
typedef enum D3D11_MAP_ESRAM_FLAG
{
    D3D11_MAP_ESRAM_64KB_PAGES	= 0x1,
    D3D11_MAP_ESRAM_LARGE_PAGES	= D3D11_MAP_ESRAM_64KB_PAGES,
    D3D11_MAP_ESRAM_4MB_PAGES	= 0x2,
    D3D11_MAP_ESRAM_2MB_PAGES	= 0x3,
} 	D3D11_MAP_ESRAM_FLAG;

#endif

// Not an all-purpose allocator; just used for obtaining backing memory for placement allocations.
// For DRAM, each allocation is its own VirtualAlloc call.
// For ESRAM, we use a linear allocator, and count on all live allocations being freed in a row.
class GpuBenchAllocator
{
    D3D12_GPU_VIRTUAL_ADDRESS           m_esramAddress;
    D3D12_GPU_VIRTUAL_ADDRESS           m_nextEsramAddress;
    D3D11_MAP_ESRAM_FLAG                m_esramPageFlagInUse;
    uint32_t                            m_esramLiveAllocations;
    bool                                m_esramNeedsReset;

    uint32_t                            m_dramLiveAllocations;

    static constexpr auto               g_64KBPageSize = 64U * 1024U;
    static constexpr auto               g_2MBPageSize = 2U * 1024U * 1024U;
    static constexpr auto               g_esramSizeIn64KBPages = g_esramSizeInBytes / g_64KBPageSize;
    static constexpr auto               g_esramSizeIn2MBPages = g_esramSizeInBytes / g_2MBPageSize;

    static_assert(0ULL == g_esramSizeInBytes % g_64KBPageSize, "Expect ESRAM to be a whole number of pages");
    static_assert(0ULL == g_esramSizeInBytes % g_2MBPageSize, "Expect ESRAM to be a whole number of pages");

public:
    GpuBenchAllocator() :
        m_esramAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
        m_nextEsramAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
        m_esramPageFlagInUse(D3D11_MAP_ESRAM_FLAG(0U)),
        m_esramLiveAllocations(0U),
        m_esramNeedsReset(true), 
        m_dramLiveAllocations(0U)
    {
    }

    ~GpuBenchAllocator()
    {
        // Assert that all of ESRAM was freed
        assert(0U == m_esramLiveAllocations);
        assert(m_esramNeedsReset);
        assert(D3D12_GPU_VIRTUAL_ADDRESS_NULL == m_esramAddress);
        assert(D3D12_GPU_VIRTUAL_ADDRESS_NULL == m_nextEsramAddress);

        // Check that every DRAM allocation was freed
        assert(0U == m_dramLiveAllocations);
    }

private:
    // Returns a mapped pointer to all of ESRAM, if it's available, and to 32 MB of DRAM otherwise
    // Default to 2MB pages, to avoid confounding measurements with TLB misses
    D3D12_GPU_VIRTUAL_ADDRESS AllocateAllEsram(uint32_t pageFlag)
    {
        assert(MEM_2MB_PAGES == pageFlag || MEM_64K_PAGES == pageFlag);
        assert(m_esramNeedsReset 
            || (MEM_2MB_PAGES == pageFlag && D3D11_MAP_ESRAM_2MB_PAGES == m_esramPageFlagInUse)
            || (MEM_64K_PAGES == pageFlag && D3D11_MAP_ESRAM_LARGE_PAGES == m_esramPageFlagInUse));  // Cannot switch page size unless you first free all of ESRAM

        m_esramPageFlagInUse = (MEM_2MB_PAGES == pageFlag) ? D3D11_MAP_ESRAM_2MB_PAGES : D3D11_MAP_ESRAM_64KB_PAGES;
        const auto esramSizeInPages = (MEM_2MB_PAGES == pageFlag) ? g_esramSizeIn2MBPages : g_esramSizeIn64KBPages;

        auto allocationType = DWORD(pageFlag | MEM_RESERVE);
        auto xMemAllocationFlags = DWORD(XMEM_GRAPHICS | XMEM_MAPPABLE);
        auto pageProtect = DWORD(PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_READWRITE);
        auto address = XMemVirtualAlloc(nullptr, g_esramSizeInBytes, allocationType, xMemAllocationFlags, pageProtect);
        if (!address)
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        if (IsDurangoClass())
        {
            // Allocate all of ESRAM
            auto pageList = new UINT[esramSizeInPages];
            for (auto page = 0U; page < esramSizeInPages; ++page)
            {
                pageList[page] = page;
            }
#ifdef _GAMING_XBOX_SCARLETT
            assert(false);  // This might trigger in backcompat
#else
            DX::ThrowIfFailed(D3DMapEsramMemory(m_esramPageFlagInUse, address, esramSizeInPages, pageList));
#endif
            delete[] pageList;
        }
        else
        {
            // There is no ESRAM, so we'll have to use DRAM and hope we don't become memory bound
            // Commit the reserved allocation
			allocationType = MEM_COMMIT;
            address = XMemVirtualAlloc(address, g_esramSizeInBytes, allocationType, xMemAllocationFlags, pageProtect);
            if (!address)
            {
                DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            }
        }

        return reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(address);
    }

    void FreeAllEsram(D3D12_GPU_VIRTUAL_ADDRESS& address)
    {
        if (IsDurangoClass())
        {
#ifdef _GAMING_XBOX_SCARLETT
            assert(false);  // This might trigger in backcompat
#else
            const auto esramSizeInPages = (D3D11_MAP_ESRAM_2MB_PAGES == m_esramPageFlagInUse) ? g_esramSizeIn2MBPages : g_esramSizeIn64KBPages;
            DX::ThrowIfFailed(D3DUnmapEsramMemory(m_esramPageFlagInUse, reinterpret_cast<void*>(address), esramSizeInPages));
#endif
        }

        auto freeType = DWORD(MEM_RELEASE);
        auto success = VirtualFree(reinterpret_cast<void*>(address), 0ULL, freeType);
        if (!success)
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        address = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
    }

public:
    D3D12_GPU_VIRTUAL_ADDRESS AllocateResourceMemory(ID3D12Device* device,
        const D3D12_RESOURCE_DESC* desc,
        MemoryType type, 
        uint32_t pageFlag = MEM_64K_PAGES)    // TODO: Switch to 2 MB pages when these are supported for ESRAM
    {
        auto allocationInfo = device->GetResourceAllocationInfo(0U, 1U, desc);

        D3D12_GPU_VIRTUAL_ADDRESS p = D3D12_GPU_VIRTUAL_ADDRESS_NULL;

        switch (type)
        {
        case MEMORY_TYPE_GARLIC:
        case MEMORY_TYPE_ONION:
        {
            // Assume that allocating entire pages is enough to satisfy alignment
            assert(g_64KBPageSize >= allocationInfo.Alignment || MEM_2MB_PAGES == pageFlag);
            auto allocationType = DWORD(pageFlag | MEM_RESERVE | MEM_COMMIT);
            auto xMemAllocationFlags = DWORD(XMEM_GRAPHICS);
            auto pageProtect = DWORD(PAGE_READWRITE | PAGE_GRAPHICS_READWRITE);

            if (MEMORY_TYPE_GARLIC == type)
            {
                pageProtect |= PAGE_WRITECOMBINE;
            }
            else if (MEMORY_TYPE_ONION == type)
            {
                pageProtect |= PAGE_GRAPHICS_COHERENT;
            }

            p = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(XMemVirtualAlloc(nullptr, allocationInfo.SizeInBytes, allocationType, xMemAllocationFlags, pageProtect));

            if (D3D12_GPU_VIRTUAL_ADDRESS_NULL == p)
            {
                DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            }

            ++m_dramLiveAllocations;
        }
        break;

        case MEMORY_TYPE_ESRAM:
        {
            if (D3D12_GPU_VIRTUAL_ADDRESS_NULL == m_esramAddress)
            {
                // This is the first allocation in ESRAM...
                m_esramAddress = m_nextEsramAddress = AllocateAllEsram(pageFlag);
                m_esramNeedsReset = false;
            }

            // We assume that once a test starts freeing ESRAM, it will not allocate any more
            assert(!m_esramNeedsReset);

            assert(m_nextEsramAddress != D3D12_GPU_VIRTUAL_ADDRESS_NULL);
            p = DirectX::AlignUp(m_nextEsramAddress, allocationInfo.Alignment);
            m_nextEsramAddress += allocationInfo.SizeInBytes;
            ++m_esramLiveAllocations;
            assert(m_nextEsramAddress <= m_esramAddress + g_esramSizeInBytes);
        }
        break;
        }

        return p;
    }

    void FreeResourceMemory(D3D12_GPU_VIRTUAL_ADDRESS p, MemoryType type)
    {
        switch (type)
        {
        case MEMORY_TYPE_GARLIC:
        case MEMORY_TYPE_ONION:
        {
            assert(0U < m_dramLiveAllocations);
            auto success = VirtualFree(reinterpret_cast<void*>(p), 0, MEM_RELEASE);
            if (!success)
            {
                DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            }
            --m_dramLiveAllocations;
        }
        break;

        case MEMORY_TYPE_ESRAM:
            // No memory is actually unmapped or released until all of ESRAM is freed
            assert(0U < m_esramLiveAllocations);
            if (0U == --m_esramLiveAllocations)
            {
                FreeAllEsram(m_esramAddress);
                m_esramAddress = m_nextEsramAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
            }
            m_esramNeedsReset = true;
            break;
        }
    }
};


