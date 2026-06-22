//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "Hang.h"
#include "Util.h"

class HangFenceDeadlock final : public Hang
{
public:
    HangFenceDeadlock() :
        Hang((1U << QueueType::Graphics) | (1U << QueueType::Async) | (1U << QueueType::Dma))
    {
    }

    virtual ~HangFenceDeadlock() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Fence deadlock";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"FenceDeadlock";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if the title submits multiple fences with a circular dependency. ");
        description.push_back(L"It can also occur with extension APIs such as Wait32BitValueX/Write32BitValue[*]X. ");

        return description;
    }

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* /* commandQueue */) override
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocator);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandList);

        D3D12_COMMAND_QUEUE_DESC descCommandQueueCompute =
        {
            D3D12_COMMAND_LIST_TYPE_COMPUTE,                    // D3D12_COMMAND_LIST_TYPE Type;
            0,                                                  // INT Priority;
            D3D12_COMMAND_QUEUE_FLAG_NONE,                      // D3D12_COMMAND_QUEUE_FLAGS Flags;
            0,                                                  // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateCommandQueue(&descCommandQueueCompute, IID_GRAPHICS_PPV_ARGS(m_commandQueueCompute.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandQueueCompute);

        D3D12_COMMAND_QUEUE_DESC descCommandQueueDma =
        {
            D3D12XBOX_COMMAND_LIST_TYPE_DMA,                    // D3D12_COMMAND_LIST_TYPE Type;
            0,                                                  // INT Priority;
            D3D12_COMMAND_QUEUE_FLAG_NONE,                      // D3D12_COMMAND_QUEUE_FLAGS Flags;
            0,                                                  // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateCommandQueue(&descCommandQueueDma, IID_GRAPHICS_PPV_ARGS(m_commandQueueDma.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandQueueDma);

        DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_fenceGraphicsToCompute.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_fenceGraphicsToCompute);

        DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_fenceComputeToDma.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_fenceComputeToDma);

        DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_fenceDmaToGraphics.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_fenceDmaToGraphics);
    }

    virtual void Uninitialize(ID3D12Device* /* device */) override
    {
        m_commandList = nullptr;
        m_commandAllocator = nullptr;

        m_commandQueueCompute = nullptr;
        m_commandQueueDma = nullptr;

        m_fenceGraphicsToCompute = nullptr;
        m_fenceComputeToDma = nullptr;
        m_fenceDmaToGraphics = nullptr;
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        // Remember to check all queues! Otherwise we just sit there like idiots.
        GpuFullStop(device, commandQueue);
        GpuFullStop(device, m_commandQueueCompute.Get());
        GpuFullStop(device, m_commandQueueDma.Get());
    }

    virtual void Render(ID3D12Device* device, ID3D12CommandQueue* commandQueue, bool hang) override
    {
        PIXBeginRetailEvent(commandQueue, PIX_COLOR_DEFAULT, __FUNCTIONW__);
        PIXBeginRetailEvent(m_commandQueueCompute.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);
        PIXBeginRetailEvent(m_commandQueueDma.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);

        for (auto value = 1U; value < 10U; ++value)
        {
            auto hangDma = hang && (Dma == m_queue) && (value >= 3U);
            auto hangGraphics = hang && (Graphics == m_queue) && (value >= 4U);
            auto hangAsync = hang && (Async == m_queue) && (value >= 5U);

            // Submit a circular dependency
            if (!hangAsync) commandQueue->Signal(m_fenceGraphicsToCompute.Get(), value);
            if (!hangDma) m_commandQueueCompute->Signal(m_fenceComputeToDma.Get(), value);
            if (!hangGraphics) m_commandQueueDma->Signal(m_fenceDmaToGraphics.Get(), value);

            m_commandQueueDma->Wait(m_fenceComputeToDma.Get(), value);
            commandQueue->Wait(m_fenceDmaToGraphics.Get(), value);
            m_commandQueueCompute->Wait(m_fenceGraphicsToCompute.Get(), value);
        }

        PIXEndRetailEvent(commandQueue);
        PIXEndRetailEvent(m_commandQueueCompute.Get());
        PIXEndRetailEvent(m_commandQueueDma.Get());

        // Workaround for driver crash which occurs if a fence is deleted before the next ExecuteCommandLists call
        SCOPED_ERROR_FILTER(device, 0x34F2B93F); // Command buffer validation detected an unsatisfied fence wait queued on this command queue
        DX::ThrowIfFailed(m_commandList->Close());
        commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>          m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>       m_commandList;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue>              m_commandQueueCompute;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>              m_commandQueueDma;

    Microsoft::WRL::ComPtr<ID3D12Fence>                     m_fenceGraphicsToCompute;   
    Microsoft::WRL::ComPtr<ID3D12Fence>                     m_fenceComputeToDma;   
    Microsoft::WRL::ComPtr<ID3D12Fence>                     m_fenceDmaToGraphics;   
};

static HangFenceDeadlock hang;
