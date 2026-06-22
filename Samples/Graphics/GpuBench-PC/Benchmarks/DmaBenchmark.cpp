//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class DmaBenchmark final : public Benchmark
{
public:
    DmaBenchmark() :
        Benchmark(), 
        m_scopedDebugFlags(nullptr), 
        m_fenceAddress(nullptr)
    {
    }

    ~DmaBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"DMA";
    }

    void Initialize(ID3D12Device* device) override
    {
        auto esram = IsDurangoClass();

        auto allSizeBytes = IsDurangoClass() ? DmaTest::m_allSizeBytesDurango : DmaTest::m_allSizeBytesScorpio;

        for (auto memoryParamsSrc : DmaTest::m_allMemoryParams)
        {
            if(!esram && memoryParamsSrc.m_type == MEMORY_TYPE_ESRAM)
            {
                continue;
            }
            for (auto memoryParamsDst : DmaTest::m_allMemoryParams)
            {
                if(!esram && memoryParamsDst.m_type == MEMORY_TYPE_ESRAM)
                {
                    continue;
                }
                for (auto sizeBytes : allSizeBytes)
                {
                    for (auto dmaCombo : DmaTest::m_allDmaCombos)
                    {
                        AddTest(new DmaCopyTest(dmaCombo, memoryParamsSrc, memoryParamsDst, sizeBytes));
                    }
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

        // Use this to detect display bandwidth
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

        m_scopedDebugFlags = new ScopedDebugFlags(device, D3D12XBOX_DEBUG_FLAG_DISABLE_BARRIER_VALIDATION, D3D12XBOX_DEBUG_FLAG_NONE);

        for (auto dmaIndex = 0U; dmaIndex < _countof(DmaTest::m_dmaEngines); ++dmaIndex)
        {
            if (0 != dmaIndex)
            {
                auto& dmaEngine = DmaTest::m_dmaEngines[dmaIndex];

                static_assert(1 == D3D11_DMA_ENGINE_CONTEXT_CREATE_SDMA_1, "Enum mismatch with dma engine index\n");
                static_assert(2 == D3D11_DMA_ENGINE_CONTEXT_CREATE_SDMA_2, "Enum mismatch with dma engine index\n");
                static_assert(3 == D3D11_DMA_ENGINE_CONTEXT_CREATE_SDMA_3, "Enum mismatch with dma engine index\n");

                DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12XBOX_COMMAND_LIST_TYPE_DMA, 
                    IID_GRAPHICS_PPV_ARGS(dmaEngine.m_commandAllocatorDma.ReleaseAndGetAddressOf())));
                SET_NAME_TO_SELF(dmaEngine.m_commandAllocatorDma);

                DX::ThrowIfFailed(device->CreateCommandList(0, D3D12XBOX_COMMAND_LIST_TYPE_DMA, 
                    dmaEngine.m_commandAllocatorDma.Get(), 
                    nullptr, 
                    IID_GRAPHICS_PPV_ARGS(dmaEngine.m_commandListDma.ReleaseAndGetAddressOf())));
                SET_NAME_TO_SELF(dmaEngine.m_commandListDma);

                D3D12XBOX_COMMAND_QUEUE_DESC descCommandQueue =
                {
                    D3D12XBOX_COMMAND_LIST_TYPE_DMA,                    // D3D12_COMMAND_LIST_TYPE Type;
                    D3D12_COMMAND_QUEUE_FLAG_NONE,                      // D3D12_COMMAND_QUEUE_FLAGS Flags;
                    dmaIndex,                                           // UINT EngineOrPipeIndex;
                    0U,                                                 // UINT QueueIndex;
                };
                DX::ThrowIfFailed(device->CreateCommandQueueX(&descCommandQueue, 
                    IID_GRAPHICS_PPV_ARGS(dmaEngine.m_commandQueueDma.ReleaseAndGetAddressOf())));
                SET_NAME_TO_SELF(dmaEngine.m_commandQueueDma);
            }
        }

        // Allocate GPU memory for manual fences. 
        // DMA requires 32-byte alignment.
        using Fence = uint64_t;
        constexpr uint32_t dmaWritebackAlignment = 32;
#pragma warning(push)
#pragma warning(disable:4324)   // structure was padded due to alignment specifier
        struct alignas(dmaWritebackAlignment) PaddedFence { Fence fence; };
#pragma warning(pop)
        static_assert(sizeof(PaddedFence) == dmaWritebackAlignment, "Not getting expected alignment");
        static_assert(alignof(PaddedFence) == dmaWritebackAlignment, "Not getting expected alignment");
        static_assert(alignof(PaddedFence[1]) == dmaWritebackAlignment, "Not getting expected alignment");
        auto allocationSizeInBytes = (2U * DmaTest::m_numDmaEngines) * sizeof(PaddedFence);
        m_fenceAddress = XMemVirtualAlloc(nullptr,
            allocationSizeInBytes,
            MEM_RESERVE | MEM_COMMIT | MEM_64K_PAGES,
            XMEM_GRAPHICS, 
            PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_READWRITE); 
        if (nullptr == m_fenceAddress)
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            return; // Unreachable, but satisfies code analysis
        }
        ZeroMemory(m_fenceAddress, allocationSizeInBytes);
        auto paddedFenceAddress = reinterpret_cast<PaddedFence*>(m_fenceAddress);
        for (auto dmaIndex = 0U; dmaIndex < DmaTest::m_numDmaEngines; ++dmaIndex)
        {
            if (paddedFenceAddress) {
                auto& dmaEngine = DmaTest::m_dmaEngines[dmaIndex];
                dmaEngine.m_fenceDmaToGfx = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(&paddedFenceAddress[0U * DmaTest::m_numDmaEngines + dmaIndex]);
                SET_FENCE_NAME_TO_SELF(device, dmaEngine.m_fenceDmaToGfx);
                assert(dmaEngine.m_fenceDmaToGfx % dmaWritebackAlignment == 0U);
                dmaEngine.m_fenceValueDmaToGfx = 0ULL;

                dmaEngine.m_fenceGfxToDma = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(&paddedFenceAddress[1U * DmaTest::m_numDmaEngines + dmaIndex]);
                SET_FENCE_NAME_TO_SELF(device, dmaEngine.m_fenceGfxToDma);
                assert(dmaEngine.m_fenceGfxToDma % dmaWritebackAlignment == 0U);
                dmaEngine.m_fenceValueGfxToDma = 0ULL;
            }
        }

        DmaTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        for (auto dmaIndex = 0U; dmaIndex < _countof(DmaTest::m_dmaEngines); ++dmaIndex)
        {
            auto& dmaEngine = DmaTest::m_dmaEngines[dmaIndex];

            dmaEngine.m_commandAllocatorDma.Reset();
            dmaEngine.m_commandListDma.Reset();
            dmaEngine.m_commandQueueDma.Reset();

            dmaEngine.m_fenceDmaToGfx = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
            dmaEngine.m_fenceGfxToDma = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
            dmaEngine.m_fenceValueDmaToGfx = 0ULL;
            dmaEngine.m_fenceValueGfxToDma = 0ULL;
        }

        auto success = VirtualFree(m_fenceAddress, 0ULL, MEM_RELEASE);
        if (!success)
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        m_fenceAddress = nullptr;

        delete m_scopedDebugFlags;
        m_scopedDebugFlags = nullptr;
    }

