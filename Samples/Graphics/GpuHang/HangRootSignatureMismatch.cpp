//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Hang.h"
#include "Util.h"

class HangRootSignatureMismatch final : public Hang
{
public:
    // This case can run on async too, but it won't hang, at least not for the same reasons
    HangRootSignatureMismatch() :
        Hang((1U << QueueType::Graphics) /*| (1U << QueueType::Async)*/)
    {
    }

    virtual ~HangRootSignatureMismatch() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Root signature mismatch";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"RootSignatureMismatch";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if a root signature is not bound, or if a root signature is bound and the title ");
        description.push_back(L"makes a SetGraphicsRoot* or SetComputeRoot* call whose RootParameterIndex is incompatible with ");
        description.push_back(L"the bound root signature. For instance, the title might call SetComputeRootDescriptorTable(0, ...) ");
        description.push_back(L"when element 0 of the root signature is not of type D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE. ");

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

        // The correct root signature, from the shader
        auto computeShaderBlob = DX::ReadData(L"CopyCs.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

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

        // The incorrect root signature, from some different shader
        auto computeShaderMismatchedBlob = DX::ReadData(L"CullGraphicsCs.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderMismatchedBlob.data(), computeShaderMismatchedBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignatureMismatched.ReleaseAndGetAddressOf())));

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
        m_commandQueueCompute = nullptr;
        m_commandAllocatorCompute = nullptr;
        m_commandListCompute = nullptr;

        m_commandAllocatorGraphics = nullptr;
        m_commandListGraphics = nullptr;

        m_rootSignature = nullptr;
        m_pipelineState = nullptr;

        m_rootSignatureMismatched = nullptr;

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
        SCOPED_ERROR_FILTER(device, 0xD7AADEE7); // (Graphics) The currently set root signature declares parameter [1] with type descriptor table, so it is invalid to set a root UAV here.
        SCOPED_ERROR_FILTER(device, 0xF4C75A06); // (Compute) The currently set root signature declares parameter [1] with type descriptor table, so it is invalid to set a root UAV here.
        
        bool async = (Async == m_queue);
        auto queue = async ? m_commandQueueCompute : commandQueue;
        auto commandAllocator = async ? m_commandAllocatorCompute : m_commandAllocatorGraphics;
        auto commandList = async ? m_commandListCompute : m_commandListGraphics;

        PIXBeginRetailEvent(commandList.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);

        auto rootSignature = hang ? m_rootSignatureMismatched : m_rootSignature;

        commandList->SetComputeRootSignature(rootSignature.Get());
        commandList->SetPipelineState(m_pipelineState.Get());

        // This root element happens to be the same for the two root signatures
        commandList->SetComputeRootShaderResourceView(0, m_bufInput->GetGPUVirtualAddress());

        // This root element happens to be different for the two root signatures
        // Now the command buffer becomes potentially unparseable
        commandList->SetComputeRootUnorderedAccessView(1, m_bufOutput->GetGPUVirtualAddress());

        // PIX events insert strings into the command buffer, likely to hang if parsing is off
        // Using retail PIX events so that the hang works in Release (but hangs could happen in Release regardless)
        PIXBeginRetailEvent(commandList.Get(), PIX_COLOR_DEFAULT, L"Random string");
        PIXEndRetailEvent(commandList.Get());

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

    Microsoft::WRL::ComPtr<ID3D12RootSignature>             m_rootSignatureMismatched;

    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufInput;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufOutput;

    static const uint32_t                                   m_bufSize = 1024U;
};

static HangRootSignatureMismatch hang;
