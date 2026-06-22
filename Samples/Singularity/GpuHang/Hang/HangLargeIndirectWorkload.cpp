//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Hang.h"
#include "Util.h"

class HangLargeIndirectWorkload final : public Hang
{
public:
    HangLargeIndirectWorkload() :
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

    virtual ~HangLargeIndirectWorkload() override {}

    virtual const wchar_t* GetName() const override
    {
        return L"Large indirect workload";
    }

    virtual const wchar_t* GetFileName() const override
    {
        return L"LargeIndirectWorkload";
    }

    virtual const std::vector<const wchar_t*> GetDescription() const override
    {
        std::vector<const wchar_t*> description;

        description.push_back(L"This hang occurs if the title accidentally submits a workload which takes several seconds. ");
        description.push_back(L"One possible scenario is bad indirect arguments, which cause an extremely large draw or dispatch. ");
        description.push_back(L"Bad indirect arguments can arise from calculation errors, from failure to initialize, or from improper synchronization. ");

        return description;
    }

    virtual void Initialize(ID3D12Device* device, ID3D12CommandQueue* /* commandQueue */) override
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocatorGraphics.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocatorGraphics);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorGraphics.Get(), nullptr, IID_PPV_ARGS(m_commandListGraphics.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandListGraphics);

        D3D12_COMMAND_QUEUE_DESC descCommandQueue =
        {
            D3D12_COMMAND_LIST_TYPE_COMPUTE,                    // D3D12_COMMAND_LIST_TYPE Type;
            0,                                                  // INT Priority;
            D3D12_COMMAND_QUEUE_FLAG_NONE,                      // D3D12_COMMAND_QUEUE_FLAGS Flags;
            0,                                                  // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateCommandQueue(&descCommandQueue, IID_PPV_ARGS(m_commandQueueCompute.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandQueueCompute);

        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(m_commandAllocatorCompute.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocatorCompute);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_commandAllocatorCompute.Get(), nullptr, IID_PPV_ARGS(m_commandListCompute.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandListCompute);

        // Culling pass 
        {
            CD3DX12_DESCRIPTOR_RANGE descRange = {};
            descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

            CD3DX12_ROOT_PARAMETER rootParams[2];
            rootParams[0].InitAsShaderResourceView(0);
            rootParams[1].InitAsDescriptorTable(1, &descRange);

            auto descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC(_countof(rootParams), rootParams);

            ID3DBlob* serializedRootSignature = nullptr;
            ID3DBlob* errorBlob = nullptr;
            DX::ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSignature, &errorBlob));
            DX::ThrowIfFailed(device->CreateRootSignature(0,
                serializedRootSignature->GetBufferPointer(),
                serializedRootSignature->GetBufferSize(),
                IID_PPV_ARGS(m_rootSignatureCull.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_rootSignatureCull);

            auto computeShaderBlob = DX::ReadData(L"CullGraphicsCs.cso");

            D3D12_COMPUTE_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureCull.Get(),                          // ID3D12RootSignature* pRootSignature;
                {
                    computeShaderBlob.data(),
                    computeShaderBlob.size(),
                },                                                  // D3D12_SHADER_BYTECODE CS;
                0,                                                  // UINT NodeMask;
                { nullptr, 0, },                                    // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            };
            DX::ThrowIfFailed(device->CreateComputePipelineState(&descPipelineState, IID_PPV_ARGS(m_pipelineStateCullGraphics.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineStateCullGraphics);
        }

        // Culling pass 
        {
            CD3DX12_DESCRIPTOR_RANGE descRange = {};
            descRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

            CD3DX12_ROOT_PARAMETER rootParams[2];
            rootParams[0].InitAsShaderResourceView(0);
            rootParams[1].InitAsDescriptorTable(1, &descRange);

            auto descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC(_countof(rootParams), rootParams);

            ID3DBlob* serializedRootSignature = nullptr;
            ID3DBlob* errorBlob = nullptr;
            DX::ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSignature, &errorBlob));
            DX::ThrowIfFailed(device->CreateRootSignature(0,
                serializedRootSignature->GetBufferPointer(),
                serializedRootSignature->GetBufferSize(),
                IID_PPV_ARGS(m_rootSignatureCull.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_rootSignatureCull);

            auto computeShaderBlob = DX::ReadData(L"CullAsyncCs.cso");

            D3D12_COMPUTE_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureCull.Get(),                          // ID3D12RootSignature* pRootSignature;
                {
                    computeShaderBlob.data(),
                    computeShaderBlob.size(),
                },                                                  // D3D12_SHADER_BYTECODE CS;
                0,                                                  // UINT NodeMask;
                { nullptr, 0, },                                    // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            };
            DX::ThrowIfFailed(device->CreateComputePipelineState(&descPipelineState, IID_PPV_ARGS(m_pipelineStateCullCompute.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineStateCullCompute);
        }

        // Transform pass 
        {
            CD3DX12_ROOT_PARAMETER rootParams[4];
            rootParams[0].InitAsConstantBufferView(0);
            rootParams[1].InitAsShaderResourceView(0);
            rootParams[2].InitAsUnorderedAccessView(0);
            rootParams[3].InitAsConstants(1, 1);

            auto descRootSignature = CD3DX12_ROOT_SIGNATURE_DESC(_countof(rootParams), rootParams);

            ID3DBlob* serializedRootSignature = nullptr;
            ID3DBlob* errorBlob = nullptr;
            DX::ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSignature, &errorBlob));
            DX::ThrowIfFailed(device->CreateRootSignature(0, 
                serializedRootSignature->GetBufferPointer(), 
                serializedRootSignature->GetBufferSize(), 
                IID_PPV_ARGS(m_rootSignatureTransform.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_rootSignatureTransform);

            auto computeShaderBlob = DX::ReadData(L"TransformCs.cso");

            D3D12_COMPUTE_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureTransform.Get(),                     // ID3D12RootSignature* pRootSignature;
                {
                    computeShaderBlob.data(),
                    computeShaderBlob.size(),
                },                                                  // D3D12_SHADER_BYTECODE CS;
                0,                                                  // UINT NodeMask;
                { nullptr, 0, },                                    // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            };
            DX::ThrowIfFailed(device->CreateComputePipelineState(&descPipelineState, IID_PPV_ARGS(m_pipelineStateTransform.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineStateTransform);
        }

        struct D3D12_SET_COMPUTE_ROOT_32BIT_CONSTANT_ARGS
        {
            UINT SrcData;
        };
        struct IndirectArgs
        {
            D3D12_SET_COMPUTE_ROOT_32BIT_CONSTANT_ARGS          m_setComputeRoot32BitConstantArgs;
            D3D12_DISPATCH_ARGUMENTS                            m_dispatchArgs;
        };
        typedef uint32_t IndirectCount;

        // Command signature graphics
        {
            // Create command signature with SetGraphicsRoot32BitConstant followed by Dispatch
            D3D12_INDIRECT_ARGUMENT_DESC descIndirectArgument[2] = {};
            descIndirectArgument[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
            descIndirectArgument[0].Constant.RootParameterIndex = 3;
            descIndirectArgument[0].Constant.Num32BitValuesToSet = 1;
            descIndirectArgument[0].Constant.DestOffsetIn32BitValues = 0;
            descIndirectArgument[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

            D3D12_COMMAND_SIGNATURE_DESC descCommandSignature =
            {
                sizeof(IndirectArgs),                               // UINT ByteStride; 
                _countof(descIndirectArgument),                     // UINT NumArgumentDescs;
                descIndirectArgument,                               // const D3D12_INDIRECT_ARGUMENT_DESC* pArgumentDescs;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateCommandSignature(&descCommandSignature, m_rootSignatureTransform.Get(), IID_PPV_ARGS(m_commandSignatureGraphics.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_commandSignatureGraphics);
        }

        // Command signature compute
        {
            D3D12_INDIRECT_ARGUMENT_DESC descIndirectArgument[1] = {};
            descIndirectArgument[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

            D3D12_COMMAND_SIGNATURE_DESC descCommandSignature =
            {
                sizeof(IndirectArgs),                               // UINT ByteStride; 
                _countof(descIndirectArgument),                     // UINT NumArgumentDescs;
                descIndirectArgument,                               // const D3D12_INDIRECT_ARGUMENT_DESC* pArgumentDescs;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateCommandSignature(&descCommandSignature, nullptr, IID_PPV_ARGS(m_commandSignatureCompute.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_commandSignatureCompute);
        }

        const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );
        const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT );

        // Indirect args buffers
        {
            // Initialize with indirect args assuming all objects are not culled
            auto indirectArgs = std::make_unique<IndirectArgs[]>(m_objectCount);
            memset(indirectArgs.get(), 0, m_objectCount * sizeof(IndirectArgs));
            for (auto i = 0U; i < m_objectCount; ++i)
            {
                auto& args = indirectArgs[i];
                args.m_dispatchArgs.ThreadGroupCountX = m_dataCountPerObject / 64;
                args.m_dispatchArgs.ThreadGroupCountY =
                    args.m_dispatchArgs.ThreadGroupCountZ = 1;

                // See "Object" and "Data" below.
                // Offset in "Data" elements within "Object" to the data for this object.
                args.m_setComputeRoot32BitConstantArgs.SrcData = i * m_dataCountPerObject;
            }

            auto descBufArgIn = CD3DX12_RESOURCE_DESC::Buffer(m_objectCount * sizeof(IndirectArgs));
            DX::ThrowIfFailed(
                device->CreateCommittedResource(&uploadHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &descBufArgIn,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(m_bufArgIn.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_bufArgIn);

            CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.

            // Copy the data to the buffer.
            void* bufData = nullptr;
            DX::ThrowIfFailed(m_bufArgIn->Map(0, &readRange, &bufData));
            memcpy(bufData, indirectArgs.get(), m_objectCount * sizeof(IndirectArgs));
            m_bufArgIn->Unmap(0, nullptr);

            auto descBufArgOut = CD3DX12_RESOURCE_DESC::Buffer(m_objectCount * sizeof(IndirectArgs), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            DX::ThrowIfFailed(
                device->CreateCommittedResource(&defaultHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &descBufArgOut,
                    D3D12_RESOURCE_STATE_COMMON,
                    nullptr,
                    IID_PPV_ARGS(m_bufArgOut.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_bufArgOut);

            auto descBufCountIn = CD3DX12_RESOURCE_DESC::Buffer(sizeof(IndirectCount));
            DX::ThrowIfFailed(
                device->CreateCommittedResource(&uploadHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &descBufCountIn,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(m_bufCountIn.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_bufCountIn);

            // Initialize the buffer to zero.
            void* bufCountData = nullptr;
            DX::ThrowIfFailed(m_bufCountIn->Map(0, &readRange, &bufCountData));
            reinterpret_cast<IndirectCount*>(bufCountData)[0] = 0;
            m_bufCountIn->Unmap(0, nullptr);

            auto descBufCountOut = CD3DX12_RESOURCE_DESC::Buffer(sizeof(IndirectCount), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            DX::ThrowIfFailed(
                device->CreateCommittedResource(&defaultHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &descBufCountOut,
                    D3D12_RESOURCE_STATE_COMMON,
                    nullptr,
                    IID_PPV_ARGS(m_bufCountOut.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_bufCountOut);
        }

        // Transform buffers
        {
            DirectX::XMMATRIX transform = DirectX::XMMatrixIdentity();

            auto descBufTransform = CD3DX12_RESOURCE_DESC::Buffer(sizeof(transform));
            DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &descBufTransform,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(m_bufTransform.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_bufTransform);

            CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.

            // Copy the data to the buffer.
            void* bufTransformData = nullptr;
            DX::ThrowIfFailed(m_bufTransform->Map(0, &readRange, &bufTransformData));
            memcpy(bufTransformData, &transform, sizeof(transform));
            m_bufTransform->Unmap(0, nullptr);

            struct Data
            {
                DirectX::XMVECTOR position;
            };
            typedef Data Object[m_dataCountPerObject];

            auto objects = new Object[m_objectCount];
            ZeroMemory(objects, m_objectCount * sizeof(Object));

            auto descBufIn = CD3DX12_RESOURCE_DESC::Buffer(m_objectCount * sizeof(Object));
            DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &descBufIn,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(m_bufIn.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_bufIn);

            // Copy the data to the buffer.
            void* bufInData = nullptr;
            DX::ThrowIfFailed(m_bufIn->Map(0, &readRange, &bufInData));
            memcpy(bufInData, objects, m_objectCount * sizeof(Object));
            m_bufIn->Unmap(0, nullptr);

            delete [] objects;

            auto descBufOut = CD3DX12_RESOURCE_DESC::Buffer(m_objectCount * sizeof(Object), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &descBufOut,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(m_bufOut.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_bufOut);

        }

        // Create descriptor heap
        {
            D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc = {};
            uavHeapDesc.NumDescriptors = 2;
            uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            DX::ThrowIfFailed(
                device->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(m_descriptorHeapUav.ReleaseAndGetAddressOf())));

            auto uavHeapDescNsv = uavHeapDesc;
            uavHeapDescNsv.Flags &= ~D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            DX::ThrowIfFailed(
                device->CreateDescriptorHeap(&uavHeapDescNsv, IID_PPV_ARGS(m_descriptorHeapUavClearNsv.ReleaseAndGetAddressOf())));

            auto uavHeapDescSv = uavHeapDesc;
            uavHeapDescSv.Flags |= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            DX::ThrowIfFailed(
                device->CreateDescriptorHeap(&uavHeapDescSv, IID_PPV_ARGS(m_descriptorHeapUavClearSv.ReleaseAndGetAddressOf())));

            auto hCpuDescriptorUavHeapStart = m_descriptorHeapUav->GetCPUDescriptorHandleForHeapStart();
            auto hCpuDescriptorUavHeapStartNsv = m_descriptorHeapUavClearNsv->GetCPUDescriptorHandleForHeapStart();
            auto hCpuDescriptorUavHeapStartSv = m_descriptorHeapUavClearSv->GetCPUDescriptorHandleForHeapStart();
            auto hGpuDescriptorUavHeapStart = m_descriptorHeapUav->GetGPUDescriptorHandleForHeapStart();
            auto hGpuDescriptorUavHeapStartSv = m_descriptorHeapUavClearSv->GetGPUDescriptorHandleForHeapStart();
            auto handleIncrementSize = device->GetDescriptorHandleIncrementSize(uavHeapDesc.Type);

            auto descriptorIndex = 0U;

            m_hCpuDescriptorUavArgOut = CD3DX12_CPU_DESCRIPTOR_HANDLE(hCpuDescriptorUavHeapStart, int(descriptorIndex), handleIncrementSize);
            m_hCpuDescriptorUavArgOutClearNsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(hCpuDescriptorUavHeapStartNsv, int(descriptorIndex), handleIncrementSize);
            m_hGpuDescriptorUavArgOut = CD3DX12_GPU_DESCRIPTOR_HANDLE(hGpuDescriptorUavHeapStart, int(descriptorIndex), handleIncrementSize);
            m_hGpuDescriptorUavArgOutClearSv = CD3DX12_GPU_DESCRIPTOR_HANDLE(hGpuDescriptorUavHeapStartSv, int(descriptorIndex), handleIncrementSize);
            auto hCpuDescriptorUavArgOutClearSv = CD3DX12_CPU_DESCRIPTOR_HANDLE(hCpuDescriptorUavHeapStartSv, int(descriptorIndex), handleIncrementSize);
            ++descriptorIndex;
            m_hCpuDescriptorUavCountOut = CD3DX12_CPU_DESCRIPTOR_HANDLE(hCpuDescriptorUavHeapStart, int(descriptorIndex), handleIncrementSize);
            m_hCpuDescriptorUavCountOutClearNsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(hCpuDescriptorUavHeapStartNsv, int(descriptorIndex), handleIncrementSize);
            m_hGpuDescriptorUavCountOut = CD3DX12_GPU_DESCRIPTOR_HANDLE(hGpuDescriptorUavHeapStart, int(descriptorIndex), handleIncrementSize);
            m_hGpuDescriptorUavCountOutClearSv = CD3DX12_GPU_DESCRIPTOR_HANDLE(hGpuDescriptorUavHeapStartSv, int(descriptorIndex), handleIncrementSize);
            auto hCpuDescriptorUavCountOutClearSv = CD3DX12_CPU_DESCRIPTOR_HANDLE(hCpuDescriptorUavHeapStartSv, int(descriptorIndex), handleIncrementSize);
            ++descriptorIndex;

            assert(descriptorIndex <= uavHeapDesc.NumDescriptors);

            D3D12_UNORDERED_ACCESS_VIEW_DESC descUavArg =
            {
                DXGI_FORMAT_UNKNOWN,                                    // DXGI_FORMAT Format;
                D3D12_UAV_DIMENSION_BUFFER,                             // D3D12_UAV_DIMENSION ViewDimension;
                {
                    0,                                                  // UINT64 FirstElement;
                    m_objectCount,                                      // UINT NumElements;
                    sizeof(IndirectArgs),                               // UINT StructureByteStride;
                    0,                                                  // UINT64 CounterOffsetInBytes;
                    D3D12_BUFFER_UAV_FLAG_NONE,                         // D3D12_BUFFER_UAV_FLAGS Flags;
                },                                                      // D3D12_BUFFER_UAV Buffer;
            };
            device->CreateUnorderedAccessView(m_bufArgOut.Get(), m_bufCountOut.Get(), &descUavArg, m_hCpuDescriptorUavArgOut);

            D3D12_UNORDERED_ACCESS_VIEW_DESC descUavArgClear =
            {
                DXGI_FORMAT_R32_UINT,                                   // DXGI_FORMAT Format;
                D3D12_UAV_DIMENSION_BUFFER,                             // D3D12_UAV_DIMENSION ViewDimension;
                {
                    0,                                                  // UINT64 FirstElement;
                    m_objectCount * sizeof(IndirectArgs) / sizeof(UINT),// UINT NumElements;
                    0,                                                  // UINT StructureByteStride;
                    0,                                                  // UINT64 CounterOffsetInBytes;
                    D3D12_BUFFER_UAV_FLAG_NONE,                         // D3D12_BUFFER_UAV_FLAGS Flags;
                },                                                      // D3D12_BUFFER_UAV Buffer;
            };
            device->CreateUnorderedAccessView(m_bufArgOut.Get(), nullptr, &descUavArgClear, m_hCpuDescriptorUavArgOutClearNsv);
            device->CreateUnorderedAccessView(m_bufArgOut.Get(), nullptr, &descUavArgClear, hCpuDescriptorUavArgOutClearSv);

            D3D12_UNORDERED_ACCESS_VIEW_DESC descUavCount =
            {
                DXGI_FORMAT_UNKNOWN,                                    // DXGI_FORMAT Format;
                D3D12_UAV_DIMENSION_BUFFER,                             // D3D12_UAV_DIMENSION ViewDimension;
                {
                    0,                                                  // UINT64 FirstElement;
                    1,                                                  // UINT NumElements;
                    sizeof(IndirectCount),                              // UINT StructureByteStride;
                    0,                                                  // UINT64 CounterOffsetInBytes;
                    D3D12_BUFFER_UAV_FLAG_NONE,                         // D3D12_BUFFER_UAV_FLAGS Flags;
                },                                                      // D3D12_BUFFER_UAV Buffer;
            };
            device->CreateUnorderedAccessView(m_bufCountOut.Get(), nullptr, &descUavCount, m_hCpuDescriptorUavCountOut);

            D3D12_UNORDERED_ACCESS_VIEW_DESC descUavCountNsv =
            {
                DXGI_FORMAT_R32_UINT,                                   // DXGI_FORMAT Format;
                D3D12_UAV_DIMENSION_BUFFER,                             // D3D12_UAV_DIMENSION ViewDimension;
                {
                    0,                                                  // UINT64 FirstElement;
                    sizeof(IndirectCount) / sizeof(UINT),               // UINT NumElements;
                    0,                                                  // UINT StructureByteStride;
                    0,                                                  // UINT64 CounterOffsetInBytes;
                    D3D12_BUFFER_UAV_FLAG_NONE,                         // D3D12_BUFFER_UAV_FLAGS Flags;
                },                                                      // D3D12_BUFFER_UAV Buffer;
            };
            device->CreateUnorderedAccessView(m_bufCountOut.Get(), nullptr, &descUavCountNsv, m_hCpuDescriptorUavCountOutClearNsv);
            device->CreateUnorderedAccessView(m_bufCountOut.Get(), nullptr, &descUavCountNsv, hCpuDescriptorUavCountOutClearSv);
        }
    }

    virtual void Uninitialize(ID3D12Device* /* device */) override
    {
        m_commandQueueCompute = nullptr;
        m_commandAllocatorCompute = nullptr;
        m_commandListCompute = nullptr;

        m_commandAllocatorGraphics = nullptr;
        m_commandListGraphics = nullptr;

        m_rootSignatureCull = nullptr;
        m_pipelineStateCullGraphics = nullptr;
        m_pipelineStateCullCompute = nullptr;
        m_commandSignatureGraphics = nullptr;
        m_commandSignatureCompute = nullptr;

        m_rootSignatureTransform = nullptr;
        m_pipelineStateTransform = nullptr;

        m_descriptorHeapUav = nullptr;
        m_descriptorHeapUavClearNsv = nullptr;
        m_descriptorHeapUavClearSv = nullptr;
        m_bufArgIn = nullptr;
        m_bufArgOut = nullptr;
        m_bufCountIn = nullptr;
        m_bufCountOut = nullptr;
        m_bufTransform = nullptr;
        m_bufIn = nullptr;
        m_bufOut = nullptr;
    }

    virtual void Check(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override
    {
        GpuFullStop(device, commandQueue);
        GpuFullStop(device, m_commandQueueCompute.Get());
    }

    virtual void Render(ID3D12Device* /* device */, ID3D12CommandQueue* commandQueue, UINT vendorId, bool hang) override
    {
        bool async = (Async == m_queue);
        auto queue = async ? m_commandQueueCompute : commandQueue;
        auto commandAllocator = async ? m_commandAllocatorCompute : m_commandAllocatorGraphics;
        auto commandList = async ? m_commandListCompute : m_commandListGraphics;
        auto commandSignature = async ? m_commandSignatureCompute : m_commandSignatureGraphics;
        auto pipelineStateCull = async ? m_pipelineStateCullCompute : m_pipelineStateCullGraphics;

        bool simpleHang = async || !IsAMD(vendorId);
        auto maxObjectCount = simpleHang ? 1U : m_objectCount;           // async only supports a max object count of 1
        auto bufCountOut = simpleHang ? nullptr : m_bufCountOut.Get();   // async doesn't support a count buffer

        PIXBeginRetailEvent(commandList.Get(), PIX_COLOR_DEFAULT, __FUNCTIONW__);

        auto heapClearSv = m_descriptorHeapUavClearSv.Get();
        commandList->SetDescriptorHeaps(1, &heapClearSv);

        UINT ClearValue[] = { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, };
        commandList->ClearUnorderedAccessViewUint(m_hGpuDescriptorUavArgOutClearSv, m_hCpuDescriptorUavArgOutClearNsv, m_bufArgOut.Get(), ClearValue, 0U, nullptr);
        commandList->ClearUnorderedAccessViewUint(m_hGpuDescriptorUavCountOutClearSv, m_hCpuDescriptorUavCountOutClearNsv, m_bufCountOut.Get(), ClearValue, 0U, nullptr);

        auto barrierCountUavToCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(m_bufCountOut.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(1, &barrierCountUavToCopyDest);
        commandList->CopyResource(m_bufCountOut.Get(), m_bufCountIn.Get());
        auto barrierCountCopyDestToUav = CD3DX12_RESOURCE_BARRIER::Transition(m_bufCountOut.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(1, &barrierCountCopyDestToUav);

        auto heap = m_descriptorHeapUav.Get();
        commandList->SetDescriptorHeaps(1, &heap);

        commandList->SetComputeRootSignature(m_rootSignatureCull.Get());
        commandList->SetPipelineState(pipelineStateCull.Get());

        // Pretend we accidentally set the wrong descriptor table range (starting at the Count buffer rather than the Arg buffer).
        // Now, indirect args were not written properly, and they retain their random initial values.
        commandList->SetComputeRootShaderResourceView(0, m_bufArgIn->GetGPUVirtualAddress());

        if (hang)
        {
            // This incorrectly binds the Count buffer to the Arg buffer slot.
            commandList->SetComputeRootDescriptorTable(1, m_hGpuDescriptorUavCountOut);
        }
        else
        {
            // This implicitly binds both the Arg and Count buffers, because they are a UAV with counter.
            commandList->SetComputeRootDescriptorTable(1, m_hGpuDescriptorUavArgOut);
        }

        assert(m_objectCount % 64 == 0);    // Don't want to worry about partial waves
        commandList->Dispatch((m_objectCount + 63) / 64, 1, 1);

        auto barrierArgUavToIndirectArg = CD3DX12_RESOURCE_BARRIER::Transition(m_bufArgOut.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        auto barrierCountUavToIndirectArg = CD3DX12_RESOURCE_BARRIER::Transition(m_bufCountOut.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        D3D12_RESOURCE_BARRIER barriersCullToTransform[] =
        {
            barrierArgUavToIndirectArg,
            barrierCountUavToIndirectArg,
        };
        commandList->ResourceBarrier(_countof(barriersCullToTransform), barriersCullToTransform);

        commandList->SetComputeRootSignature(m_rootSignatureTransform.Get());
        commandList->SetPipelineState(m_pipelineStateTransform.Get());

        commandList->SetComputeRootConstantBufferView(0, m_bufTransform->GetGPUVirtualAddress());
        commandList->SetComputeRootShaderResourceView(1, m_bufIn->GetGPUVirtualAddress());
        commandList->SetComputeRootUnorderedAccessView(2, m_bufOut->GetGPUVirtualAddress());
        if (async)
        {
            commandList->SetComputeRoot32BitConstant(3, 0, 0);  // This will be set by indirect args on graphics
        }

        commandList->ExecuteIndirect(commandSignature.Get(), maxObjectCount, m_bufArgOut.Get(), 0U, bufCountOut, 0U);

        PIXEndEvent(commandList.Get());

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

    Microsoft::WRL::ComPtr<ID3D12RootSignature>             m_rootSignatureCull;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>             m_pipelineStateCullGraphics;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>             m_pipelineStateCullCompute;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature>          m_commandSignatureGraphics;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature>          m_commandSignatureCompute;
    
    Microsoft::WRL::ComPtr<ID3D12RootSignature>             m_rootSignatureTransform;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>             m_pipelineStateTransform;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapUav;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapUavClearNsv;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapUavClearSv;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufArgIn;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufArgOut;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufCountIn;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufCountOut;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufTransform;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufIn;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bufOut;

    CD3DX12_CPU_DESCRIPTOR_HANDLE                           m_hCpuDescriptorUavArgOut;
    CD3DX12_GPU_DESCRIPTOR_HANDLE                           m_hGpuDescriptorUavArgOut;
    CD3DX12_CPU_DESCRIPTOR_HANDLE                           m_hCpuDescriptorUavCountOut;
    CD3DX12_GPU_DESCRIPTOR_HANDLE                           m_hGpuDescriptorUavCountOut;

    CD3DX12_CPU_DESCRIPTOR_HANDLE                           m_hCpuDescriptorUavArgOutClearNsv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE                           m_hGpuDescriptorUavArgOutClearSv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE                           m_hCpuDescriptorUavCountOutClearNsv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE                           m_hGpuDescriptorUavCountOutClearSv;

    static const uint32_t                                   m_objectCount = 1U << 14U;
    static const uint32_t                                   m_dataCountPerObject = 128U;
};

static HangLargeIndirectWorkload hang;
