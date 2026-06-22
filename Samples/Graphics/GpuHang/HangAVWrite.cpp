//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Hang.h"
#include "Util.h"

class HangAVWrite final : public Hang
{
public:
    // This case can run on async too, but it won't hang, at least not for the same reasons
    HangAVWrite() :
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

    virtual ~HangAVWrite() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"AV on write";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"AVWrite";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if a shader attempts to write to a resource which was allocated in read-only memory.");
        auto processDebugFlags = D3D12XboxGetProcessDebugFlags();
        if (!(processDebugFlags & (D3D12XBOX_PROCESS_DEBUG_FLAG_VALIDATED | D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED)))
        {
            description.push_back(L"");
            description.push_back(L"THIS CASE WILL NOT HANG BECAUSE THE RELEASE DRIVER IS IN USE!");
        }
        if (!IsDebuggerPresent())
        {
            description.push_back(L"");
            description.push_back(L"THIS CASE WILL NOT HANG UNLESS PIX OR A DEBUGGER WAS ATTACHED AT STARTUP!");
        }
#ifndef HIX_EXCEPTION_SUPPORT
        {
            description.push_back(L"");
            description.push_back(L"THIS CASE WILL NOT HANG BECAUSE IT REQUIRES THE MARCH 2023 GDK!");
        }
#endif

        return description;
    }


    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* /* commandQueue */) override
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
                IID_GRAPHICS_PPV_ARGS(m_bufOutputReadWrite.ReleaseAndGetAddressOf())));

        // We could create an AV on write in two different ways:
        //     (1) Allocate read-only memory for a resource created with D3D12_RESOURCE_STATE_UNORDERED_ACCESS.
        //     (2) Allocate read-only memory for a resource created without D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        //         but then transition that resource to D3D12_RESOURCE_STATE_UNORDERED_ACCESS.
        // We choose (1) for now.
        auto AllocationType = DWORD(MEM_64K_PAGES | MEM_RESERVE | MEM_COMMIT);
        auto xMemAllocationFlags = DWORD(XMEM_GRAPHICS);
        auto pageProtect = DWORD(PAGE_READWRITE | PAGE_GRAPHICS_READONLY);
        auto addressReadOnly = XMemVirtualAlloc(nullptr, m_bufSize, AllocationType, xMemAllocationFlags, pageProtect);
        assert(nullptr != addressReadOnly);
        DX::ThrowIfFailed(
            device->CreatePlacedResourceX(D3D12_GPU_VIRTUAL_ADDRESS(addressReadOnly),
                &descBufOutput,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_bufOutputReadOnly.ReleaseAndGetAddressOf())));
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
        m_bufOutputReadWrite = nullptr;
        m_bufOutputReadOnly = nullptr;
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        GpuFullStop(device, commandQueue);
        GpuFullStop(device, m_commandQueueCompute.Get());
    }

    virtual void Render(ID3D12Device* /*device*/, ID3D12CommandQueue* commandQueue, bool hang) override
    {
        bool async = (Async == m_queue);
        auto queue = async ? m_commandQueueCompute : commandQueue;
        auto commandAllocator = async ? m_commandAllocatorCompute : m_commandAllocatorGraphics;
        auto commandList = async ? m_commandListCompute : m_commandListGraphics;

        PIXBeginRetailEvent(commandList.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);

        auto bufOutput = hang ? m_bufOutputReadOnly : m_bufOutputReadWrite;

        commandList->SetComputeRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_pipelineState.Get());

        commandList->SetComputeRootShaderResourceView(0, m_bufInput->GetGPUVirtualAddress());
        commandList->SetComputeRootUnorderedAccessView(1, bufOutput->GetGPUVirtualAddress());

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
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufOutputReadWrite;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufOutputReadOnly;

    static const uint32_t                                   m_bufSize = 1024U;
};

static HangAVWrite hang;
