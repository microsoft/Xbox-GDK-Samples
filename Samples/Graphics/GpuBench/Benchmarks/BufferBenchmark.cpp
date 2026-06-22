//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class BufferBenchmark final : public Benchmark
{
public:
    BufferBenchmark() = default;

    ~BufferBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Buffer";
    }

    void Initialize(ID3D12Device* device) override
    {
        // Test for all formats, msaa
        for (auto& memOpParams : BufferTest::m_allMemOpParams)
        {
            for (auto& formatParams : BufferTest::m_allFormatParams)
            {
                // SMEM ops don't respect format anyhow
                if (BufferTest::MemOpType::OP_SMEM_READ == memOpParams.m_type
                    && !(DXGI_FORMAT_UNKNOWN == formatParams.m_format || DXGI_FORMAT_R32_UINT == formatParams.m_format))
                {
                    continue;
                }

                // Atomics are single channel by nature
                // In standard HLSL, atomics require a 32-bit type
                if ((BufferTest::MemOpType::OP_VMEM_ATOMIC == memOpParams.m_type || BufferTest::MemOpType::OP_VMEM_ATOMIC_RETURN == memOpParams.m_type)
                    && !(DXGI_FORMAT_UNKNOWN == formatParams.m_format || (1 == formatParams.m_channels && 4 == formatParams.m_elementBytes)))
                {
                    continue;
                }

                AddTest(new BufferTest(memOpParams, formatParams));
            }
        }

#ifdef _GAMING_XBOX_SCARLETT
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_TEX);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_SMEM);
#else
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_VMEM);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_SMEM);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_BUFFER_COALESCED_READ_CYCLES);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_BUFFER_COALESCED_WRITE_CYCLES);
#endif

#ifdef _GAMING_XBOX_XBOXONE
        // Barrier validation complains because we place m_bufRead and m_bufWrite at the same address with different resource states.
        // ID3D12CommandQueue(Graphics)::ExecuteCommandLists: Descriptor index 0 (range index 0, index in the range 0, api slot 0, heap index 2, ptr 0x801a0040) in the descriptor table of root parameter 3 belongs to resource (0x0000000240002700 "m_bufRead") that is expected to have UNORDERED_ACCESS resource state (0x8), but the resource has D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE state (0x40).
        m_scopedErrorFilter = new ScopedErrorFilter(device, 0xD51D6921);
#else
        std::ignore = device;
#endif

        BufferTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
#ifdef _GAMING_XBOX_XBOXONE
        delete m_scopedErrorFilter;
#endif
    }

private:
#ifdef _GAMING_XBOX_XBOXONE
    ScopedErrorFilter*                              m_scopedErrorFilter = {};