private:
    ScopedDebugFlags*                                   m_scopedDebugFlags;

    void*                                               m_fenceAddress;

    class DmaTest : public Test
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

        union DmaCombo
        {
            DmaCombo(uint32_t engine1, uint32_t engine2, uint32_t engine3) 
            {
                m_engine[0] = 0; // reserved for system
                m_engine[1] = engine1;
                m_engine[2] = engine2;
                m_engine[3] = engine3;
            }

            uint32_t                                    m_engine[4];

            uint32_t operator[] (uint32_t index) const { return m_engine[index];  }
        };

        static const std::vector<DmaCombo> m_allDmaCombos;

        DmaTest() :
            m_resourceAllocator()
        {
        }

        virtual ~DmaTest()
        {
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Test", L"", 16);
            report->AddColumn(L"DMA units", L"", 8);
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
        float GetGBPerSecReadIdeal(DmaCombo dmaCombo, MemoryType type) const 
        {
            const auto& gpuProperties = GpuProperties::Get();
            auto gpuFrequency = g_gpuHardwareConfiguration.GpuFrequency;
            auto numMemoryPathways = (dmaCombo[0] || dmaCombo[2]) + (dmaCombo[1] || dmaCombo[3]);
            auto baseDmaRate = float(numMemoryPathways * gpuProperties.m_bytesPerSClkDma * gpuFrequency) / GB;
            switch (type)
            {
            case MEMORY_TYPE_GARLIC:
                return std::min(baseDmaRate, float(gpuProperties.m_bytesPerMemTransGddr * gpuProperties.m_memTransPerSecondGddr) / GB);
            case MEMORY_TYPE_ONION:
                return std::min(baseDmaRate, float(gpuProperties.m_bytesPerNClkOnionRead * gpuProperties.m_nClkPerSecondNorthbridge) / GB);
            case MEMORY_TYPE_ESRAM:
                return std::min(baseDmaRate, float(gpuProperties.m_bytesPerSClkEsram * gpuFrequency) / GB);
            default:
                return 0.0f;
            }
        }

        float GetGBPerSecWrittenIdeal(DmaCombo dmaCombo, MemoryType type) const
        {
            const auto& gpuProperties = GpuProperties::Get();
            auto gpuFrequency = g_gpuHardwareConfiguration.GpuFrequency;
            auto numMemoryPathways = (dmaCombo[0] || dmaCombo[2]) + (dmaCombo[1] || dmaCombo[3]);
            auto baseDmaRate = float(numMemoryPathways * gpuProperties.m_bytesPerSClkDma * gpuFrequency) / GB;
            switch (type)
            {
            case MEMORY_TYPE_GARLIC:
                return std::min(baseDmaRate, float(gpuProperties.m_bytesPerMemTransGddr * gpuProperties.m_memTransPerSecondGddr) / GB);
            case MEMORY_TYPE_ONION:
                return std::min(baseDmaRate, float(gpuProperties.m_bytesPerNClkOnionWrite * gpuProperties.m_nClkPerSecondNorthbridge) / GB);
            case MEMORY_TYPE_ESRAM:
                return std::min(baseDmaRate, float(gpuProperties.m_bytesPerSClkEsram * gpuFrequency) / GB);
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

    protected:
        friend class DmaBenchmark;
        static constexpr uint32_t               m_numDmaEngines = 4;
        typedef struct
        {
            ComPtr<ID3D12CommandAllocator>      m_commandAllocatorDma;
            ComPtr<ID3D12XboxDmaCommandList>    m_commandListDma;
            ComPtr<ID3D12CommandQueue>          m_commandQueueDma;

            // ID3D12Fences require access to the graphics command queue, which we don't have.
            // So we use manual fences instead.
            D3D12_GPU_VIRTUAL_ADDRESS           m_fenceGfxToDma = 0;
            D3D12_GPU_VIRTUAL_ADDRESS           m_fenceDmaToGfx = 0;

            // DMA engines only support "GREATER_EQUAL" on waits, so we need increasing fence values.
            uint64_t                            m_fenceValueGfxToDma = 0;
            uint64_t                            m_fenceValueDmaToGfx = 0;
        }
        DmaEngines[m_numDmaEngines];
        
        static DmaEngines                       m_dmaEngines;

        GpuBenchAllocator                       m_resourceAllocator;
    };

    class DmaCopyTest final : public DmaTest
    {
    public:
        DmaCopyTest(DmaCombo dmaCombo, const MemoryParams& memoryParamsSrc, const MemoryParams& memoryParamsDst, uint64_t sizeBytes) :
            m_dmaCombo(dmaCombo),
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
            for (auto dmaIndex = 0U; dmaIndex < m_numDmaEngines; ++dmaIndex)
            {
                auto addressSrc = m_resourceAllocator.AllocateResourceMemory(device, &desc, m_memoryParamsSrc.m_type);
                DX::ThrowIfFailed(device->CreatePlacedResourceX(addressSrc,
                    &desc,
                    D3D12_RESOURCE_STATE_COPY_SOURCE,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_src[dmaIndex].ReleaseAndGetAddressOf())));

                auto addressDst = m_resourceAllocator.AllocateResourceMemory(device, &desc, m_memoryParamsDst.m_type);
                DX::ThrowIfFailed(device->CreatePlacedResourceX(addressDst,
                    &desc,
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_dst[dmaIndex].ReleaseAndGetAddressOf())));
            }
        };

        void Uninitialize() override
        {
            for (auto dmaIndex = 0U; dmaIndex < m_numDmaEngines; ++dmaIndex)
            {
                m_resourceAllocator.FreeResourceMemory(m_src[dmaIndex]->GetGPUVirtualAddress(), m_memoryParamsSrc.m_type);
                m_resourceAllocator.FreeResourceMemory(m_dst[dmaIndex]->GetGPUVirtualAddress(), m_memoryParamsDst.m_type);

                m_src[dmaIndex].Reset();
                m_dst[dmaIndex].Reset();
            }
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
            uint32_t numDma = 0;
            std::wostringstream dmaUnits;
            for (auto dmaIndex = 0U; dmaIndex < m_numDmaEngines; ++dmaIndex)
            {
                numDma += m_dmaCombo[dmaIndex];
                dmaUnits << (m_dmaCombo[dmaIndex] ? L'X' : L'_');
            }

            auto timeMs = m_elapsedTime;
            auto bytesExpected = m_sizeBytes * numDma;
            auto bytesRead = GetBytesRead(m_memoryParamsSrc.m_type);
            auto bytesWritten = GetBytesWritten(m_memoryParamsDst.m_type);
            auto gbPerSecRead = (bytesRead / float(GB)) / timeMs * 1000.0f;
            auto gbPerSecWritten = (bytesWritten / float(GB)) / timeMs * 1000.0f;
            auto gbPerSec = gbPerSecRead + gbPerSecWritten;
            auto gbPerSecReadIdeal = GetGBPerSecReadIdeal(m_dmaCombo, m_memoryParamsSrc.m_type);
            auto gbPerSecWrittenIdeal = GetGBPerSecWrittenIdeal(m_dmaCombo, m_memoryParamsDst.m_type);
            auto timeMsReadIdeal = bytesRead ? (bytesRead / float(gbPerSecReadIdeal * GB) * 1000.0f) : 0.0f;
            auto timeMsWrittenIdeal = bytesWritten ? (bytesWritten / float(gbPerSecWrittenIdeal * GB) * 1000.0f) : 0.0f;

            // Garlic is really bound by memory speed, rather than bus speed, and memory has shared read/write bandwidth.
            // Every other case has separate read/write pathways, however it's not always possible to fully exercise both.
            // For example, the GPU can only fully exercise ESRAM if it uses large transactions, and that's not under software control.
            auto parallelBandwidth = !((MEMORY_TYPE_GARLIC == m_memoryParamsSrc.m_type) && (MEMORY_TYPE_GARLIC == m_memoryParamsDst.m_type));
            auto timeMsIdeal = parallelBandwidth ? std::max(timeMsReadIdeal, timeMsWrittenIdeal) : (timeMsReadIdeal + timeMsWrittenIdeal);
            auto gbPerSecIdeal = (bytesRead + bytesWritten) / float(GB) / timeMsIdeal * 1000.0f;

            report->AddRowData(L"CopyResource");
            report->AddRowData(dmaUnits.str());
            report->AddRowData(m_memoryParamsSrc.m_typeName);
            report->AddRowData(m_memoryParamsDst.m_typeName);
            report->AddRowData(timeMs);
            report->AddRowData(2 * bytesExpected);
            report->AddRowData(bytesRead + bytesWritten);
            report->AddRowData(gbPerSecRead + gbPerSecWritten);
            report->AddRowData(100.0f * gbPerSec / gbPerSecIdeal);

            auto dmaCount = 0UL;
            for (auto dmaIndex = 0U; dmaIndex < _countof(DmaTest::m_dmaEngines); ++dmaIndex)
            {
                dmaCount += m_dmaCombo[dmaIndex];
            }
            DirectX::XMVECTOR color;
            switch (dmaCount)
            {
            case 1:
                color = DirectX::Colors::Tan;
                break;
            case 2:
                color = DirectX::Colors::Silver;
                break;
            case 3:
            default:
                color = DirectX::Colors::Wheat;
                break;
            }
            report->EndRow(color);
        }

        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            // Reset fences 
            // - fence writes must be Executed now, or we will think the GPU is hung
            // - all engines wait in Run() until they see the clear value before proceeding
            for (auto dmaIndex = 0U; dmaIndex < m_numDmaEngines; ++dmaIndex)
            {
                if (m_dmaCombo[dmaIndex])
                {
                    auto& dmaEngine = m_dmaEngines[dmaIndex];

                    commandList->Write64BitValueBottomOfPipeX(dmaEngine.m_fenceGfxToDma, ++dmaEngine.m_fenceValueGfxToDma, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);

                    dmaEngine.m_commandListDma->Write64BitValueBottomOfPipeX(dmaEngine.m_fenceDmaToGfx, ++dmaEngine.m_fenceValueDmaToGfx, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);

                    DX::ThrowIfFailed(dmaEngine.m_commandListDma->Close());
                    dmaEngine.m_commandQueueDma->ExecuteCommandLists(1U, CommandListCast(dmaEngine.m_commandListDma.GetAddressOf()));
                }
            }
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            for (auto dmaIndex = 0U; dmaIndex < m_numDmaEngines; ++dmaIndex)
            {
                if (m_dmaCombo[dmaIndex])
                {
                    auto& dmaEngine = m_dmaEngines[dmaIndex];

                    // Wait for completion of the dma queue before calling reset on the command allocator
                    while (*reinterpret_cast<uint64_t*>(dmaEngine.m_fenceDmaToGfx) < dmaEngine.m_fenceValueDmaToGfx);

                    DX::ThrowIfFailed(dmaEngine.m_commandAllocatorDma->Reset());
                    DX::ThrowIfFailed(dmaEngine.m_commandListDma->Reset(dmaEngine.m_commandAllocatorDma.Get(), nullptr));

                    // Wait until all engines see the "reset" signal
                    commandList->Wait64BitValueX(dmaEngine.m_fenceDmaToGfx, D3D12_COMPARISON_FUNC_GREATER_EQUAL, dmaEngine.m_fenceValueDmaToGfx, D3D12XBOX_WAIT_FLAG_NONE);
                    dmaEngine.m_commandListDma->Wait64BitValueX(dmaEngine.m_fenceGfxToDma, D3D12_COMPARISON_FUNC_GREATER_EQUAL, dmaEngine.m_fenceValueGfxToDma, D3D12XBOX_WAIT_FLAG_NONE);

                    // Signal that data collection has started on the graphics pipe
                    commandList->Write64BitValueBottomOfPipeX(dmaEngine.m_fenceGfxToDma, ++dmaEngine.m_fenceValueGfxToDma, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);
                }
            }

            for (auto dmaIndex = 0U; dmaIndex < m_numDmaEngines; ++dmaIndex)
            {
                if (m_dmaCombo[dmaIndex])
                {
                    auto& dmaEngine = m_dmaEngines[dmaIndex];

                    // Block dma activity until after data collection has started on the graphics pipe
                    dmaEngine.m_commandListDma->Wait64BitValueX(dmaEngine.m_fenceGfxToDma, D3D12_COMPARISON_FUNC_GREATER_EQUAL, dmaEngine.m_fenceValueGfxToDma, D3D12XBOX_WAIT_FLAG_NONE);

                    // Use dma to copy
                    dmaEngine.m_commandListDma->CopyResource(m_dst[dmaIndex].Get(), m_src[dmaIndex].Get());

                    // Signal that dma activity is finished
                    dmaEngine.m_commandListDma->Write64BitValueBottomOfPipeX(dmaEngine.m_fenceDmaToGfx, ++dmaEngine.m_fenceValueDmaToGfx, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);

                    // Submit the dma work
                    DX::ThrowIfFailed(dmaEngine.m_commandListDma->Close());
                    dmaEngine.m_commandQueueDma->ExecuteCommandLists(1U, CommandListCast(dmaEngine.m_commandListDma.GetAddressOf()));
                }
            }

            for (auto dmaIndex = 0U; dmaIndex < m_numDmaEngines; ++dmaIndex)
            {
                if (m_dmaCombo[dmaIndex])
                {
                    auto& dmaEngine = m_dmaEngines[dmaIndex];

                    // Wait until dma activity is finished before completing data collection on the graphics pipe
                    commandList->Wait64BitValueX(dmaEngine.m_fenceDmaToGfx, D3D12_COMPARISON_FUNC_GREATER_EQUAL, dmaEngine.m_fenceValueDmaToGfx, D3D12XBOX_WAIT_FLAG_NONE);
                }
            }
        }

        void Stop(ID3D12GraphicsCommandList* /*commandList*/) const override
        {
            for (auto dmaIndex = 0U; dmaIndex < m_numDmaEngines; ++dmaIndex)
            {
                if (m_dmaCombo[dmaIndex])
                {
                    auto& dmaEngine = m_dmaEngines[dmaIndex];

                    DX::ThrowIfFailed(dmaEngine.m_commandAllocatorDma->Reset());
                    DX::ThrowIfFailed(dmaEngine.m_commandListDma->Reset(dmaEngine.m_commandAllocatorDma.Get(), nullptr));
                }
            }
        }

    private:
        ComPtr<ID3D12Resource>          m_src[m_numDmaEngines];
        ComPtr<ID3D12Resource>          m_dst[m_numDmaEngines];

        DmaCombo                        m_dmaCombo;
        
        MemoryParams                    m_memoryParamsSrc;
        MemoryParams                    m_memoryParamsDst;

        uint64_t                        m_sizeBytes;
    };
};

const std::vector<DmaBenchmark::DmaTest::MemoryParams> DmaBenchmark::DmaTest::m_allMemoryParams =
{
    { MEMORY_TYPE_GARLIC,  L"Garlic",  },
    { MEMORY_TYPE_ONION,   L"Onion",   },
    { MEMORY_TYPE_ESRAM,   L"ESRAM",   },
};

const std::vector<uint64_t> DmaBenchmark::DmaTest::m_allSizeBytesDurango = 
{
    4 * MB, 
};

const std::vector<uint64_t> DmaBenchmark::DmaTest::m_allSizeBytesScorpio = 
{
    16 * MB, 
};

const std::vector<DmaBenchmark::DmaTest::DmaCombo> DmaBenchmark::DmaTest::m_allDmaCombos =
{
    DmaCombo(1, 0, 0),
    DmaCombo(0, 1, 0),
    DmaCombo(0, 0, 1),
    DmaCombo(1, 1, 0),
    DmaCombo(1, 0, 1),
    DmaCombo(0, 1, 1),
    DmaCombo(1, 1, 1),
};

DmaBenchmark::DmaTest::DmaEngines DmaBenchmark::DmaTest::m_dmaEngines;

DmaBenchmark benchmark;

