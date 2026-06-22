//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "Hang.h"
#include "Util.h"

class HangUnpairedBarrier final : public Hang
{
public:
    HangUnpairedBarrier() :
        Hang()
    {
    }

    virtual ~HangUnpairedBarrier() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Unpaired barrier";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"UnpairedBarrier";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if the title calls ResourceBarrier with D3D12_RESOURCE_BARRIER_FLAG_END_ONLY without. ");
        description.push_back(L"previously having made a matching call with D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY. ");
        description.push_back(L"This hang should trigger a validation error if barrier validation is not disabled. ");

        return description;
    }

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* /* commandQueue */) override
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocator);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandList);

        D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM,
            1024U,
            1024U,
            1U, 
            1U, 
            1U, 
            0U, 
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
        );
        auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_resource.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_resource);
    }

    virtual void Uninitialize(ID3D12Device* /* device */) override
    {
        m_commandList = nullptr;
        m_commandAllocator = nullptr;

        m_resource = nullptr;
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        GpuFullStop(device, commandQueue);
    }

    virtual void Render(ID3D12Device* device, ID3D12CommandQueue* commandQueue, bool hang) override
    {
        SCOPED_ERROR_FILTER(device, 0x753499B4);    // Resource transition end barrier on resource (0x00000001C0002300 "m_resource") is missing a preceding transition begin barrier.
        SCOPED_ERROR_FILTER(device, 0x818EDAB4);    // Resource transition end barrier on resource (0x00000001C0002300 "m_resource") specified StateAfter of PIXEL_SHADER_RESOURCE (0x80), which is incompatible with the resource's current state of RENDER_TARGET (0x4).

        PIXBeginRetailEvent(m_commandList.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);

        if (!hang)
        {
            auto barrierBegin = CD3DX12_RESOURCE_BARRIER::Transition(m_resource.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
            m_commandList->ResourceBarrier(1U, &barrierBegin);
        }

        auto barrierEnd = CD3DX12_RESOURCE_BARRIER::Transition(m_resource.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);
        m_commandList->ResourceBarrier(1U, &barrierEnd);

        PIXEndRetailEvent(m_commandList.Get());

        DX::ThrowIfFailed(m_commandList->Close());
        commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));

        // Force validation now, while we have the errors disabled
        commandQueue->KickoffX();
    }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>          m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>       m_commandList;

    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_resource;   
};

static HangUnpairedBarrier hang;
