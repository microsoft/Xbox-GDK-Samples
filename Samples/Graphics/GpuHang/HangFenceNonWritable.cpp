//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "Hang.h"
#include "Util.h"

class HangFenceNonWritable final : public Hang
{
public:
    HangFenceNonWritable() :
        Hang(),
        m_goodFenceData(nullptr),
        m_goodFenceDataAttributes{},
        m_goodFenceHandle(0),
        m_badFenceData(nullptr),
        m_badFenceDataAttributes{},
        m_badFenceHandle(0)
    {
    }

    virtual ~HangFenceNonWritable() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Fence non-writable";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"FenceNonWritable";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if the title performs manual fence writes to memory which is not GPU-writable. ");
        description.push_back(L"The title must guarantee correct memory type for Write32BitValueTopOfPipeX and related APIs. ");

        return description;
    }

    typedef uint64_t Fence;

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* /* commandQueue */) override
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocator);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandList);

        auto alignment = sizeof(Fence);
        DWORD logAlignment = 0;
        if (0 == _BitScanReverse64(&logAlignment, alignment))
        {
            logAlignment = 0;
        }

        // good fence allocation
        {
            m_goodFenceDataAttributes = MAKE_XALLOC_ATTRIBUTES(
                eXALLOCAllocatorId_GameMin,
                0,
                XALLOC_MEMTYPE_GRAPHICS_WRITECOMBINE, 
                XALLOC_PAGESIZE_2MB,
                logAlignment, 
                false);
            m_goodFenceData = XMemAlloc(sizeof(Fence), m_goodFenceDataAttributes);
            if (nullptr == m_goodFenceData)
            {
                throw std::bad_alloc();
            }
            ZeroMemory(m_goodFenceData, sizeof(Fence));

            // Name the goodFence for hix captures
            auto goodFenceAddr = reinterpret_cast<Fence*>(m_goodFenceData);
            auto goodFence = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS&>(goodFenceAddr);
            m_goodFenceHandle = SET_FENCE_NAME_TO_SELF(device, goodFence);
        }

        // bad fence allocation
        {
            m_badFenceDataAttributes = MAKE_XALLOC_ATTRIBUTES(
                eXALLOCAllocatorId_GameMin,
                0,
                XALLOC_MEMTYPE_GRAPHICS_WRITECOMBINE_GPU_READONLY,  // Whoops, should not have _GPU_READONLY! 
                XALLOC_PAGESIZE_2MB,
                logAlignment, 
                false);
            m_badFenceData = XMemAlloc(sizeof(Fence), m_badFenceDataAttributes);
            if (nullptr == m_badFenceData)
            {
                throw std::bad_alloc();
            }
            ZeroMemory(m_badFenceData, sizeof(Fence));

            // Name the badFence for hix captures
            auto badFenceAddr = reinterpret_cast<Fence*>(m_badFenceData);
            auto badFence = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS&>(badFenceAddr);
            m_badFenceHandle = SET_FENCE_NAME_TO_SELF(device, badFence);
        }
    }

    virtual void Uninitialize(ID3D12Device* device) override
    {
        m_commandList = nullptr;
        m_commandAllocator = nullptr;

        if (m_goodFenceData)
        {
            XMemFree(m_goodFenceData, m_goodFenceDataAttributes);
        }
        if (m_goodFenceHandle)
        {
            UNSET_FENCE_NAME(device, m_goodFenceHandle);
        }

        if (m_badFenceData)
        {
            XMemFree(m_badFenceData, m_badFenceDataAttributes);
        }
        if (m_badFenceHandle)
        {
            UNSET_FENCE_NAME(device, m_badFenceHandle);
        }
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        GpuFullStop(device, commandQueue);
    }

    virtual void Render(ID3D12Device* device, ID3D12CommandQueue* commandQueue, bool hang) override
    {
#ifdef _GAMING_XBOX_SCARLETT
        SCOPED_ERROR_FILTER(device, 0x6BCA2E89);    // ID3D12CommandQueue(Graphics)::ExecuteCommandLists: WARNING: Barrier validation may raise false positives or hang when custom synchronization via Wait32BitValueX or Wait64BitValueX is in use. To disable barrier validation, call ID3D12Device::SetDebugFlagsX with the D3D12XBOX_DEBUG_FLAG_DISABLE_BARRIER_VALIDATION flag, or disable barrier validation on individual resources with ID3D12Resource::SetValidationFlagsX.
#else
        SCOPED_ERROR_FILTER(device, 0xDA62126E);    // [Graphics] Barrier validation may raise false positives or hang when custom synchronization via Wait32BitValueX or Wait64BitValueX is in use. 
#endif

        PIXBeginRetailEvent(commandQueue, PIX_COLOR_DEFAULT, __FUNCTIONW__);

        auto fenceAddr = reinterpret_cast<Fence*>(hang ? m_badFenceData : m_goodFenceData);

        auto fence = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS&>(fenceAddr);
        m_commandList->Write64BitValueBottomOfPipeX(fence, 1, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);
        m_commandList->Wait64BitValueX(fence, D3D12_COMPARISON_FUNC_EQUAL, 1, D3D12XBOX_WAIT_FLAG_NONE);

        DX::ThrowIfFailed(m_commandList->Close());
        commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);

        PIXEndRetailEvent(commandQueue);

        // Force validation now, while we have the errors disabled
        commandQueue->KickoffX();
    }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>          m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>       m_commandList;

    void*                                                   m_goodFenceData;
    ULONGLONG                                               m_goodFenceDataAttributes;
    UINT32                                                  m_goodFenceHandle;

    void*                                                   m_badFenceData;
    ULONGLONG                                               m_badFenceDataAttributes;
    UINT32                                                  m_badFenceHandle;
};

static HangFenceNonWritable hang;