#endif

    class BufferTest final : public Test
    {
        static constexpr uint32_t m_maxFetches = 64;

        static constexpr uint32_t m_threadGroupX = 64;
        static constexpr uint32_t m_threadGroupY = 1;
        static constexpr uint32_t m_threadGroupZ = 1;
        static constexpr uint32_t m_threadPerWave = 64; // Change this to per-test if we ever go to wave32 on Scarlett

    public:
        enum MemOpType : uint32_t
        {
            OP_SMEM_READ, 
            OP_VMEM_READ,
            OP_VMEM_WRITE, 
            OP_VMEM_ATOMIC, 
            OP_VMEM_ATOMIC_RETURN, 

            OP_COUNT
        };

        struct MemOpParams
        {
            MemOpType                                   m_type;
            const wchar_t*                              m_typeName;
            const wchar_t*                              m_shaderPrefix;
            uint32_t                                    m_clockPerOpPerWaveIdeal;
            uint32_t                                    m_opsExpected;
            uint32_t                                    m_opsExtra;
            bool                                        m_scalar;
        };

        static const std::vector<MemOpParams>           m_allMemOpParams;

        struct FormatParams
        {
            const wchar_t*                              m_formatName;
            const wchar_t*                              m_numberName;
            const wchar_t*                              m_dataName;
            const wchar_t*                              m_widthName;
            DXGI_FORMAT                                 m_format;
            uint32_t                                    m_channels;
            uint32_t                                    m_elementBytes;
        };

        static const std::vector<FormatParams> m_allFormatParams;

        BufferTest(const MemOpParams& memOpParams, const FormatParams& formatParams) :
            m_memOpParams(memOpParams),
            m_formatParams(formatParams),
            m_descriptorSrvGpu{},
            m_descriptorRawSrvGpu{},
            m_descriptorUavCpu{},
            m_descriptorUavGpu{},
            m_descriptorRawUavGpu{}
        {
            if (m_maxFetches < m_memOpParams.m_opsExpected)
            {
                throw std::exception("More fetches in shader than expected.");
            }

            // We will build a 1D buffer, long enough to hold m_sizeX threadgroups, plus whatever additional fetches 
            // are made by the shader. The Dispatch will have a m_sizeY whose puepose is to repeat operations enough times
            // for a good measurement.
            m_sizeX = 32 * 1024;

            // Can have at most 65535 threadgroups in each dimension, so m_sizeX / m_threadGroupX must be < 4 * 1024 * 1024 
            m_sizeX = std::min(m_sizeX, m_threadGroupX * uint32_t(D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION));

            m_sizeY = 512;
            m_sizeY /= m_memOpParams.m_clockPerOpPerWaveIdeal;
            if (g_gpuHardwareConfiguration.HardwareVersion == D3D12XBOX_HARDWARE_VERSION_SCORPIO)
            {
                m_sizeY *= 4;
            }
            m_sizeY = std::max(m_sizeY, 1U);

            m_sizeZ = 1;

            // We sometimes only use small portions of the buffer, but for safety, allocate enough elements for separate fetches
            m_sizeBytes = (m_sizeX * m_maxFetches) * m_formatParams.m_elementBytes;
        }

        void Initialize(ID3D12Device* device) override
        {
            auto shaderNeedsFormat = (OP_VMEM_READ == m_memOpParams.m_type || OP_VMEM_WRITE == m_memOpParams.m_type);
            auto shaderNeedsWidth = (OP_VMEM_WRITE == m_memOpParams.m_type);

            std::wostringstream computeShaderName;
            computeShaderName << m_memOpParams.m_shaderPrefix;
            if (shaderNeedsFormat)
            {
                computeShaderName << m_formatParams.m_numberName;
                computeShaderName << L"x";
                computeShaderName << m_formatParams.m_dataName;
            }
            if (shaderNeedsWidth)
            {
                computeShaderName << m_formatParams.m_widthName;
            }
            computeShaderName << L"Cs.cso";
            auto computeShaderBlob = DX::ReadData(computeShaderName.str().c_str());
            DX::ThrowIfFailed(device->CreateRootSignature(0, 
                computeShaderBlob.data(),
                computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_rootSignature);

            D3D12_COMPUTE_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignature.Get(),                          // ID3D12RootSignature* pRootSignature;
                {
                    computeShaderBlob.data(),
                    computeShaderBlob.size(),
                },                                              // D3D12_SHADER_BYTECODE CS;
                0,                                              // UINT NodeMask;
                { nullptr, 0, },                                // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            };

            // The default for NumThreadgroupsWalkedPerCu changed from 1U to 2U between D3D11.X and D3D12.X, 
            // but the original default gives better results here.
            // The result is that COMPUTE_SHADER_LIMITS.CU_GROUP_COUNT gets set to 0U rather than 1U.
            D3D12XBOX_COMPUTE_SHADER_LIMITS_DESC descShaderLimits =
            {
                D3D12XBOX_SHADER_UNIT_DISABLE_FLAG_NONE,                    // D3D12XBOX_SHADER_UNIT_DISABLE_FLAGS DisableFlags;
                D3D12XBOX_SHADER_MAX_WAVES_DEFAULT,                         // UINT MaxWaves;
#if defined(_GAMING_XBOX_SCARLETT)
                D3D12XBOX_COMPUTE_SHADER_MAX_THREADGROUPS_PER_CU_DEFAULT,   // UINT MaxThreadgroupsPerCu;
#else
                D3D12XBOX_SHADER_MAX_THREADGROUPS_PER_CU_DEFAULT,           // UINT MaxThreadgroupsPerCu;
#endif
                D3D12XBOX_SHADER_MAX_WAVES_FOR_CU_LOCKING_DEFAULT,          // UINT MaxWavesForCuLocking;
                1U,                                                         // UINT NumThreadgroupsWalkedPerCu;
                D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM_DEFAULT,            // D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM SimdWalkAlgorithm;
            };
            D3D12XBOX_EXTENDED_COMPUTE_PIPELINE_STATE_DESC descExtended =
            {
                D3D12XBOX_EXTENDED_COMPUTE_PIPELINE_STATE_SHADER_LIMITS,    // D3D12XBOX_EXTENDED_COMPUTE_PIPELINE_STATE_TYPE Type; 
                {},
            };
            descExtended.ShaderLimitsDesc = descShaderLimits;

            DX::ThrowIfFailed(device->CreateComputePipelineStateX(&descPipelineState, 1U, &descExtended, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineState);

            if (DXGI_FORMAT_UNKNOWN == m_formatParams.m_format)
            {
                // Test instruction cost in the absence of memory operations
                // Leave the buffers as null
            }
            else
            {
                CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

                auto descBuf = CD3DX12_RESOURCE_DESC::Buffer(
                    UINT(m_sizeBytes)
                );
                DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,             
                    D3D12_HEAP_FLAG_NONE, 
                    &descBuf, 
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_bufRead.ReleaseAndGetAddressOf())));
                SET_NAME_TO_SELF(m_bufRead);

#ifdef _GAMING_XBOX_SCARLETT
                descBuf.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,             
                    D3D12_HEAP_FLAG_NONE, 
                    &descBuf, 
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_bufWrite.ReleaseAndGetAddressOf())));
                SET_NAME_TO_SELF(m_bufWrite);
