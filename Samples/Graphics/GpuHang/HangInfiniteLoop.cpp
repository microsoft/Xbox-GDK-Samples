//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Hang.h"
#include "Util.h"

class HangInfiniteLoop final : public Hang
{
public:
    HangInfiniteLoop() :
        Hang(
            ((1U << QueueType::Graphics) | (1U << QueueType::Async)),
#ifdef LIVE_DEBUGGING_SUPPORT
            ((1U << HangAction::Dump) | (1U << HangAction::LiveDebug) | (1U << HangAction::LiveDebugThenDump))
#else
            ((1U << HangAction::Dump))
#endif
        )
    {
    }

    virtual ~HangInfiniteLoop() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Infinite loop";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"InfiniteLoop";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if a shader runs for multiple seconds. ");
        description.push_back(L"A typical cause is a dynamic loop within the shader, where the loop count comes from a buffer. ");
        description.push_back(L"If the buffer is not correctly populated, either by the CPU or by previous GPU work, then the loop count might be very large. ");

        return description;
    }

    struct Node
    {
        uint32_t next;
        uint32_t value;
    };

    struct Output
    {
        uint32_t value;
    };

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* /*commandQueue*/) override
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocatorGraphics.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocatorGraphics);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorGraphics.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandListGraphics.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandListGraphics);

        D3D12_COMMAND_QUEUE_DESC descCommandQueue =
        {
            D3D12_COMMAND_LIST_TYPE_COMPUTE,                    // D3D12_COMMAND_LIST_TYPE Type;
            0,                                                  // INT Priority;
            D3D12_COMMAND_QUEUE_FLAG_NONE,                      // D3D12_COMMAND_QUEUE_FLAGS Flags;
            0,                                                  // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateCommandQueue(&descCommandQueue, IID_GRAPHICS_PPV_ARGS(m_commandQueueCompute.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandQueueCompute);

        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_GRAPHICS_PPV_ARGS(m_commandAllocatorCompute.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocatorCompute);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_commandAllocatorCompute.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandListCompute.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandListCompute);

        CD3DX12_ROOT_PARAMETER rootParams[2] = {};
        rootParams[0].InitAsShaderResourceView(0);
        rootParams[1].InitAsUnorderedAccessView(0);

        auto descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC(_countof(rootParams), rootParams);
 
        ID3DBlob* serializedRootSignature = nullptr;
        ID3DBlob* errorBlob = nullptr;
        DX::ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSignature, &errorBlob));
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            serializedRootSignature->GetBufferPointer(), 
            serializedRootSignature->GetBufferSize(), 
            IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

        auto computeShaderBlob = DX::ReadData(L"InfiniteLoopCs.cso");

        D3D12_COMPUTE_PIPELINE_STATE_DESC descPipelineState =
        {
            m_rootSignature.Get(),                              // ID3D12RootSignature* pRootSignature;
            {
                computeShaderBlob.data(),
                computeShaderBlob.size(),
            },                                                  // D3D12_SHADER_BYTECODE CS;
            0,                                                  // UINT NodeMask;
            { nullptr, 0, },                                    // D3D12_CACHED_PIPELINE_STATE CachedPSO;
        };
        DX::ThrowIfFailed(device->CreateComputePipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));

        // Create input and output buffers
        auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        {
            // A proper linked list, ending at node 3
            Node linkedList[] = 
            {
                { 5, 0, },
                { 4, 1, },
                { 7, 2, },
                { 0xffffffff, 3, },
                { 3, 4, },
                { 2, 5, },
                { 1, 6, },
                { 6, 7, },
            };

            auto descBufInput = CD3DX12_RESOURCE_DESC::Buffer(sizeof(linkedList));
            DX::ThrowIfFailed(
                device->CreateCommittedResource(&uploadHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &descBufInput,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_bufInput.ReleaseAndGetAddressOf())));

            // Copy the linked list data to the buffer.
            void* bufData = nullptr;
            CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
            DX::ThrowIfFailed(m_bufInput->Map(0, &readRange, &bufData));
            memcpy(bufData, linkedList, sizeof(linkedList));
            m_bufInput->Unmap(0, nullptr);

            auto descBufOutput = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Output), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            DX::ThrowIfFailed(
                device->CreateCommittedResource(&defaultHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &descBufOutput,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_bufOutput.ReleaseAndGetAddressOf())));
        }
    }

    virtual void Uninitialize(ID3D12Device* /* device */) override
    {
        m_commandAllocatorGraphics = nullptr;
        m_commandListGraphics = nullptr;

        m_commandQueueCompute = nullptr;
        m_commandAllocatorCompute = nullptr;
        m_commandListCompute = nullptr;

        m_rootSignature = nullptr;
        m_pipelineState = nullptr;

        m_bufInput = nullptr;
        m_bufOutput = nullptr;
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        GpuFullStop(device, commandQueue);
        GpuFullStop(device, m_commandQueueCompute.Get());
    }

    virtual void Render(ID3D12Device* /* device */, ID3D12CommandQueue* commandQueue, bool hang) override
    {
        bool async = (Async == m_queue);
        auto queue = async ? m_commandQueueCompute : commandQueue;
        auto commandAllocator = async ? m_commandAllocatorCompute : m_commandAllocatorGraphics;
        auto commandList = async ? m_commandListCompute : m_commandListGraphics;

        PIXBeginRetailEvent(commandList.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);

        commandList->SetComputeRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_pipelineState.Get());

        commandList->SetComputeRootShaderResourceView(0, m_bufInput->GetGPUVirtualAddress());
        commandList->SetComputeRootUnorderedAccessView(1, m_bufOutput->GetGPUVirtualAddress());

        if (hang)
        {
            // Now mess up the linked list, so that traversal never terminates
            void* bufData = nullptr;
            DX::ThrowIfFailed(m_bufInput->Map(0, nullptr, &bufData));
            reinterpret_cast<Node*>(bufData)[3].next = 0;
            m_bufInput->Unmap(0, nullptr);
        }

        commandList->Dispatch(1, 1, 1);

        PIXEndRetailEvent(commandList.Get());

        DX::ThrowIfFailed(commandList->Close());
        queue->ExecuteCommandLists(1, CommandListCast(commandList.GetAddressOf()));

        DX::ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
    }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>              m_commandQueueCompute;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>          m_commandAllocatorCompute;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>       m_commandListCompute;

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>          m_commandAllocatorGraphics;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>       m_commandListGraphics;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>             m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>             m_pipelineState;

    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufInput;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufOutput;
};

static HangInfiniteLoop hang;
