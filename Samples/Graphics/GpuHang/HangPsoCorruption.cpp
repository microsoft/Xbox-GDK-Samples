//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Hang.h"
#include "Util.h"

class HangPsoCorruption final : public Hang
{
public:
    HangPsoCorruption() :
        Hang(
            ((1U << QueueType::Graphics) | (1U << QueueType::Async)),
#ifdef LIVE_DEBUGGING_SUPPORT
            ((1U << HangAction::Dump) | (1U << HangAction::LiveDebug) | (1U << HangAction::LiveDebugThenDump))
#else
            ((1U << HangAction::Dump))
#endif
        ),
        m_psoData(nullptr),
        m_psoAttributes{}
    {
    }

    virtual ~HangPsoCorruption() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Pso corruption";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"PsoCorruption";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if the underlying packet for a PSO is corrupted. ");
        description.push_back(L"This might happen from improper deserialization of offline PSOs, or from random memory corruption. ");
        description.push_back(L"Because the PSO contains a description of GPU state, corrupted data can have a wide range of deleterious effects. ");

        return description;
    }

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

        auto computeShaderBlob = DX::ReadData(L"CopyCs.cso");

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

        // Serialize/deserialize
        D3D12XBOX_SERIALIZE_COMPUTE_PIPELINE_STATE serializePipelineState = {};
        DX::ThrowIfFailed(device->SerializeComputePipelineStateX(m_pipelineState.Get(), D3D12XBOX_SERIALIZE_FLAGS_NONE, &serializePipelineState));

        m_pipelineState = nullptr;

        auto psoSize = DirectX::AlignUp(serializePipelineState.pPacket->GetBufferSize(), D3D12XBOX_GPU_INSTRUCTIONS_ALIGNMENT)
            + DirectX::AlignUp(serializePipelineState.pMetaData->GetBufferSize(), D3D12XBOX_GPU_INSTRUCTIONS_ALIGNMENT)
            + DirectX::AlignUp(serializePipelineState.CS.pGpuInstructions->GetBufferSize(), D3D12XBOX_GPU_INSTRUCTIONS_ALIGNMENT)
            + DirectX::AlignUp(serializePipelineState.CS.pMetaData->GetBufferSize(), D3D12XBOX_GPU_INSTRUCTIONS_ALIGNMENT);

        m_psoAttributes = MAKE_XALLOC_ATTRIBUTES(
            eXALLOCAllocatorId_GameMin,
            0,
            XALLOC_MEMTYPE_GRAPHICS_WRITECOMBINE_GPU_READONLY, 
            XALLOC_PAGESIZE_2MB,
            XALLOC_ALIGNMENT_256, 
            false);
        static_assert(1U << XALLOC_ALIGNMENT_256 == D3D12XBOX_GPU_INSTRUCTIONS_ALIGNMENT, "Incorrect alignment");
        m_psoData = XMemAlloc( 2 * psoSize, m_psoAttributes);
        if (nullptr == m_psoData)
        {
            throw std::bad_alloc();
        }
        ZeroMemory(m_psoData, 2 * psoSize);

        auto currentData = (uint8_t*)m_psoData;

        {
            // Correct serialize/deserialize steps
            D3D12XBOX_DESERIALIZE_COMPUTE_PIPELINE_STATE deserializePipelineState = {
                m_rootSignature.Get(),
            };

            auto PopulateBlob = [&currentData] (const void** dst, SIZE_T* dstSize, const void* src, SIZE_T srcSize) -> void
            {
                *dst = currentData;
                *dstSize = srcSize;
                memcpy(currentData, src, srcSize);
                currentData += srcSize;
                auto& pointerAsUint64 = reinterpret_cast<uint64_t&>(currentData);
                pointerAsUint64 = DirectX::AlignUp(pointerAsUint64, D3D12XBOX_GPU_INSTRUCTIONS_ALIGNMENT);
            };

            PopulateBlob(const_cast<const void**>(&deserializePipelineState.pPacket),
                &deserializePipelineState.PacketSize,
                serializePipelineState.pPacket->GetBufferPointer(),
                serializePipelineState.pPacket->GetBufferSize());

            PopulateBlob(const_cast<const void**>(&deserializePipelineState.pMetaData),
                &deserializePipelineState.MetaDataSize,
                serializePipelineState.pMetaData->GetBufferPointer(),
                serializePipelineState.pMetaData->GetBufferSize());

            PopulateBlob(&deserializePipelineState.CS.pGpuInstructions,
                &deserializePipelineState.CS.GpuInstructionsSize,
                serializePipelineState.CS.pGpuInstructions->GetBufferPointer(),
                serializePipelineState.CS.pGpuInstructions->GetBufferSize());

            PopulateBlob(&deserializePipelineState.CS.pMetaData,
                &deserializePipelineState.CS.MetaDataSize,
                serializePipelineState.CS.pMetaData->GetBufferPointer(),
                serializePipelineState.CS.pMetaData->GetBufferSize());

            DX::ThrowIfFailed(device->DeserializeComputePipelineStateX(&deserializePipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
        }

        // Simulate some error in serialize/deserialize
        {
            D3D12XBOX_DESERIALIZE_COMPUTE_PIPELINE_STATE deserializePipelineState =         {
                m_rootSignature.Get(),
            };

            auto PopulateBlob = [&currentData](const void** dst, SIZE_T* dstSize, const void* src, SIZE_T srcSize) -> void
            {
                *dst = currentData;
                *dstSize = srcSize;
                memcpy(currentData, src, srcSize);
                currentData +=  srcSize;
                auto& pointerAsUint64 = reinterpret_cast<uint64_t&>(currentData);
                pointerAsUint64 = DirectX::AlignUp(pointerAsUint64, D3D12XBOX_GPU_INSTRUCTIONS_ALIGNMENT);
            };

            PopulateBlob(const_cast<const void**>(&deserializePipelineState.pPacket),
                &deserializePipelineState.PacketSize,
                serializePipelineState.pPacket->GetBufferPointer(),
                serializePipelineState.pPacket->GetBufferSize());

            PopulateBlob(const_cast<const void**>(&deserializePipelineState.pMetaData),
                &deserializePipelineState.MetaDataSize,
                serializePipelineState.pMetaData->GetBufferPointer(),
                serializePipelineState.pMetaData->GetBufferSize());

            PopulateBlob(&deserializePipelineState.CS.pGpuInstructions,
                &deserializePipelineState.CS.GpuInstructionsSize,
                serializePipelineState.CS.pGpuInstructions->GetBufferPointer(),
                serializePipelineState.CS.pGpuInstructions->GetBufferSize());

            PopulateBlob(&deserializePipelineState.CS.pMetaData,
                &deserializePipelineState.CS.MetaDataSize,
                serializePipelineState.CS.pMetaData->GetBufferPointer(),
                serializePipelineState.CS.pMetaData->GetBufferSize());

            // Memory corruption
            auto packet = reinterpret_cast<uint32_t*>(deserializePipelineState.pPacket);
#ifdef _GAMING_XBOX_SCARLETT
            packet[6] = 0xabcdefff;
#else
            packet[5] = 0xabcdefff;
#endif

            // Validation will catch this via CRC check
            // ID3D12Device::DeserializeComputePipelineStateX: CRC mismatch: Contents (or sizes) of buffers differ from those provided by DeserializeComputePipelineStateX.
            SCOPED_ERROR_FILTER(device, 0x5C0D6B2D);

            DX::ThrowIfFailed(device->DeserializeComputePipelineStateX(&deserializePipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineStateCorrupted.ReleaseAndGetAddressOf())));
        }

        // Create input and output buffers
        auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        auto descBufInput = CD3DX12_RESOURCE_DESC::Buffer(m_bufSize);
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
        memset(bufData, 0xab, m_bufSize);
        m_bufInput->Unmap(0, nullptr);

        auto descBufOutput = CD3DX12_RESOURCE_DESC::Buffer(m_bufSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(&defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &descBufOutput,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_bufOutput.ReleaseAndGetAddressOf())));
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
        m_pipelineStateCorrupted = nullptr;
        XMemFree(m_psoData, m_psoAttributes);

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

        if (hang)
        {
            commandList->SetPipelineState(m_pipelineStateCorrupted.Get());
        }
        else
        {
            commandList->SetPipelineState(m_pipelineState.Get());
        }

        commandList->SetComputeRootShaderResourceView(0, m_bufInput->GetGPUVirtualAddress());
        commandList->SetComputeRootUnorderedAccessView(1, m_bufOutput->GetGPUVirtualAddress());

        auto threadgroupSize = 64U;
        auto numThreadgroups = m_bufSize / threadgroupSize / uint32_t(sizeof(uint32_t));
        commandList->Dispatch(numThreadgroups, 1, 1);

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
    Microsoft::WRL::ComPtr<ID3D12PipelineState>             m_pipelineStateCorrupted;
    void*                                                   m_psoData;
    ULONGLONG                                               m_psoAttributes;

    static const uint32_t                                   m_bufSize = 1024U;

    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufInput;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufOutput;
};

static HangPsoCorruption hang;
