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
        Hang((1U << QueueType::Graphics) | (1U << QueueType::Async) | (1U << QueueType::Copy))
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

        return description;
    }

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* /* commandQueue */) override
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocator);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandList);

        D3D12_COMMAND_QUEUE_DESC descCommandQueueCompute =
        {
            D3D12_COMMAND_LIST_TYPE_COMPUTE,                    // D3D12_COMMAND_LIST_TYPE Type;
            0,                                                  // INT Priority;
            D3D12_COMMAND_QUEUE_FLAG_NONE,                      // D3D12_COMMAND_QUEUE_FLAGS Flags;
            0,                                                  // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateCommandQueue(&descCommandQueueCompute, IID_PPV_ARGS(m_commandQueueCompute.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandQueueCompute);

        D3D12_COMMAND_QUEUE_DESC descCommandQueueCopy =
        {
            D3D12_COMMAND_LIST_TYPE_COPY,                       // D3D12_COMMAND_LIST_TYPE Type;
            0,                                                  // INT Priority;
            D3D12_COMMAND_QUEUE_FLAG_NONE,                      // D3D12_COMMAND_QUEUE_FLAGS Flags;
            0,                                                  // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateCommandQueue(&descCommandQueueCopy, IID_PPV_ARGS(m_commandQueueCopy.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandQueueCopy);

        DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fenceGraphicsToCompute.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_fenceGraphicsToCompute);

        DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fenceComputeToCopy.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_fenceComputeToCopy);

        DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fenceCopyToGraphics.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_fenceCopyToGraphics);
    }

    virtual void Uninitialize(ID3D12Device* /* device */) override
    {
        m_commandList = nullptr;
        m_commandAllocator = nullptr;

        m_commandQueueCompute = nullptr;
        m_commandQueueCopy = nullptr;

        m_fenceGraphicsToCompute = nullptr;
        m_fenceComputeToCopy = nullptr;
        m_fenceCopyToGraphics = nullptr;
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        // Remember to check all queues! Otherwise we just sit there like idiots.
        GpuFullStop(device, commandQueue);
        GpuFullStop(device, m_commandQueueCompute.Get());
        GpuFullStop(device, m_commandQueueCopy.Get());
    }

    virtual void Render(ID3D12Device* /*device*/, ID3D12CommandQueue* commandQueue, UINT /*vendorId*/, bool hang) override
    {
        PIXBeginRetailEvent(commandQueue, PIX_COLOR_DEFAULT, __FUNCTIONW__);
        PIXBeginRetailEvent(m_commandQueueCompute.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);
        PIXBeginRetailEvent(m_commandQueueCopy.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);

        for (auto value = 1U; value < 10U; ++value)
        {
            auto hangCopy = hang && (Copy == m_queue) && (value >= 3U);
            auto hangGraphics = hang && (Graphics == m_queue) && (value >= 4U);
            auto hangAsync = hang && (Async == m_queue) && (value >= 5U);

            // Submit a circular dependency
            if (!hangAsync) commandQueue->Signal(m_fenceGraphicsToCompute.Get(), value);
            if (!hangCopy) m_commandQueueCompute->Signal(m_fenceComputeToCopy.Get(), value);
            if (!hangGraphics) m_commandQueueCopy->Signal(m_fenceCopyToGraphics.Get(), value);

            m_commandQueueCopy->Wait(m_fenceComputeToCopy.Get(), value);
            commandQueue->Wait(m_fenceCopyToGraphics.Get(), value);
            m_commandQueueCompute->Wait(m_fenceGraphicsToCompute.Get(), value);
        }

        PIXEndRetailEvent(commandQueue);
        PIXEndRetailEvent(m_commandQueueCompute.Get());
        PIXEndRetailEvent(m_commandQueueCopy.Get());

        DX::ThrowIfFailed(m_commandList->Close());
        commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>          m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>       m_commandList;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue>              m_commandQueueCompute;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>              m_commandQueueCopy;

    Microsoft::WRL::ComPtr<ID3D12Fence>                     m_fenceGraphicsToCompute;   
    Microsoft::WRL::ComPtr<ID3D12Fence>                     m_fenceComputeToCopy;   
    Microsoft::WRL::ComPtr<ID3D12Fence>                     m_fenceCopyToGraphics;   
};

static HangFenceDeadlock hang;
