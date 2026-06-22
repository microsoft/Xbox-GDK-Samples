//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "Hang.h"
#include "Util.h"

class HangFenceUnsignaled final : public Hang
{
public:
    HangFenceUnsignaled() :
        Hang()
    {
    }

    virtual ~HangFenceUnsignaled() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Fence unsignaled";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"FenceUnsignaled";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if the title calls Wait on a fence without calling Signal. ");

        return description;
    }

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* /* commandQueue */) override
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocator);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandList);

        DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_fence);
    }

    virtual void Uninitialize(ID3D12Device* /* device */) override
    {
        m_commandList = nullptr;
        m_commandAllocator = nullptr;

        m_fence = nullptr;
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        GpuFullStop(device, commandQueue);
    }

    virtual void Render(ID3D12Device* /*device*/, ID3D12CommandQueue* commandQueue, UINT /*vendorId*/, bool hang) override
    {
        PIXBeginRetailEvent(commandQueue, PIX_COLOR_DEFAULT, __FUNCTIONW__);

        commandQueue->Wait(m_fence.Get(), 1);

        if (!hang)
        {
            m_fence->Signal(1);
        }

        PIXEndRetailEvent(commandQueue);

        DX::ThrowIfFailed(m_commandList->Close());
        commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>          m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>       m_commandList;

    Microsoft::WRL::ComPtr<ID3D12Fence>                     m_fence;
};

static HangFenceUnsignaled hang;
