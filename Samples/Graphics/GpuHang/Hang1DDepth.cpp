//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Hang.h"
#include "Util.h"

class Hang1DDepth final : public Hang
{
public:
    // This case can run on async too, but it won't hang, at least not for the same reasons
    Hang1DDepth() :
        Hang(
            ((1U << QueueType::Graphics)),
#ifdef LIVE_DEBUGGING_SUPPORT
            ((1U << HangAction::Dump) | (1U << HangAction::LiveDebug) | (1U << HangAction::LiveDebugThenDump))
#else
            ((1U << HangAction::Dump))
#endif
        )
    {
    }

    virtual ~Hang1DDepth() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"1D Depth clear";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"1DDepth";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if a title attempts to call ClearDepthStencilView on a 1D Depth Buffer on Scarlett. ");
        description.push_back(L"That's valid D3D12 usage, but the Scarlett driver does not support it.");

        return description;
    }


    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* /* commandQueue */) override
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocatorGraphics.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocatorGraphics);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorGraphics.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandListGraphics.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandListGraphics);

        // 1D depth buffer
        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto clearValueDepth = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

        auto descDepth = CD3DX12_RESOURCE_DESC::Tex1D(DXGI_FORMAT_D32_FLOAT, 1024, 1, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &descDepth,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValueDepth,
            IID_GRAPHICS_PPV_ARGS(m_depth1D.ReleaseAndGetAddressOf())));

        D3D12_DESCRIPTOR_HEAP_DESC heapDescDsv =
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            1,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            0,
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&heapDescDsv, IID_GRAPHICS_PPV_ARGS(m_descriptorHeapDsv.ReleaseAndGetAddressOf())));

        device->CreateDepthStencilView(m_depth1D.Get(), nullptr, m_descriptorHeapDsv->GetCPUDescriptorHandleForHeapStart());

        FLOAT color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        auto clearValueColor = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, color);

        auto descColor = CD3DX12_RESOURCE_DESC::Tex1D(DXGI_FORMAT_R8G8B8A8_UNORM, 1024, 1, 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &descColor,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clearValueColor,
            IID_GRAPHICS_PPV_ARGS(m_color1D.ReleaseAndGetAddressOf())));

        D3D12_DESCRIPTOR_HEAP_DESC heapDescRtv =
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            1,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            0,
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&heapDescRtv, IID_GRAPHICS_PPV_ARGS(m_descriptorHeapRtv.ReleaseAndGetAddressOf())));

        device->CreateRenderTargetView(m_color1D.Get(), nullptr, m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart());
    }

    virtual void Uninitialize(ID3D12Device* /* device */) override
    {
        m_commandAllocatorGraphics = nullptr;
        m_commandListGraphics = nullptr;

        m_depth1D = nullptr;
        m_color1D = nullptr;

        m_descriptorHeapDsv = nullptr;
        m_descriptorHeapRtv = nullptr;
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        GpuFullStop(device, commandQueue);
    }

    virtual void Render(ID3D12Device* /*device*/, ID3D12CommandQueue* commandQueue, bool hang) override
    {
        PIXBeginRetailEvent(m_commandListGraphics.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);

        auto rtvDescriptor = m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart();
        auto dsvDescriptor = m_descriptorHeapDsv->GetCPUDescriptorHandleForHeapStart();
        
        if (hang)
        {
            m_commandListGraphics->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        }
        m_commandListGraphics->ClearRenderTargetView(rtvDescriptor, DirectX::Colors::Black, 0, nullptr);

        PIXEndRetailEvent(m_commandListGraphics.Get());

        DX::ThrowIfFailed(m_commandListGraphics->Close());
        commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandListGraphics.GetAddressOf()));

        DX::ThrowIfFailed(m_commandListGraphics->Reset(m_commandAllocatorGraphics.Get(), nullptr));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>          m_commandAllocatorGraphics;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>       m_commandListGraphics;

    // 1D depth buffer
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_depth1D;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_color1D;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapDsv;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapRtv;
};

static Hang1DDepth hang;
