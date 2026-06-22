//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Hang.h"
#include "Util.h"

class HangMissingScratch final : public Hang
{
public:
    HangMissingScratch() :
        Hang(1U << QueueType::Async)
    {
    }

    virtual ~HangMissingScratch() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Missing scratch";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"MissingScratch";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if a shader uses scratch memory, but no scratch buffer was allocated. ");
        description.push_back(L"Scratch buffer allocation for the graphics queue is controlled by GraphicsScratchMemorySizeBytes. ");
        description.push_back(L"and ComputeScratchMemorySizeBytes. ");
        description.push_back(L"Scratch buffers are not currently allocated at all for async compute queues. ");
        auto processDebugFlags = D3D12XboxGetProcessDebugFlags();
        if (processDebugFlags & D3D12XBOX_PROCESS_DEBUG_FLAG_VALIDATED)
        {
            description.push_back(L"");
            description.push_back(L"THIS CASE WILL NOT HANG BECAUSE THE VALIDATED DRIVER IS IN USE!");
        }

        return description;
    }

    struct Input
    {
        uint32_t value;
    };

    struct Output
    {
        uint32_t value;
    };

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* /*commandQueue*/) override
    {
        // We use an async compute command queue, because we it doesn't have scratch memory (under the current driver).
        // We could use ComputeScratchMemorySizeBytes=0 to prevent scratch memory in the graphics queue, but that
        // would affect the whole app, and would be irreversible unless we recreated the device.
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

        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocatorGraphics.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocatorGraphics);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorGraphics.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandListGraphics.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandListGraphics);

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

        auto computeShaderBlob = DX::ReadData(L"ScratchCs.cso");

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
            // Any inputs
            Input input[256] = {};

            auto descBufInput = CD3DX12_RESOURCE_DESC::Buffer(sizeof(input));
            DX::ThrowIfFailed(
                device->CreateCommittedResource(&uploadHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &descBufInput,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_bufInput.ReleaseAndGetAddressOf())));

            // Copy the data to the buffer. This isn't really necessary, as we don't care about the buffer contents
            void* bufData = nullptr;
            CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
            DX::ThrowIfFailed(m_bufInput->Map(0, &readRange, &bufData));
            memcpy(bufData, input, sizeof(input));
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
        m_commandQueueCompute = nullptr;
        m_commandAllocatorCompute = nullptr;
        m_commandListCompute = nullptr;

        m_commandAllocatorGraphics = nullptr;
        m_commandListGraphics = nullptr;

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

    virtual void Render(ID3D12Device* device, ID3D12CommandQueue* commandQueue, bool hang) override
    {
        // On the graphics queue, scratch will work (but scratch usage is highly undesirable).
        // On the compute queue, scratch will fail.
        auto queue = hang ? m_commandQueueCompute : commandQueue;
        auto commandAllocator = hang ? m_commandAllocatorCompute : m_commandAllocatorGraphics;
        auto commandList = hang ? m_commandListCompute : m_commandListGraphics;

        SCOPED_ERROR_FILTER(device, 0x335CA89D);    // Driver does not yet support compute shaders utilizing register spill to scratch memory.

        PIXBeginRetailEvent(commandList.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);

        commandList->SetComputeRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_pipelineState.Get());

        commandList->SetComputeRootShaderResourceView(0, m_bufInput->GetGPUVirtualAddress());
        commandList->SetComputeRootUnorderedAccessView(1, m_bufOutput->GetGPUVirtualAddress());

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

static HangMissingScratch hang;