#else
                // It's important to have the read and write resource be at the same address, even
                // though we only use one or the other. The write resource is bound during the read
                // tests, just to give a possible side effect. For some reason, its address affects
                // performance, even though no writes are done to it.
                descBuf.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                DX::ThrowIfFailed(device->CreatePlacedResourceX(m_bufRead->GetGPUVirtualAddress(), 
                    &descBuf, 
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_bufWrite.ReleaseAndGetAddressOf())));
                SET_NAME_TO_SELF(m_bufWrite);
#endif
            }

            // Srv
            D3D12_DESCRIPTOR_HEAP_DESC descHeap = 
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                4U,                                                 // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,          // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeap, IID_GRAPHICS_PPV_ARGS(m_descriptorHeap.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeap);

            auto descriptorIndex = 0;
            auto descriptorSrvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
                descriptorIndex, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));
            m_descriptorSrvGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
                descriptorIndex, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));
            ++descriptorIndex;

            auto descriptorRawSrvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
                descriptorIndex, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));
            m_descriptorRawSrvGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
                descriptorIndex, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));
            ++descriptorIndex;

            if (DXGI_FORMAT_UNKNOWN == m_formatParams.m_format)
            {
                NullDescriptor(descriptorSrvCpu, device->GetDescriptorHandleIncrementSize(descHeap.Type));
                NullDescriptor(descriptorRawSrvCpu, device->GetDescriptorHandleIncrementSize(descHeap.Type));
            }
            else
            {
                D3D12_BUFFER_SRV bufferSrv =
                {
                    0U,                                                 // UINT64 FirstElement;
                    UINT(m_sizeBytes / m_formatParams.m_elementBytes),  // UINT NumElements;
                    0U,                                                 // UINT StructureByteStride;
                    D3D12_BUFFER_SRV_FLAG_NONE,                         // D3D12_BUFFER_SRV_FLAGS Flags;
                };
                D3D12_SHADER_RESOURCE_VIEW_DESC descSrv =
                {
                    m_formatParams.m_format,                            // DXGI_FORMAT Format;
                    D3D12_SRV_DIMENSION_BUFFER,                         // D3D12_SRV_DIMENSION ViewDimension;
                    D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,           // UINT Shader4ComponentMapping;
                };
                descSrv.Buffer = bufferSrv;
                device->CreateShaderResourceView(m_bufRead.Get(), &descSrv, descriptorSrvCpu);

                // Raw views must be typeless
                descSrv.Format = DXGI_FORMAT_R32_TYPELESS;
                descSrv.Buffer.NumElements =  UINT(m_sizeBytes / 4);
                descSrv.Buffer.Flags |= D3D12_BUFFER_SRV_FLAG_RAW;
                device->CreateShaderResourceView(m_bufRead.Get(), &descSrv, descriptorRawSrvCpu);
            }

            // Uav
            m_descriptorUavCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
                descriptorIndex, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));
            m_descriptorUavGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
                descriptorIndex, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));
            ++descriptorIndex;

            auto descriptorRawUavCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
                descriptorIndex, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));
            m_descriptorRawUavGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
                descriptorIndex, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));
            ++descriptorIndex;

            if (DXGI_FORMAT_UNKNOWN == m_formatParams.m_format)
            {
                NullDescriptor(m_descriptorUavCpu, device->GetDescriptorHandleIncrementSize(descHeap.Type));
                NullDescriptor(descriptorRawUavCpu, device->GetDescriptorHandleIncrementSize(descHeap.Type));
            }
            else
            {
                D3D12_BUFFER_UAV bufferUav =
                {
                    0U,                                                 // UINT64 FirstElement;
                    UINT(m_sizeBytes / m_formatParams.m_elementBytes),  // UINT NumElements;
                    0U,                                                 // UINT StructureByteStride;
                    0ULL,                                               // UINT64 CounterOffsetInBytes;
                    D3D12_BUFFER_UAV_FLAG_NONE,                          // D3D12_BUFFER_UAV_FLAGS Flags;
                };
                D3D12_UNORDERED_ACCESS_VIEW_DESC descUav =
                {
                    m_formatParams.m_format,                            // DXGI_FORMAT Format;
                    D3D12_UAV_DIMENSION_BUFFER,                         // D3D12_SRV_DIMENSION ViewDimension;
                };
                descUav.Buffer = bufferUav;
                device->CreateUnorderedAccessView(m_bufWrite.Get(), nullptr, &descUav, m_descriptorUavCpu);

                // Raw views must be typeless
                descUav.Format = DXGI_FORMAT_R32_TYPELESS;
                descUav.Buffer.NumElements =  UINT(m_sizeBytes / 4);
                descUav.Buffer.Flags |= D3D12_BUFFER_UAV_FLAG_RAW;
                device->CreateUnorderedAccessView(m_bufWrite.Get(), nullptr, &descUav, descriptorRawUavCpu);
            }

            assert(UINT(descriptorIndex) <= descHeap.NumDescriptors);
        }

        void Uninitialize() override
        {
            m_rootSignature.Reset();
            m_pipelineState.Reset();

            m_bufRead.Reset();
            m_bufWrite.Reset();

            m_descriptorHeap.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Format: ";
            name << m_formatParams.m_formatName << L", ";
            name << L"Op: ";
            name << m_memOpParams.m_typeName << L", ";

            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Format", L"", 20);
            report->AddColumn(L"Type", L"", 12);
            report->AddColumn(L"Time", L" ms", 6, 2);
            report->AddColumn(L"MEM(API)", L"", 12, 0);
            report->AddColumn(L"MEM(GPU)", L"", 12, 0);
            report->AddColumn(L"Coalesced", L"", 12, 0);
            report->AddColumn(L"MEM/clock", L"", 5, 2);
            report->AddColumn(L"% of max", L"%", 6, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            const auto& gpuProperties = GpuProperties::Get();
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;

            auto timeMs = m_elapsedTime;

            auto memPerInstruction = m_memOpParams.m_scalar ? 1 : m_threadPerWave;
            auto memApi = m_sizeX * m_sizeY * m_sizeZ * (m_memOpParams.m_opsExpected + m_memOpParams.m_opsExtra) / (m_threadPerWave / memPerInstruction);
#ifdef _GAMING_XBOX_SCARLETT
            auto memGpu = memPerInstruction * GetCounterValue(m_memOpParams.m_scalar ? GPUPerfCounters::SQ_PERF_SEL_INSTS_SMEM : GPUPerfCounters::SQ_PERF_SEL_INSTS_TEX);
#else
            auto memGpu = memPerInstruction * GetCounterValue(m_memOpParams.m_scalar ? GPUPerfCounters::SQ_PERF_SEL_INSTS_SMEM : GPUPerfCounters::SQ_PERF_SEL_INSTS_VMEM);
#endif
            auto memPerClock = (memGpu * 1000.0f) / (timeMs * clockSpeed);
            auto numSe = gpuProperties.m_numSe;
            auto numCuPerSe = gpuProperties.m_numCuPerSe;
            auto memPerClockIdeal = memPerInstruction * numCuPerSe * numSe / (float) m_memOpParams.m_clockPerOpPerWaveIdeal;

            // coalescing
            if (1 == m_formatParams.m_channels && (OP_VMEM_READ == m_memOpParams.m_type || OP_VMEM_WRITE == m_memOpParams.m_type))
            {
                // This also requires coherent memory accesses, which are provided for by the shader
                memPerClockIdeal *= 4;
            }
#ifdef _GAMING_XBOX_SCARLETT
            // Coalescing works differently on Scarlett and the relevant counters are gone.
            auto memCoalesced = 0UL;
#else
            // There are several counters applicable here, but this seems to be what we want. 
            // Some of the others mean "potentially coalescable" but don't actually indicate coalescing.
            auto memCoalesced = (GetCounterValue(GPUPerfCounters::TA_PERF_SEL_BUFFER_COALESCED_READ_CYCLES)
                + GetCounterValue(GPUPerfCounters::TA_PERF_SEL_BUFFER_COALESCED_WRITE_CYCLES))
                * memPerInstruction / 4;
#endif

            report->AddRowData(m_formatParams.m_formatName);
            report->AddRowData(m_memOpParams.m_typeName);
            report->AddRowData(timeMs);
            report->AddRowData(memApi);
            report->AddRowData(memGpu);
            report->AddRowData(memCoalesced);
            report->AddRowData(memPerClock);
            report->AddRowData(100.0f * memPerClock / memPerClockIdeal);

            const DirectX::XMVECTOR color = (m_memOpParams.m_type % 2) ? DirectX::Colors::Tan : DirectX::Colors::White;
            report->EndRow(color);
        }

        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            ID3D12DescriptorHeap* descriptorHeaps[] =
            {
                m_descriptorHeap.Get(), 
            };
            commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

            if (DXGI_FORMAT_UNKNOWN != m_formatParams.m_format)
            {
                auto TransitionBarrier = [commandList] (ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)->void
                {
                    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
                    commandList->ResourceBarrier(1U, &barrier);
                };

                UINT clearValue[4] = {};
                commandList->ClearUnorderedAccessViewUint(m_descriptorUavGpu, m_descriptorUavCpu, m_bufWrite.Get(), clearValue, 0U, nullptr);

                TransitionBarrier(m_bufWrite.Get(), 
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 
                    D3D12_RESOURCE_STATE_COPY_SOURCE);
                TransitionBarrier(m_bufRead.Get(), 
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 
                    D3D12_RESOURCE_STATE_COPY_DEST);

                commandList->CopyResource(m_bufRead.Get(), m_bufWrite.Get());

                TransitionBarrier(m_bufRead.Get(), 
                    D3D12_RESOURCE_STATE_COPY_DEST, 
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
                TransitionBarrier(m_bufWrite.Get(), 
                    D3D12_RESOURCE_STATE_COPY_SOURCE, 
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            }
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            ID3D12DescriptorHeap* descriptorHeaps[] =
            {
                m_descriptorHeap.Get(), 
            };
            commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

            commandList->SetComputeRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            commandList->SetComputeRootDescriptorTable(ROOT_PARAMETER_INDEX_SRV, m_descriptorSrvGpu);
            commandList->SetComputeRootDescriptorTable(ROOT_PARAMETER_INDEX_UAV, m_descriptorUavGpu);

            commandList->Dispatch(m_sizeX / m_threadGroupX, m_sizeY / m_threadGroupY, m_sizeZ / m_threadGroupZ);
        }

    private:
        // Resources which are unique per-test
        MemOpParams                         m_memOpParams;
        FormatParams                        m_formatParams;

        ComPtr<ID3D12RootSignature>         m_rootSignature;
        ComPtr<ID3D12PipelineState>         m_pipelineState;

        ComPtr<ID3D12Resource>              m_bufRead;
        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeap;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorSrvGpu;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorRawSrvGpu;
        ComPtr<ID3D12Resource>              m_bufWrite;
        D3D12_CPU_DESCRIPTOR_HANDLE         m_descriptorUavCpu;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorUavGpu;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorRawUavGpu;

        uint32_t                            m_sizeX;
        uint32_t                            m_sizeY;
        uint32_t                            m_sizeZ;
        uint32_t                            m_sizeBytes;
    };
};

