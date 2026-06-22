//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class MemoryBenchmark final : public Benchmark
{
public:
    MemoryBenchmark() = default;

    ~MemoryBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Memory";
    }

    void Initialize(ID3D12Device* device) override
    {
        auto esram = IsDurangoClass();

        auto allSizeBytes = IsDurangoClass() ? MemoryTest::m_allSizeBytesDurango : MemoryTest::m_allSizeBytesScorpio;

        for (auto memoryParamsSrc : MemoryTest::m_allMemoryParams)
        {
            if(!esram && memoryParamsSrc.m_type == MEMORY_TYPE_ESRAM)
            {
                continue;
            }
            for (auto memoryParamsDst : MemoryTest::m_allMemoryParams)
            {
                if(!esram && memoryParamsDst.m_type == MEMORY_TYPE_ESRAM)
                {
                    continue;
                }
                for (auto sizeBytes : allSizeBytes)
                {
                    AddTest(new MemoryCopyTest(memoryParamsSrc, memoryParamsDst, sizeBytes));
                }
            }
        }

        for (auto memoryParams : MemoryTest::m_allMemoryParams)
        {
            if(!esram && memoryParams.m_type == MEMORY_TYPE_ESRAM)
            {
                continue;
            }
            for (auto sizeBytes : allSizeBytes)
            {
                for (auto write : {false, true,})
                {
                    for (auto read : {false, true,})
                    {
                        if (read || write)
                        {
                            AddTest(new MemoryCSBufferTest(memoryParams, sizeBytes, read, write));
                        }
                    }
                }
            }
        }

        for (auto blend : {false, true, })
        {
            for (auto memoryParams : MemoryTest::m_allMemoryParams)
            {
                if(!esram && memoryParams.m_type == MEMORY_TYPE_ESRAM)
                {
                    continue;
                }
                for (auto sizeBytes : allSizeBytes)
                {
                    AddTest(new MemoryWriteRTTest(memoryParams, sizeBytes, blend));
                }
            }
        }

        // MC_ARB has 4 slots

        // DRAM
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_ALL_READ_RESPONSE_DATA_BEATS_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_ALL_READ_RESPONSE_DATA_BEATS_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_32B_STALLABLE_WRITE_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_32B_STALLABLE_WRITE_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_64B_STALLABLE_WRITE_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_64B_STALLABLE_WRITE_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_32B_NONSTALLABLE_WRITE_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_32B_NONSTALLABLE_WRITE_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_64B_NONSTALLABLE_WRITE_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_64B_NONSTALLABLE_WRITE_CH1);

        // Subtract off display bandwidth
        AddCounter(GPUPerfCounters::MC_CITF_PERF_MCC_MCB_READ_RETURN_EOP_MATCHING_CID);

        // Onion
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_READ_SEND_REQ_ONION_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_READ_SEND_REQ_ONION_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_32B_ONION_WRITE_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_32B_ONION_WRITE_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_64B_ONION_WRITE_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_GN_64B_ONION_WRITE_CH1);

        // ESRAM
        AddCounter(GPUPerfCounters::MC_ARB_PERF_ESRAM_READ_RESPONSE_DATA_BEATS_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_ESRAM_READ_RESPONSE_DATA_BEATS_CH1);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_ESRAM_WRITE_RETURN_CH0);
        AddCounter(GPUPerfCounters::MC_ARB_PERF_ESRAM_WRITE_RETURN_CH1);

        auto computeShaderBlob = DX::ReadData(L"MemoryCs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            computeShaderBlob.data(),
            computeShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(MemoryTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(MemoryTest::m_rootSignature);

        MemoryTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        MemoryTest::m_rootSignature.Reset();
    }

private:
    class MemoryTest : public Test
    {
    public:
#ifdef BASE_2_BANDWIDTH
        static constexpr uint64_t KB = 1024UL;
        static constexpr uint64_t MB = 1024UL * 1024UL;
        static constexpr uint64_t GB = 1024UL * 1024UL * 1024UL;
#else
        static constexpr uint64_t KB = 1000UL;
        static constexpr uint64_t MB = 1000UL * 1000UL;
        static constexpr uint64_t GB = 1000UL * 1000UL * 1000UL;
#endif

        struct MemoryParams
        {
            MemoryType                                  m_type;
            const wchar_t*                              m_typeName;
        };

        static const std::vector<MemoryParams> m_allMemoryParams;
        static const std::vector<uint64_t> m_allSizeBytesDurango;
        static const std::vector<uint64_t> m_allSizeBytesScorpio;

        MemoryTest() :
            m_resourceAllocator()
        {
        }

        virtual ~MemoryTest()
        {
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Test", L"", 16);
            report->AddColumn(L"Src", L"", 8);
            report->AddColumn(L"Dst", L"", 8);
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Bytes (API)", L"", 12);
            report->AddColumn(L"Bytes (GPU)", L"", 12);
#ifdef BASE_2_BANDWIDTH
            report->AddColumn(L"Bandwidth", L" GiB/s", 6, 2);
#else
            report->AddColumn(L"Bandwidth", L" GB/s", 6, 2);
#endif
            report->AddColumn(L"% of \"max\"", L"%", 6, 2);

            report->AddHeader();
        }

    protected:
        float GetGBPerSecReadIdeal(MemoryType type) const
        {
            const auto& gpuProperties = GpuProperties::Get();
            auto gpuFrequency = g_gpuHardwareConfiguration.GpuFrequency;
            switch (type)
            {
            case MEMORY_TYPE_GARLIC:
                return float(gpuProperties.m_bytesPerMemTransGddr * gpuProperties.m_memTransPerSecondGddr) / GB;
            case MEMORY_TYPE_ONION:
                return float(gpuProperties.m_bytesPerNClkOnionRead * gpuProperties.m_nClkPerSecondNorthbridge) / GB;
            case MEMORY_TYPE_ESRAM:
                return float(gpuProperties.m_bytesPerSClkEsram * gpuFrequency) / GB;
            default:
                return 0.0f;
            }
        }

        float GetGBPerSecWrittenIdeal(MemoryType type) const 
        {
            const auto& gpuProperties = GpuProperties::Get();
            auto gpuFrequency = g_gpuHardwareConfiguration.GpuFrequency;
            switch (type)
            {
            case MEMORY_TYPE_GARLIC:
                return float(gpuProperties.m_bytesPerMemTransGddr * gpuProperties.m_memTransPerSecondGddr) / GB;
            case MEMORY_TYPE_ONION:
                return float(gpuProperties.m_bytesPerNClkOnionWrite * gpuProperties.m_nClkPerSecondNorthbridge) / GB;
            case MEMORY_TYPE_ESRAM:
                return float(gpuProperties.m_bytesPerSClkEsram * gpuFrequency) / GB;
            default:
                return 0.0f;
            }
        }

        uint64_t GetBytesRead(MemoryType type) const
        {
            switch (type)
            {
            case MEMORY_TYPE_GARLIC:
            {
                // These are total DRAM bytes, so we are assuming no Onion traffic
                auto reads32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_ALL_READ_RESPONSE_DATA_BEATS_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_ALL_READ_RESPONSE_DATA_BEATS_CH1);

                // Better to include display traffic, so we can see if it's affecting us
                //auto displayTraffic = GetCounterValue(GPUPerfCounters::MC_CITF_PERF_MCC_MCB_READ_RETURN_EOP_MATCHING_CID);

                return 32 * reads32;
            }

            case MEMORY_TYPE_ONION:
            {
                auto reads64 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_READ_SEND_REQ_ONION_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_READ_SEND_REQ_ONION_CH1);

                return 64 * reads64;
            }

            case MEMORY_TYPE_ESRAM:
            {
                auto reads32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_ESRAM_READ_RESPONSE_DATA_BEATS_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_ESRAM_READ_RESPONSE_DATA_BEATS_CH1);

                return 32 * reads32;
            }

            default:
                return 0;
            }
        }

        uint64_t GetBytesWritten(MemoryType type) const
        {
            switch (type)
            {
            case MEMORY_TYPE_GARLIC:
            {
                auto writes32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_STALLABLE_WRITE_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_STALLABLE_WRITE_CH1)
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_NONSTALLABLE_WRITE_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_NONSTALLABLE_WRITE_CH1);
                auto writes64 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_STALLABLE_WRITE_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_STALLABLE_WRITE_CH1)
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_NONSTALLABLE_WRITE_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_NONSTALLABLE_WRITE_CH1);

                return 32 * writes32 + 64 * writes64;
            }

            case MEMORY_TYPE_ONION:
            {
                auto writes32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_ONION_WRITE_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_ONION_WRITE_CH1);
                auto writes64 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_ONION_WRITE_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_ONION_WRITE_CH1);

                return 32 * writes32 + 64 * writes64;
            }

            case MEMORY_TYPE_ESRAM:
            {
                auto writes32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_ESRAM_WRITE_RETURN_CH0) 
                    + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_ESRAM_WRITE_RETURN_CH1);

                return 32 * writes32;
            }

            default:
                return 0;
            }
        }

        GpuBenchAllocator                       m_resourceAllocator;

    public:
        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>      m_rootSignature;
    };

    class MemoryCopyTest final : public MemoryTest
    {
    public:
        MemoryCopyTest(const MemoryParams& memoryParamsSrc, const MemoryParams& memoryParamsDst, uint64_t sizeBytes) :
            MemoryTest(),
            m_memoryParamsSrc(memoryParamsSrc),
            m_memoryParamsDst(memoryParamsDst),
            m_sizeBytes(sizeBytes)
        {
        }

        void Initialize(ID3D12Device* device) override
        {
            auto esramSize = 32 * MB;
            if ((MEMORY_TYPE_ESRAM == m_memoryParamsSrc.m_type || MEMORY_TYPE_ESRAM == m_memoryParamsDst.m_type) && m_sizeBytes > esramSize)
            {
                throw std::exception("Can't do CopyResource test on a resource larger than ESRAM");
            }
            if ((MEMORY_TYPE_ESRAM == m_memoryParamsSrc.m_type && MEMORY_TYPE_ESRAM == m_memoryParamsDst.m_type) && m_sizeBytes > esramSize / 2)
            {
                throw std::exception("Can't do CopyResource test between resources larger than 1/2 of ESRAM");
            }

            auto width = 1024U;

            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_R32G32B32A32_TYPELESS,
                width,
                UINT((m_sizeBytes / width) / 16),
                1U,
                1U);
            auto addressSrc = m_resourceAllocator.AllocateResourceMemory(device, &desc, m_memoryParamsSrc.m_type);
            DX::ThrowIfFailed(device->CreatePlacedResourceX(addressSrc, 
                &desc, 
                D3D12_RESOURCE_STATE_COPY_SOURCE, 
                nullptr, 
                IID_GRAPHICS_PPV_ARGS(m_src.ReleaseAndGetAddressOf())));

            auto addressDst = m_resourceAllocator.AllocateResourceMemory(device, &desc, m_memoryParamsDst.m_type);
            DX::ThrowIfFailed(device->CreatePlacedResourceX(addressDst, 
                &desc, 
                D3D12_RESOURCE_STATE_COPY_DEST, 
                nullptr, 
                IID_GRAPHICS_PPV_ARGS(m_dst.ReleaseAndGetAddressOf())));
        };

        void Uninitialize() override
        {
            m_resourceAllocator.FreeResourceMemory(m_src->GetGPUVirtualAddress(), m_memoryParamsSrc.m_type);
            m_resourceAllocator.FreeResourceMemory(m_dst->GetGPUVirtualAddress(), m_memoryParamsDst.m_type);

            m_src.Reset();
            m_dst.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"CopyResource ";
            name << L'(' << m_memoryParamsSrc.m_typeName << L"-->" << m_memoryParamsDst.m_typeName << L')';

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;
            auto bytesExpected = m_sizeBytes;
            auto bytesRead = GetBytesRead(m_memoryParamsSrc.m_type);
            auto bytesWritten = GetBytesWritten(m_memoryParamsDst.m_type);
            auto gbPerSecRead = (bytesRead / float(GB)) / timeMs * 1000.0f;
            auto gbPerSecWritten = (bytesWritten / float(GB)) / timeMs * 1000.0f;
            auto gbPerSec = gbPerSecRead + gbPerSecWritten;
            auto gbPerSecReadIdeal = GetGBPerSecReadIdeal(m_memoryParamsSrc.m_type);
            auto gbPerSecWrittenIdeal = GetGBPerSecWrittenIdeal(m_memoryParamsDst.m_type);
            auto timeMsReadIdeal = bytesRead ? (bytesRead / float(gbPerSecReadIdeal * GB) * 1000.0f) : 0.0f;
            auto timeMsWrittenIdeal = bytesWritten ? (bytesWritten / float(gbPerSecWrittenIdeal * GB) * 1000.0f) : 0.0f;

            // Garlic is really bound by memory speed, rather than bus speed, and memory has shared read/write bandwidth.
            // Every other case has separate read/write pathways, however it's not always possible to fully exercise both.
            // For example, the GPU can only fully exercise ESRAM if it uses large transactions, and that's not under software control.
            auto parallelBandwidth = !((MEMORY_TYPE_GARLIC == m_memoryParamsSrc.m_type) && (MEMORY_TYPE_GARLIC == m_memoryParamsDst.m_type));
            auto timeMsIdeal = parallelBandwidth ? std::max(timeMsReadIdeal, timeMsWrittenIdeal) : (timeMsReadIdeal + timeMsWrittenIdeal);
            auto gbPerSecIdeal = (bytesRead + bytesWritten) / float(GB) / timeMsIdeal * 1000.0f;

            report->AddRowData(L"CopyResource");
            report->AddRowData(m_memoryParamsSrc.m_typeName);
            report->AddRowData(m_memoryParamsDst.m_typeName);
            report->AddRowData(timeMs);
            report->AddRowData(2 * bytesExpected);
            report->AddRowData(bytesRead + bytesWritten);
            report->AddRowData(gbPerSecRead + gbPerSecWritten);
            report->AddRowData(100.0f * gbPerSec / gbPerSecIdeal);

            DirectX::XMVECTOR color;
            switch (m_memoryParamsDst.m_type)
            {
            case MEMORY_TYPE_GARLIC:
                color = DirectX::Colors::Tan;
                break;
            case MEMORY_TYPE_ONION:
                color = DirectX::Colors::White;
                break;
            case MEMORY_TYPE_ESRAM:
            default:
                color = DirectX::Colors::Silver;
                break;
            }
            report->EndRow(color);
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            commandList->CopyResource(m_dst.Get(), m_src.Get());

            // Include the full bandwidth by flushing caches
            commandList->FlushPipelineX(D3D12XBOX_FLUSH_BOP_MASK | D3D12XBOX_FLUSH_TOP_MASK, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
        }

    private:
        ComPtr<ID3D12Resource>          m_src;
        ComPtr<ID3D12Resource>          m_dst;

        MemoryParams                    m_memoryParamsSrc;
        MemoryParams                    m_memoryParamsDst;

        uint64_t                        m_sizeBytes;
    };

    class MemoryCSBufferTest final : public MemoryTest
    {
    public:
        MemoryCSBufferTest(const MemoryParams& memoryParams, uint64_t sizeBytes, bool read, bool write) :
            MemoryTest(),
            m_descriptorSrvGpu{},
            m_descriptorUavGpu{},
            m_memoryParams(memoryParams),
            m_sizeBytes(sizeBytes),
            m_read(read),
            m_write(write)
        {
            if (!m_read && !m_write)
            {
                throw std::exception("MemoryCSBufferTest requires read, write or both.\n");
            }
        }

        void Initialize(ID3D12Device* device) override
        {
            // Create two placed buffers at the same address, one for read, one for write
            auto desc = CD3DX12_RESOURCE_DESC::Buffer(
                UINT(m_sizeBytes)
            );
            auto address = m_resourceAllocator.AllocateResourceMemory(device, &desc, m_memoryParams.m_type);
            DX::ThrowIfFailed(device->CreatePlacedResourceX(address, 
                &desc, 
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 
                nullptr, 
                IID_GRAPHICS_PPV_ARGS(m_bufIn.ReleaseAndGetAddressOf())));
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            DX::ThrowIfFailed(device->CreatePlacedResourceX(address, 
                &desc, 
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 
                nullptr, 
                IID_GRAPHICS_PPV_ARGS(m_bufOut.ReleaseAndGetAddressOf())));

            D3D12_DESCRIPTOR_HEAP_DESC descHeap = 
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                2U,                                                 // UINT NumDescriptors;
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

            if (m_read)
            {
                D3D12_BUFFER_SRV bufferSrv =
                {
                    0U,                                                 // UINT64 FirstElement;
                    UINT(m_sizeBytes / m_elementBytes),                 // UINT NumElements;
                    m_elementBytes,                                     // UINT StructureByteStride;
                    D3D12_BUFFER_SRV_FLAG_NONE,                         // D3D12_BUFFER_SRV_FLAGS Flags;
                };
                D3D12_SHADER_RESOURCE_VIEW_DESC descSrv =
                {
                    DXGI_FORMAT_UNKNOWN,                                // DXGI_FORMAT Format;
                    D3D12_SRV_DIMENSION_BUFFER,                         // D3D12_SRV_DIMENSION ViewDimension;
                    D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,           // UINT Shader4ComponentMapping;
                };
                descSrv.Buffer = bufferSrv;
                device->CreateShaderResourceView(m_bufIn.Get(), &descSrv, descriptorSrvCpu);
            }
            else
            {
                NullDescriptor(descriptorSrvCpu, device->GetDescriptorHandleIncrementSize(descHeap.Type));
            }

            auto descriptorUavCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
                descriptorIndex, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));
            m_descriptorUavGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
                descriptorIndex, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));
            ++descriptorIndex;

            if (m_write)
            {
                D3D12_BUFFER_UAV bufferUav =
                {
                    0U,                                                 // UINT64 FirstElement;
                    UINT(m_sizeBytes / m_elementBytes),                 // UINT NumElements;
                    m_elementBytes,                                     // UINT StructureByteStride;
                    0ULL,                                               // UINT64 CounterOffsetInBytes;
                    D3D12_BUFFER_UAV_FLAG_NONE,                         // D3D12_BUFFER_UAV_FLAGS Flags;
                };
                D3D12_UNORDERED_ACCESS_VIEW_DESC descUav =
                {
                    DXGI_FORMAT_UNKNOWN,                                // DXGI_FORMAT Format;
                    D3D12_UAV_DIMENSION_BUFFER,                         // D3D12_UAV_DIMENSION ViewDimension;
                };
                descUav.Buffer = bufferUav;
                device->CreateUnorderedAccessView(m_bufOut.Get(), nullptr, &descUav, descriptorUavCpu);
            }
            else
            {
                NullDescriptor(descriptorUavCpu, device->GetDescriptorHandleIncrementSize(descHeap.Type));
            }

            assert(UINT(descriptorIndex) <= descHeap.NumDescriptors);

            auto computeShaderBlob = DX::ReadData(L"MemoryCs.cso");
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
            DX::ThrowIfFailed(device->CreateComputePipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineState);
        };

        void Uninitialize() override
        {
            m_pipelineState.Reset();

            m_resourceAllocator.FreeResourceMemory(m_bufIn->GetGPUVirtualAddress(), m_memoryParams.m_type);

            m_bufIn.Reset();
            m_bufOut.Reset();
            m_descriptorHeap.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"CSBuffer ";
            if (m_read)
            {
                name << L"Read";
            }
            if (m_read && m_write)
            {
                name << L"-modify-";
            }
            if (m_write)
            {
                name << L"Write";
            }
            name << L' ' << L'(' << m_memoryParams.m_typeName << L')';

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;
            auto bytesReadExpected = m_read ? m_sizeBytes : 0ULL;
            auto bytesWrittenExpected = m_write ? m_sizeBytes : 0ULL;
            auto bytesRead = GetBytesRead(m_memoryParams.m_type);
            auto bytesWritten = GetBytesWritten(m_memoryParams.m_type);
            auto gbPerSecRead = (bytesRead / float(GB)) / timeMs * 1000.0f;
            auto gbPerSecWritten = (bytesWritten / float(GB)) / timeMs * 1000.0f;
            auto gbPerSec = gbPerSecRead + gbPerSecWritten;
            auto gbPerSecReadIdeal = GetGBPerSecReadIdeal(m_memoryParams.m_type);
            auto gbPerSecWrittenIdeal = GetGBPerSecWrittenIdeal(m_memoryParams.m_type);
            auto timeMsReadIdeal = bytesRead ? (bytesRead / float(gbPerSecReadIdeal * GB) * 1000.0f) : 0.0f;
            auto timeMsWrittenIdeal = bytesWritten ? (bytesWritten / float(gbPerSecWrittenIdeal * GB) * 1000.0f) : 0.0f;

            // Garlic is really bound by memory speed, rather than bus speed, and memory has shared read/write bandwidth.
            // Every other case has separate read/write pathways, however it's not always possible to fully exercise both.
            // For example, the GPU can only fully exercise ESRAM if it uses large transactions, and that's not under software control.
            auto parallelBandwidth = MEMORY_TYPE_GARLIC != m_memoryParams.m_type;
            auto timeMsIdeal = parallelBandwidth ? std::max(timeMsReadIdeal, timeMsWrittenIdeal) : (timeMsReadIdeal + timeMsWrittenIdeal);
            auto gbPerSecIdeal = (bytesRead + bytesWritten) / float(GB) / timeMsIdeal * 1000.0f;

            report->AddRowData(m_read ? (m_write ? L"CSBuffer R/W" : L"CSBuffer Read") : L"CSBuffer Write");
            report->AddRowData(m_read ? m_memoryParams.m_typeName : L"");
            report->AddRowData(m_write ? m_memoryParams.m_typeName : L"");
            report->AddRowData(timeMs);
            report->AddRowData(bytesReadExpected + bytesWrittenExpected);
            report->AddRowData(bytesRead + bytesWritten);
            report->AddRowData(gbPerSecRead + gbPerSecWritten);
            report->AddRowData(100.0f * gbPerSec / gbPerSecIdeal);

            DirectX::XMVECTOR color;
            switch (m_memoryParams.m_type)
            {
            case MEMORY_TYPE_GARLIC:
                color = DirectX::Colors::Tan;
                break;
            case MEMORY_TYPE_ONION:
                color = DirectX::Colors::White;
                break;
            case MEMORY_TYPE_ESRAM:
            default:
                color = DirectX::Colors::Silver;
                break;
            }
            report->EndRow(color);
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

            constexpr uint32_t threadGroupSize = 64;
            commandList->Dispatch(UINT((m_sizeBytes / m_elementBytes) / threadGroupSize), 1, 1);

            // Include the full bandwidth by flushing caches
            commandList->FlushPipelineX(D3D12XBOX_FLUSH_BOP_MASK | D3D12XBOX_FLUSH_TOP_MASK, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
        }

    private:
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>         m_pipelineState;

        // These are two placed resource at the same address, so that we can do read-modify-write
        ComPtr<ID3D12Resource>              m_bufIn;
        ComPtr<ID3D12Resource>              m_bufOut;

        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeap;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorSrvGpu;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorUavGpu;

        MemoryParams                        m_memoryParams;

        uint64_t                            m_sizeBytes;
        static constexpr uint64_t           m_elementBytes = 16ULL; // uint4 in hlsl
        bool                                m_read;
        bool                                m_write;
    };

    class MemoryWriteRTTest final : public MemoryTest
    {
    public:
        MemoryWriteRTTest(MemoryParams memoryParams, uint64_t sizeBytes, bool blend) :
            MemoryTest(),
            m_descriptorRtvCpu{},
            m_viewport{},
            m_scissorRect{},
            m_memoryParams(memoryParams),
            m_sizeBytes(sizeBytes),
            m_blend(blend)
        {
            m_instances = 4;
        }

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto geometryShaderBlob = DX::ReadData(L"FullScreenGs.cso");
            auto pixelShaderBlob = DX::ReadData(L"MemoryWriteRtPs.cso");

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignature.Get(),                          // ID3D12RootSignature* pRootSignature;
                {
                    vertexShaderBlob.data(),
                    vertexShaderBlob.size(),
                },                                              // D3D12_SHADER_BYTECODE VS;
                {
                    pixelShaderBlob.data(),
                    pixelShaderBlob.size(),
                },                                              // D3D12_SHADER_BYTECODE PS;
                {},                                             // D3D12_SHADER_BYTECODE DS;
                {},                                             // D3D12_SHADER_BYTECODE HS;
                {
                    geometryShaderBlob.data(),
                    geometryShaderBlob.size(),
                },                                              // D3D12_SHADER_BYTECODE GS;
                {},                                             // D3D12_STREAM_OUTPUT_DESC StreamOutput;
                CD3DX12_BLEND_DESC(D3D12_DEFAULT),              // D3D12_BLEND_DESC BlendState;
                UINT_MAX,                                       // UINT SampleMask;
                CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),         // D3D12_RASTERIZER_DESC RasterizerState;
                CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),      // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
                {},                                             // D3D12_INPUT_LAYOUT_DESC InputLayout;
                D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,    // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,            // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
                1U,                                             // UINT NumRenderTargets;
                { DXGI_FORMAT_R16G16B16A16_FLOAT, },            // DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
                DXGI_FORMAT_UNKNOWN,                            // DXGI_FORMAT DSVFormat;
                { 
                    1U, 
                    DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN, 
                },                                              // DXGI_SAMPLE_DESC SampleDesc;
                0U,                                             // UINT NodeMask;
                {},                                             // D3D12_CACHED_PIPELINE_STATE CachedPSO;
                D3D12_PIPELINE_STATE_FLAG_NONE,                 // D3D12_PIPELINE_STATE_FLAGS Flags;
            };
            if (m_blend)
            {
                // Subtractive blend can iterate indefinitely without converging to an optimized case
                D3D12_RENDER_TARGET_BLEND_DESC blendSubtractive = 
                {
                    m_blend,                                            // BOOL BlendEnable;
                    FALSE,                                              // BOOL LogicOpEnable; // LogicOpEnable and BlendEnable can't both be true
                    D3D12_BLEND_ONE,                                    // D3D12_BLEND SrcBlend;
                    D3D12_BLEND_ONE,                                    // D3D12_BLEND DestBlend;
                    D3D12_BLEND_OP_SUBTRACT,                            // D3D12_BLEND_OP BlendOp;
                    D3D12_BLEND_ONE,                                    // D3D12_BLEND SrcBlendAlpha;
                    D3D12_BLEND_ONE,                                    // D3D12_BLEND DestBlendAlpha;
                    D3D12_BLEND_OP_SUBTRACT,                            // D3D12_BLEND_OP BlendOpAlpha;
                    D3D12_LOGIC_OP_CLEAR,                               // D3D12_LOGIC_OP LogicOp; // applies to RGBA
                    D3D12_COLOR_WRITE_ENABLE_ALL,                       // UINT8 RenderTargetWriteMask;
                };

                descPipelineState.BlendState.RenderTarget[0] = blendSubtractive;
            }
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineState);

            auto width = 1024U;
            auto height = (m_sizeBytes / width) / 8;

            D3D12_VIEWPORT viewport = 
            {
                0,                                                          // FLOAT TopLeftX;
                0,                                                          // FLOAT TopLeftY;
                FLOAT(width),                                               // FLOAT Width;
                FLOAT(height),                                              // FLOAT Height;
                0.0f,                                                       // FLOAT MinDepth;
                1.0f,                                                       // FLOAT MaxDepth;
            };
            m_viewport = viewport;

            D3D12_RECT scissorRect =
            {
                0,                                                          // LONG    left;
                0,                                                          // LONG    top;
                LONG(width),                                                // LONG    right;
                LONG(height),                                               // LONG    bottom;
            };
            m_scissorRect = scissorRect;

            // Note that whatever type we choose here, we will usually hit pixel rate before we hit ESRAM rate.
            // R16G16B16A16_FLOAT is the only type which hits bandwidth limits first, and only when blend is enabled.
            auto descTex = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_R16G16B16A16_FLOAT,
                width,
                UINT(height),
                1U,
                1U,
                1U,
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12XBOX_RESOURCE_FLAG_DENY_COLOR_COMPRESSION_DATA
            );
            auto pageFlag = uint32_t(IsScorpioClass() ? MEM_2MB_PAGES : MEM_64K_PAGES);
            auto address = m_resourceAllocator.AllocateResourceMemory(device, &descTex, m_memoryParams.m_type, pageFlag);
            DX::ThrowIfFailed(device->CreatePlacedResourceX(address, 
                &descTex, 
                D3D12_RESOURCE_STATE_RENDER_TARGET, 
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_tex.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_tex);

            D3D12_DESCRIPTOR_HEAP_DESC descHeapRtv = 
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                1U,                                                 // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapRtv, IID_GRAPHICS_PPV_ARGS(m_descriptorHeapRtv.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeapRtv);

            m_descriptorRtvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeapRtv.Type));

            device->CreateRenderTargetView(m_tex.Get(), nullptr, m_descriptorRtvCpu);
        };

        void Uninitialize() override
        {
            m_pipelineState.Reset();

            m_resourceAllocator.FreeResourceMemory(m_tex->GetGPUVirtualAddress(), m_memoryParams.m_type);

            m_tex.Reset();
            m_descriptorHeapRtv.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << (m_blend ? L"BlendRT" : L"WriteRT");
            name << L'(' << m_memoryParams.m_typeName << L')';

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;
            auto bytesExpected = m_sizeBytes * m_instances;
            auto bytesRead = m_blend ? GetBytesRead(m_memoryParams.m_type) : 0;
            auto bytesWritten = GetBytesWritten(m_memoryParams.m_type);
            auto gbPerSecRead = (bytesRead / float(GB)) / timeMs * 1000.0f;
            auto gbPerSecWritten = (bytesWritten / float(GB)) / timeMs * 1000.0f;
            auto gbPerSec = gbPerSecRead + gbPerSecWritten;
            auto gbPerSecReadIdeal = GetGBPerSecReadIdeal(m_memoryParams.m_type);
            auto gbPerSecWrittenIdeal = GetGBPerSecWrittenIdeal(m_memoryParams.m_type);
            auto timeMsReadIdeal = bytesRead ? (bytesRead / float(gbPerSecReadIdeal * GB) * 1000.0f) : 0.0f;
            auto timeMsWrittenIdeal = bytesWritten ? (bytesWritten / float(gbPerSecWrittenIdeal * GB) * 1000.0f) : 0.0f;

            // Garlic is really bound by memory speed, rather than bus speed, and memory has shared read/write bandwidth.
            // Every other case has separate read/write pathways, however it's not always possible to fully exercise both.
            // For example, the GPU can only fully exercise ESRAM if it uses large transactions, and that's not under software control.
            auto parallelBandwidth = MEMORY_TYPE_GARLIC != m_memoryParams.m_type;
            auto timeMsIdeal = parallelBandwidth ? std::max(timeMsReadIdeal, timeMsWrittenIdeal) : (timeMsReadIdeal + timeMsWrittenIdeal);
            auto gbPerSecIdeal = (bytesRead + bytesWritten) / float(GB) / timeMsIdeal * 1000.0f;

            report->AddRowData(m_blend ? L"BlendRT" : L"WriteRT");
            report->AddRowData(m_blend ? m_memoryParams.m_typeName : L"");
            report->AddRowData(m_memoryParams.m_typeName);
            report->AddRowData(timeMs);
            report->AddRowData(m_blend ? (2 * bytesExpected) : bytesExpected);
            report->AddRowData(bytesRead + bytesWritten);
            report->AddRowData(gbPerSecRead + gbPerSecWritten);
            report->AddRowData(100.0f * gbPerSec / gbPerSecIdeal);

            DirectX::XMVECTOR color;
            switch (m_memoryParams.m_type)
            {
            case MEMORY_TYPE_GARLIC:
                color = DirectX::Colors::Tan;
                break;
            case MEMORY_TYPE_ONION:
                color = DirectX::Colors::White;
                break;
            case MEMORY_TYPE_ESRAM:
            default:
                color = DirectX::Colors::Silver;
                break;
            }
            report->EndRow(color);
        }

        // This activity occurs outside the timing brackets
        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            commandList->ClearRenderTargetView(m_descriptorRtvCpu, DirectX::Colors::Transparent, 0U, nullptr);
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

            commandList->OMSetRenderTargets(1U, &m_descriptorRtvCpu, FALSE, nullptr);

            commandList->RSSetViewports(1, &m_viewport);
            commandList->RSSetScissorRects(1, &m_scissorRect);

            commandList->DrawInstanced(1, m_instances, 0, 0);

            // Include the full bandwidth by flushing caches
            commandList->FlushPipelineX(D3D12XBOX_FLUSH_BOP_MASK | D3D12XBOX_FLUSH_TOP_MASK, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
        }

    private:
        ComPtr<ID3D12PipelineState>     m_pipelineState;

        ComPtr<ID3D12Resource>          m_tex;
        ComPtr<ID3D12DescriptorHeap>    m_descriptorHeapRtv;
        D3D12_CPU_DESCRIPTOR_HANDLE     m_descriptorRtvCpu;

        D3D12_VIEWPORT                  m_viewport;
        D3D12_RECT                      m_scissorRect;

        MemoryParams                    m_memoryParams;

        uint64_t                        m_sizeBytes;
        uint32_t                        m_instances;
        bool                            m_blend;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature> MemoryBenchmark::MemoryTest::m_rootSignature;

const std::vector<MemoryBenchmark::MemoryTest::MemoryParams> MemoryBenchmark::MemoryTest::m_allMemoryParams =
{
    { MEMORY_TYPE_GARLIC,  L"Garlic",  },
    { MEMORY_TYPE_ONION,   L"Onion",   },
    { MEMORY_TYPE_ESRAM,   L"ESRAM",   },
};

const std::vector<uint64_t> MemoryBenchmark::MemoryTest::m_allSizeBytesDurango = 
{
    16 * MB, 
};

const std::vector<uint64_t> MemoryBenchmark::MemoryTest::m_allSizeBytesScorpio = 
{
    64 * MB, 
};

MemoryBenchmark benchmark;