const std::vector<BufferBenchmark::BufferTest::MemOpParams> BufferBenchmark::BufferTest::m_allMemOpParams =
{
    { OP_SMEM_READ,             L"SMEM read",       L"BufferSmemRead",            1,      64,   1,  true,    },
    { OP_VMEM_READ,             L"VMEM read",       L"BufferVmemRead",            16,     64,   0,  false,   },
    { OP_VMEM_WRITE,            L"VMEM write",      L"BufferVmemWrite",           16,     64,   0,  false,   },
    { OP_VMEM_ATOMIC,           L"VMEM atomic",     L"BufferVmemAtomic",          16,     64,   0,  false,   },
    { OP_VMEM_ATOMIC_RETURN,    L"VMEM atomret",    L"BufferVmemAtomicReturn",    16,     64,   0,  false,   },
};

const std::vector<BufferBenchmark::BufferTest::FormatParams> BufferBenchmark::BufferTest::m_allFormatParams = 
{
    { L"NULL",                L"1",     L"uint",    L"8",   DXGI_FORMAT_UNKNOWN,              1,  1,  }, 
    { L"R8_UINT",             L"1",     L"uint",    L"8",   DXGI_FORMAT_R8_UINT,              1,  1,  }, 
    { L"R8G8_UINT",           L"2",     L"uint",    L"8",   DXGI_FORMAT_R8G8_UINT,            2,  2,  }, 
    { L"R16_UINT",            L"1",     L"uint",    L"16",  DXGI_FORMAT_R16_UINT,             1,  2,  }, 
    { L"R8G8B8A8_UINT",       L"4",     L"uint",    L"8",   DXGI_FORMAT_R8G8B8A8_UINT,        4,  4,  }, 
    { L"R16G16_UINT",         L"2",     L"uint",    L"16",  DXGI_FORMAT_R16G16_UINT,          2,  4,  }, 
    { L"R32_UINT",            L"1",     L"uint",    L"32",  DXGI_FORMAT_R32_UINT,             1,  4,  }, 
    { L"R16G16B16A16_UINT",   L"4",     L"uint",    L"16",  DXGI_FORMAT_R16G16B16A16_UINT,    4,  8,  }, 
    { L"R32G32_UINT",         L"2",     L"uint",    L"32",  DXGI_FORMAT_R32G32_UINT,          2,  8,  }, 
    { L"R32G32B32A32_UINT",   L"4",     L"uint",    L"32",  DXGI_FORMAT_R32G32B32A32_UINT,    4,  16, }, 
};

BufferBenchmark benchmark;

