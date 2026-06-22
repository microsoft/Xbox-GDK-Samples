//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class DepthTileBenchmark final : public Benchmark
{
public:
    DepthTileBenchmark() :
        Benchmark(), 
        m_fenceAddress(nullptr)
    {
    }

    ~DepthTileBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"DepthTile";
    }

    void Initialize(ID3D12Device* device) override
    {
        auto esram = IsDurangoClass();

        for (auto memoryParamsSrc : DepthTileTest::m_allMemoryParams)
        {
            if(!esram && memoryParamsSrc.m_type == MEMORY_TYPE_ESRAM)
            {
                continue;
            }
            for (auto memoryParamsDst : DepthTileTest::m_allMemoryParams)
            {
                if(!esram && memoryParamsDst.m_type == MEMORY_TYPE_ESRAM)
                {
                    continue;
                }
                for (const auto& formatParams : DepthTileTest::m_allFormatParams)
                {
                    for (const auto& tileMode : DepthTileTest::m_allTileModes)
                    {
                        // Some tile modes are incompatible with stencil
                        if (formatParams.m_useStencil && !tileMode.m_supportsStencil)
                        {
                            continue;
                        }

                        AddTest(new DepthCopyTest(tileMode, memoryParamsSrc, memoryParamsDst, formatParams));
                    }
                }
            }
        }

        for (auto memoryParamsSrc : DepthTileTest::m_allMemoryParams)
        {
            if(!esram && memoryParamsSrc.m_type == MEMORY_TYPE_ESRAM)
            {
                continue;
            }
            for (auto memoryParamsDst : DepthTileTest::m_allMemoryParams)
            {
                if(!esram && memoryParamsDst.m_type == MEMORY_TYPE_ESRAM)
                {
                    continue;
                }
                for (const auto& formatParams : DepthTileTest::m_allFormatParams)
                {
                    for (const auto& tileMode : DepthTileTest::m_allTileModes)
                    {
                        // Some tile modes are incompatible with stencil
                        if (formatParams.m_useStencil && !tileMode.m_supportsStencil)
                        {
                            continue;
                        }

                        AddTest(new DepthDmaTest(tileMode, memoryParamsSrc, memoryParamsDst, formatParams));
                    }
                }
            }
        }

        for (auto memoryParams : DepthTileTest::m_allMemoryParams)
        {
            if(!esram && memoryParams.m_type == MEMORY_TYPE_ESRAM)
            {
                continue;
            }
            for (const auto& formatParams : DepthTileTest::m_allFormatParams)
            {
                for (const auto& tileMode : DepthTileTest::m_allTileModes)
                {
                    // Some tile modes are incompatible with stencil
                    if (formatParams.m_useStencil && !tileMode.m_supportsStencil)
                    {
                        continue;
                    }

                    AddTest(new DepthReadSRTest(tileMode, memoryParams, formatParams));
                }
            }
        }

        for (auto memoryParams : DepthTileTest::m_allMemoryParams)
        {
            if (!esram && memoryParams.m_type == MEMORY_TYPE_ESRAM)
            {
                continue;
            }
            for (const auto& formatParams : DepthTileTest::m_allFormatParams)
            {
                for (auto write : {false, true,})
                {
                    for (const auto& tileMode : DepthTileTest::m_allTileModes)
                    {
                        // Some tile modes are incompatible with stencil
                        if (formatParams.m_useStencil && !tileMode.m_supportsStencil)
                        {
                            continue;
                        }

                        AddTest(new DepthWriteDSTest(tileMode, memoryParams, formatParams, write));
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
        //AddCounter(GPUPerfCounters::MC_CITF_PERF_MCC_MCB_READ_RETURN_EOP_MATCHING_CID);

        if (esram)
        {
            // ESRAM
            AddCounter(GPUPerfCounters::MC_ARB_PERF_ESRAM_READ_RESPONSE_DATA_BEATS_CH0);
            AddCounter(GPUPerfCounters::MC_ARB_PERF_ESRAM_READ_RESPONSE_DATA_BEATS_CH1);
            AddCounter(GPUPerfCounters::MC_ARB_PERF_ESRAM_WRITE_RETURN_CH0);
            AddCounter(GPUPerfCounters::MC_ARB_PERF_ESRAM_WRITE_RETURN_CH1);
        }

        auto computeShaderBlob = DX::ReadData(L"DepthTileReadSrCs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            computeShaderBlob.data(),
            computeShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(DepthReadSRTest::m_rootSignatureCompute.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(DepthReadSRTest::m_rootSignatureCompute);

        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(DepthWriteDSTest::m_rootSignatureGraphics.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(DepthWriteDSTest::m_rootSignatureGraphics);

        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12XBOX_COMMAND_LIST_TYPE_DMA, 
            IID_GRAPHICS_PPV_ARGS(DepthDmaTest::m_commandAllocatorDma.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(DepthDmaTest::m_commandAllocatorDma);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12XBOX_COMMAND_LIST_TYPE_DMA, 
            DepthDmaTest::m_commandAllocatorDma.Get(), 
            nullptr, 
            IID_GRAPHICS_PPV_ARGS(DepthDmaTest::m_commandListDma.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(DepthDmaTest::m_commandListDma);

        D3D12_COMMAND_QUEUE_DESC descCommandQueue =
        {
            D3D12XBOX_COMMAND_LIST_TYPE_DMA,                    // D3D12_COMMAND_LIST_TYPE Type;
            D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,                // INT Priority;
            D3D12_COMMAND_QUEUE_FLAG_NONE,                      // D3D12_COMMAND_QUEUE_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateCommandQueue(&descCommandQueue, 
            IID_GRAPHICS_PPV_ARGS(DepthDmaTest::m_commandQueueDma.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(DepthDmaTest::m_commandQueueDma);

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
        m_fenceAddress = XMemVirtualAlloc(nullptr,
            2 * sizeof(PaddedFence),
            MEM_RESERVE | MEM_COMMIT | MEM_64K_PAGES,
            XMEM_GRAPHICS, 
            PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_READWRITE); 
        if (nullptr == m_fenceAddress)
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            return; // Unreachable, but satisfies code analysis
        }
        ZeroMemory(m_fenceAddress, 2 * sizeof(PaddedFence));
        auto paddedFenceAddress = reinterpret_cast<PaddedFence*>(m_fenceAddress);
        if (paddedFenceAddress) {
            DepthDmaTest::m_fenceDmaToGfx = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(&paddedFenceAddress[0]);
            assert(DepthDmaTest::m_fenceDmaToGfx % dmaWritebackAlignment == 0U);
            DepthDmaTest::m_fenceValueDmaToGfx = 0ULL;
            DepthDmaTest::m_fenceGfxToDma = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(&paddedFenceAddress[1]);
            assert(DepthDmaTest::m_fenceGfxToDma % dmaWritebackAlignment == 0U);
            DepthDmaTest::m_fenceValueGfxToDma = 0ULL;
        }

        DepthTileTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        DepthReadSRTest::m_rootSignatureCompute.Reset();
        DepthWriteDSTest::m_rootSignatureGraphics.Reset();

        DepthDmaTest::m_commandAllocatorDma.Reset();
        DepthDmaTest::m_commandListDma.Reset();
        DepthDmaTest::m_commandQueueDma.Reset();

        DepthDmaTest::m_fenceDmaToGfx = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
        DepthDmaTest::m_fenceGfxToDma = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
        DepthDmaTest::m_fenceValueDmaToGfx = 0ULL;
        DepthDmaTest::m_fenceValueGfxToDma = 0ULL;

        auto success = VirtualFree(m_fenceAddress, 0ULL, MEM_RELEASE);
        if (!success)
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        m_fenceAddress = nullptr;
    }

private:
    void*                                               m_fenceAddress;

    class DepthTileTest : public Test
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

        struct TileModeParams
        {
            D3D12_TEXTURE_LAYOUT                        m_tileMode;
            const wchar_t*                              m_tileModeName;
            bool                                        m_supportsStencil;
        };

        struct FormatParams
        {
            const wchar_t*                              m_formatName;
            DXGI_FORMAT                                 m_formatTex;
            DXGI_FORMAT                                 m_formatDsv;
            DXGI_FORMAT                                 m_formatSrv;
            uint32_t                                    m_bytesPerDepth;
            uint32_t                                    m_bytesPerStencil;
            bool                                        m_useStencil;
        };

        static const std::vector<MemoryParams> m_allMemoryParams;
        static const std::vector<TileModeParams> m_allTileModes;
        static const std::vector<FormatParams> m_allFormatParams;

        DepthTileTest() :
            m_resourceAllocator()
        {
        }

        virtual ~DepthTileTest()
        {
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Test", L"", 16);
            report->AddColumn(L"Format", L"", 10);
            report->AddColumn(L"Tile mode", L"", 12);
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

        protected:
            GpuBenchAllocator                                       m_resourceAllocator;
    };

    class DepthCopyTest final : public DepthTileTest
    {
    public:
        DepthCopyTest(const TileModeParams& tileModeParams, const MemoryParams& memoryParamsSrc, const MemoryParams& memoryParamsDst, const FormatParams& formatParams) :
            DepthTileTest(), 
            m_tileModeParams(tileModeParams),
            m_memoryParamsSrc(memoryParamsSrc),
            m_memoryParamsDst(memoryParamsDst),
            m_formatParams(formatParams)
        {
            if (IsScorpioClass())
            {
                m_width = 4096;
                m_height = 3064;
            }
            else
            {
                // Leave room for two resources to fit in ESRAM with both depth and stencil
                m_width = 2048;
                m_height = 1532;
            }
            m_sizeBytes = m_width * m_height * (m_formatParams.m_bytesPerDepth + m_formatParams.m_bytesPerStencil);
        }

        void Initialize(ID3D12Device* device) override
        {
            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
                m_formatParams.m_formatTex,
                m_width,
                m_height,
                1U,
                1U,
                1U, 
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN, 
                D3D12XBOX_RESOURCE_FLAG_DENY_COMPRESSION_DATA, 
                m_tileModeParams.m_tileMode
            );
            auto addressSrc = m_resourceAllocator.AllocateResourceMemory(device, &desc, m_memoryParamsSrc.m_type);
            DX::ThrowIfFailed(device->CreatePlacedResourceX(addressSrc, 
                &desc, 
                D3D12_RESOURCE_STATE_COPY_SOURCE, 
                nullptr, 
                IID_GRAPHICS_PPV_ARGS(m_src.ReleaseAndGetAddressOf())));

            // The D3D12.X driver will perform a raw memory copy when the source and dest are an exact match.
            // This takes tile mode out of the equation, and defeats the purpose of the test.
            // By making a cosmetic change of format, we invoke the tile-mode-aware codepath.
            desc.Format = m_formatParams.m_formatDsv;
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
            name << m_formatParams.m_formatName << L" ";
            name << m_tileModeParams.m_tileModeName << L" ";
            name << L'(' << m_memoryParamsSrc.m_typeName << L"-->" << m_memoryParamsDst.m_typeName << L')';

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;
            auto bytesExpected = 2 * m_sizeBytes;
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
            report->AddRowData(m_formatParams.m_formatName);
            report->AddRowData(m_tileModeParams.m_tileModeName);
            report->AddRowData(m_memoryParamsSrc.m_typeName);
            report->AddRowData(m_memoryParamsDst.m_typeName);
            report->AddRowData(timeMs);
            report->AddRowData(bytesExpected);
            report->AddRowData(bytesRead + bytesWritten);
            report->AddRowData(gbPerSecRead + gbPerSecWritten);
            report->AddRowData(100.0f * gbPerSec / gbPerSecIdeal);

            DirectX::XMVECTOR color;
            switch (m_formatParams.m_formatTex)
            {
            case DXGI_FORMAT_R16_TYPELESS:
                color = DirectX::Colors::Tan;
                break;
            case DXGI_FORMAT_R32_TYPELESS:
                color = DirectX::Colors::White;
                break;
            case DXGI_FORMAT_R32G8X24_TYPELESS:
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

        TileModeParams                  m_tileModeParams;

        MemoryParams                    m_memoryParamsSrc;
        MemoryParams                    m_memoryParamsDst;

        FormatParams                    m_formatParams;

        uint32_t                        m_width;
        uint32_t                        m_height;
        uint64_t                        m_sizeBytes;
    };

    class DepthDmaTest final : public DepthTileTest
    {
    public:
        DepthDmaTest(const TileModeParams& tileModeParams, const MemoryParams& memoryParamsSrc, const MemoryParams& memoryParamsDst, const FormatParams& formatParams) :
            DepthTileTest(), 
            m_scopedDebugFlags(nullptr),
            m_tileModeParams(tileModeParams),
            m_memoryParamsSrc(memoryParamsSrc),
            m_memoryParamsDst(memoryParamsDst),
            m_formatParams(formatParams)
        {
            if (IsScorpioClass())
            {
                m_width = 4096;
                m_height = 3064;
            }
            else
            {
                // Leave room for two resources to fit in ESRAM with both depth and stencil
                m_width = 2048;
                m_height = 1532;
            }
            m_sizeBytes = m_width * m_height * (m_formatParams.m_bytesPerDepth + m_formatParams.m_bytesPerStencil);
        }

        void Initialize(ID3D12Device* device) override
        {
            m_scopedDebugFlags = new ScopedDebugFlags(device, D3D12XBOX_DEBUG_FLAG_DISABLE_BARRIER_VALIDATION, D3D12XBOX_DEBUG_FLAG_NONE);

            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
                m_formatParams.m_formatTex,
                m_width,
                m_height,
                1U,
                1U,
                1U, 
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN, 
                D3D12XBOX_RESOURCE_FLAG_DENY_COMPRESSION_DATA, 
                m_tileModeParams.m_tileMode
            );
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

            m_fenceDmaToGfx = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
            m_fenceGfxToDma = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
            m_fenceValueDmaToGfx = 0ULL;
            m_fenceValueGfxToDma = 0ULL;

            delete m_scopedDebugFlags;
            m_scopedDebugFlags = nullptr;
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"CopyResource DMA ";
            name << m_formatParams.m_formatName << L" ";
            name << m_tileModeParams.m_tileModeName << L" ";
            name << L'(' << m_memoryParamsSrc.m_typeName << L"-->" << m_memoryParamsDst.m_typeName << L')';

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;
            auto bytesExpected = 2 * m_sizeBytes;
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

            report->AddRowData(L"CopyResource DMA");
            report->AddRowData(m_formatParams.m_formatName);
            report->AddRowData(m_tileModeParams.m_tileModeName);
            report->AddRowData(m_memoryParamsSrc.m_typeName);
            report->AddRowData(m_memoryParamsDst.m_typeName);
            report->AddRowData(timeMs);
            report->AddRowData(bytesExpected);
            report->AddRowData(bytesRead + bytesWritten);
            report->AddRowData(gbPerSecRead + gbPerSecWritten);
            report->AddRowData(100.0f * gbPerSec / gbPerSecIdeal);

            DirectX::XMVECTOR color;
            switch (m_formatParams.m_formatTex)
            {
            case DXGI_FORMAT_R16_TYPELESS:
                color = DirectX::Colors::Tan;
                break;
            case DXGI_FORMAT_R32_TYPELESS:
                color = DirectX::Colors::White;
                break;
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            default:
                color = DirectX::Colors::Silver;
                break;
            }
            report->EndRow(color);
        }

        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            // Reset fences 
            // - fence writes must be Executed now, or we will think the GPU is hung
            // - all engines wait in Run() until they see the clear value before proceeding
            commandList->Write64BitValueBottomOfPipeX(m_fenceGfxToDma, ++m_fenceValueGfxToDma, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);

            m_commandListDma->Write64BitValueBottomOfPipeX(m_fenceDmaToGfx, ++m_fenceValueDmaToGfx, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);

            DX::ThrowIfFailed(m_commandListDma->Close());
            m_commandQueueDma->ExecuteCommandLists(1U, CommandListCast(m_commandListDma.GetAddressOf()));
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            DX::ThrowIfFailed(m_commandAllocatorDma->Reset());
            DX::ThrowIfFailed(m_commandListDma->Reset(m_commandAllocatorDma.Get(), nullptr));

            // Block dma activity until after data collection has started on the graphics pipe
            commandList->Write64BitValueBottomOfPipeX(m_fenceGfxToDma, ++m_fenceValueGfxToDma, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);
            m_commandListDma->Wait64BitValueX(m_fenceGfxToDma, D3D12_COMPARISON_FUNC_GREATER_EQUAL, m_fenceValueGfxToDma, D3D12XBOX_WAIT_FLAG_NONE);

            // Use dma to copy
            m_commandListDma->CopyResource(m_dst.Get(), m_src.Get());

            // Wait until dma activity is finished before completing data collection on the graphics pipe
            m_commandListDma->Write64BitValueBottomOfPipeX(m_fenceDmaToGfx, ++m_fenceValueDmaToGfx, D3D12XBOX_FLUSH_NONE, D3D12XBOX_WRITE_VALUE_BOP_FLAG_NONE);
            commandList->Wait64BitValueX(m_fenceDmaToGfx, D3D12_COMPARISON_FUNC_GREATER_EQUAL, m_fenceValueDmaToGfx, D3D12XBOX_WAIT_FLAG_NONE);

            // Submit the dma work
            DX::ThrowIfFailed(m_commandListDma->Close());
            m_commandQueueDma->ExecuteCommandLists(1U, CommandListCast(m_commandListDma.GetAddressOf()));
        }
 
        void Stop(ID3D12GraphicsCommandList* /*commandList*/) const override
        {
            DX::ThrowIfFailed(m_commandAllocatorDma->Reset());
            DX::ThrowIfFailed(m_commandListDma->Reset(m_commandAllocatorDma.Get(), nullptr));
        }

        static ComPtr<ID3D12CommandAllocator>       m_commandAllocatorDma;
        static ComPtr<ID3D12XboxDmaCommandList>     m_commandListDma;
        static ComPtr<ID3D12CommandQueue>           m_commandQueueDma;

        // ID3D12Fences require access to the graphics command queue, which we don't have.
        // So we use manual fences instead.
        static D3D12_GPU_VIRTUAL_ADDRESS            m_fenceGfxToDma;
        static D3D12_GPU_VIRTUAL_ADDRESS            m_fenceDmaToGfx;

        // DMA engines only support "GREATER_EQUAL" on waits, so we need increasing fence values.
        static uint64_t                             m_fenceValueGfxToDma;
        static uint64_t                             m_fenceValueDmaToGfx;

    private:
        ComPtr<ID3D12Resource>                      m_src;
        ComPtr<ID3D12Resource>                      m_dst;

        ScopedDebugFlags*                           m_scopedDebugFlags;

        TileModeParams                              m_tileModeParams;

        MemoryParams                                m_memoryParamsSrc;
        MemoryParams                                m_memoryParamsDst;

        FormatParams                                m_formatParams;

        uint32_t                                    m_width;
        uint32_t                                    m_height;
        uint64_t                                    m_sizeBytes;
    };

    class DepthReadSRTest final : public DepthTileTest
    {
    public:
        DepthReadSRTest(const TileModeParams& tileModeParams, const MemoryParams& memoryParams, const FormatParams& formatParams) :
            DepthTileTest(),
            m_descriptorSrvGpu{},
            m_descriptorUavGpu{},
            m_tileModeParams(tileModeParams),
            m_memoryParams(memoryParams),
            m_formatParams(formatParams)
        {
            if (IsScorpioClass())
            {
                m_width = 4096;
                m_height = 4096;
            }
            else
            {
                // Leave room for a resource to fit in ESRAM with both depth and stencil
                m_width = 3072;
                m_height = 2048;
            }
            m_sizeBytes = m_width * m_height * (m_formatParams.m_useStencil ? m_formatParams.m_bytesPerStencil : m_formatParams.m_bytesPerDepth);
        }

        void Initialize(ID3D12Device* device) override
        {
            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
                m_formatParams.m_formatTex,
                m_width,
                m_height,
                1U,
                1U,
                1U, 
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN, 
                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12XBOX_RESOURCE_FLAG_DENY_COMPRESSION_DATA, 
                m_tileModeParams.m_tileMode
            );
            auto addressSrc = m_resourceAllocator.AllocateResourceMemory(device, &desc, m_memoryParams.m_type);
            DX::ThrowIfFailed(device->CreatePlacedResourceX(addressSrc, 
                &desc, 
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 
                nullptr, 
                IID_GRAPHICS_PPV_ARGS(m_tex.ReleaseAndGetAddressOf())));

            D3D12_DESCRIPTOR_HEAP_DESC descHeap = 
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                1U,                                                 // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,          // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeap, IID_GRAPHICS_PPV_ARGS(m_descriptorHeap.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeap);

            auto descriptorSrvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));
            m_descriptorSrvGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));

            // If format has stencil, then test reading stencil, otherwise test reading depth
            auto PlaneSlice = m_formatParams.m_useStencil ? 1U : 0U;
            D3D12_TEX2D_SRV tex2DSrv =
            {
                0,                                                  // UINT MostDetailedMip;
                UINT(-1),                                           // UINT MipLevels;
                PlaneSlice,                                         // UINT PlaneSlice;
                                                                    // FLOAT ResourceMinLODClamp;
            };
            D3D12_SHADER_RESOURCE_VIEW_DESC descSrv =
            {
                m_formatParams.m_formatSrv,                         // DXGI_FORMAT Format;
                D3D12_SRV_DIMENSION_TEXTURE2D,                      // D3D12_SRV_DIMENSION ViewDimension;
                D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,           // UINT Shader4ComponentMapping;
            };
            descSrv.Texture2D = tex2DSrv;
            device->CreateShaderResourceView(m_tex.Get(), &descSrv, descriptorSrvCpu);

            // Create a null uav descriptor, so that we can have a shader which reads from depth and discards all writes
            auto descriptorUavCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
                1U, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));
            m_descriptorUavGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
                1U, 
                device->GetDescriptorHandleIncrementSize(descHeap.Type));

            NullDescriptor(descriptorUavCpu, device->GetDescriptorHandleIncrementSize(descHeap.Type));

            auto computeShaderBlob = DX::ReadData(L"DepthTileReadSrCs.cso");
            D3D12_COMPUTE_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureCompute.Get(),                          // ID3D12RootSignature* pRootSignature;
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

            m_resourceAllocator.FreeResourceMemory(m_tex->GetGPUVirtualAddress(), m_memoryParams.m_type);

            m_tex.Reset();
            m_descriptorHeap.ReleaseAndGetAddressOf();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"CSReadDepth ";
            name << m_formatParams.m_formatName << L" ";
            name << m_tileModeParams.m_tileModeName << L" ";
            name << L' ' << L'(' << m_memoryParams.m_typeName << L')';

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;
            auto bytesReadExpected = m_sizeBytes;
            auto bytesWrittenExpected = 0ULL;
            auto bytesRead = GetBytesRead(m_memoryParams.m_type);
            auto bytesWritten = GetBytesWritten(m_memoryParams.m_type);
            auto gbPerSecRead = (bytesRead / float(GB)) / timeMs * 1000.0f;
            auto gbPerSecWritten = (bytesWritten / float(GB)) / timeMs * 1000.0f;
            auto gbPerSec = gbPerSecRead + gbPerSecWritten;
            auto gbPerSecReadIdeal = GetGBPerSecReadIdeal(m_memoryParams.m_type);
            auto gbPerSecWrittenIdeal = GetGBPerSecWrittenIdeal(m_memoryParams.m_type);
            auto timeMsReadIdeal = bytesRead ? (bytesRead / float(gbPerSecReadIdeal * GB) * 1000.0f) : 0.0f;
            auto timeMsWrittenIdeal = bytesWritten ? (bytesWritten / float(gbPerSecWrittenIdeal * GB) * 1000.0f) : 0.0f;
            auto timeMsIdeal = timeMsReadIdeal + timeMsWrittenIdeal;
            auto gbPerSecIdeal = (bytesRead + bytesWritten) / float(GB) / timeMsIdeal * 1000.0f;

            report->AddRowData(L"CSReadDepth");
            report->AddRowData(m_formatParams.m_formatName);
            report->AddRowData(m_tileModeParams.m_tileModeName);
            report->AddRowData(m_memoryParams.m_typeName);
            report->AddRowData(L"");
            report->AddRowData(timeMs);
            report->AddRowData(bytesReadExpected + bytesWrittenExpected);
            report->AddRowData(bytesRead + bytesWritten);
            report->AddRowData(gbPerSecRead + gbPerSecWritten);
            report->AddRowData(100.0f * gbPerSec / gbPerSecIdeal);

            DirectX::XMVECTOR color;
            switch (m_formatParams.m_formatTex)
            {
            case DXGI_FORMAT_R16_TYPELESS:
                color = DirectX::Colors::Tan;
                break;
            case DXGI_FORMAT_R32_TYPELESS:
                color = DirectX::Colors::White;
                break;
            case DXGI_FORMAT_R32G8X24_TYPELESS:
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

            commandList->SetComputeRootSignature(m_rootSignatureCompute.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            // Bind depth-tiled srv
            commandList->SetComputeRootDescriptorTable(ROOT_PARAMETER_INDEX_SRV, m_descriptorSrvGpu);

            // Unbind all uavs, so our CS outputs are dropped.
            commandList->SetComputeRootDescriptorTable(ROOT_PARAMETER_INDEX_UAV, m_descriptorUavGpu);

            constexpr uint32_t threadGroupSizeX = 8, threadGroupSizeY = 8;
            commandList->Dispatch(UINT(m_width / threadGroupSizeX), UINT(m_height / threadGroupSizeY), 1);

            // Include the full bandwidth by flushing caches
            commandList->FlushPipelineX(D3D12XBOX_FLUSH_BOP_MASK | D3D12XBOX_FLUSH_TOP_MASK, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>  m_rootSignatureCompute;

    private:
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>         m_pipelineState;

        ComPtr<ID3D12Resource>              m_tex;
        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeap;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorSrvGpu;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorUavGpu; // null descriptor

        TileModeParams                      m_tileModeParams;

        MemoryParams                        m_memoryParams;

        FormatParams                        m_formatParams;

        uint32_t                            m_width;
        uint32_t                            m_height;
        uint64_t                            m_sizeBytes;
    };

    class DepthWriteDSTest final : public DepthTileTest
    {
    public:
        // For multiple instances, we must make sure there is a passing test for each instance, even after previous
        // instances have done a write.  We don't currently bother with that.
        static constexpr uint32_t m_instances = 1;

        DepthWriteDSTest(const TileModeParams& tileModeParams, MemoryParams memoryParams, const FormatParams& formatParams, bool write) :
            DepthTileTest(),
            m_descriptorDsvCpu{},
            m_viewport{},
            m_scissorRect{},
            m_tileModeParams(tileModeParams),
            m_memoryParams(memoryParams),
            m_formatParams(formatParams),
            m_write(write)
        {
            if (IsScorpioClass())
            {
                m_width = 4096;
                m_height = 4096;
            }
            else
            {
                // Leave room for a resource to fit in ESRAM with both depth and stencil
                m_width = 3072;
                m_height = 2048;
            }
            m_sizeBytes = m_width * m_height * (m_formatParams.m_useStencil ? m_formatParams.m_bytesPerStencil : m_formatParams.m_bytesPerDepth);
        }

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto geometryShaderBlob = DX::ReadData(L"FullScreenGs.cso");

            auto depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            if (m_formatParams.m_useStencil)
            {
                // If we want to measure writes, then stencil will always pass, regardless of the current stencil buffer contents
                depthStencilState.DepthEnable = FALSE;
                depthStencilState.StencilEnable = TRUE;
                depthStencilState.StencilReadMask = 0x02;
                depthStencilState.StencilWriteMask = 0x01;
                depthStencilState.FrontFace.StencilFunc = m_write ? D3D12_COMPARISON_FUNC_GREATER_EQUAL : D3D12_COMPARISON_FUNC_LESS;
                depthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
            }
            else
            {
                // If we want to measure writes, then depth will always pass, but the DB will discard redundant writes after one pass
                depthStencilState.DepthEnable = TRUE;
                depthStencilState.StencilEnable = FALSE;
                depthStencilState.DepthFunc = m_write ? D3D12_COMPARISON_FUNC_GREATER_EQUAL : D3D12_COMPARISON_FUNC_LESS;
                depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            }

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignatureGraphics.Get(),                  // ID3D12RootSignature* pRootSignature;
                {
                    vertexShaderBlob.data(),
                    vertexShaderBlob.size(),
                },                                              // D3D12_SHADER_BYTECODE VS;
                {},                                             // D3D12_SHADER_BYTECODE PS;
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
                depthStencilState,                              // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
                {},                                             // D3D12_INPUT_LAYOUT_DESC InputLayout;
                D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,    // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,            // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
                0U,                                             // UINT NumRenderTargets;
                {},                                             // DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
                m_formatParams.m_formatDsv,                     // DXGI_FORMAT DSVFormat;
                { 
                    1U, 
                    DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN, 
                },                                              // DXGI_SAMPLE_DESC SampleDesc;
                0U,                                             // UINT NodeMask;
                {},                                             // D3D12_CACHED_PIPELINE_STATE CachedPSO;
                D3D12_PIPELINE_STATE_FLAG_NONE,                 // D3D12_PIPELINE_STATE_FLAGS Flags;
            };
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineState);

            D3D12_VIEWPORT viewport = 
            {
                0,                                                          // FLOAT TopLeftX;
                0,                                                          // FLOAT TopLeftY;
                FLOAT(m_width),                                             // FLOAT Width;
                FLOAT(m_height),                                            // FLOAT Height;
                0.0f,                                                       // FLOAT MinDepth;
                1.0f,                                                       // FLOAT MaxDepth;
            };
            m_viewport = viewport;

            D3D12_RECT scissorRect =
            {
                0,                                                          // LONG    left;
                0,                                                          // LONG    top;
                LONG(m_width),                                              // LONG    right;
                LONG(m_height),                                             // LONG    bottom;
            };
            m_scissorRect = scissorRect;

            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
                m_formatParams.m_formatTex,
                m_width,
                m_height,
                1U,
                1U,
                1U, 
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN, 
                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12XBOX_RESOURCE_FLAG_DENY_COMPRESSION_DATA, 
                m_tileModeParams.m_tileMode
            );
            auto addressSrc = m_resourceAllocator.AllocateResourceMemory(device, &desc, m_memoryParams.m_type);
            DX::ThrowIfFailed(device->CreatePlacedResourceX(addressSrc, 
                &desc, 
                D3D12_RESOURCE_STATE_DEPTH_WRITE, 
                nullptr, 
                IID_GRAPHICS_PPV_ARGS(m_tex.ReleaseAndGetAddressOf())));

            D3D12_DESCRIPTOR_HEAP_DESC descHeapDsv = 
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_DSV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                1U,                                                 // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapDsv, IID_GRAPHICS_PPV_ARGS(m_descriptorHeapDsv.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeapDsv);

            m_descriptorDsvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapDsv->GetCPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeapDsv.Type));

            D3D12_TEX2D_DSV tex2DDsv =
            {
                                                                    // UINT MipSlice;
            };
            D3D12_DEPTH_STENCIL_VIEW_DESC descDsv =
            {
                m_formatParams.m_formatDsv,                         // DXGI_FORMAT Format;
                D3D12_DSV_DIMENSION_TEXTURE2D,                      // D3D12_DSV_DIMENSION ViewDimension;
                D3D12_DSV_FLAG_NONE,                                // D3D12_DSV_FLAGS Flags;
            };
            descDsv.Texture2D = tex2DDsv;
            device->CreateDepthStencilView(m_tex.Get(), &descDsv, m_descriptorDsvCpu);
        };

        void Uninitialize() override
        {
            m_pipelineState.Reset();

            m_resourceAllocator.FreeResourceMemory(m_tex->GetGPUVirtualAddress(), m_memoryParams.m_type);

            m_tex.Reset();
            m_descriptorHeapDsv.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << (m_write ? L"WriteDepth " : L"TestDepth ");
            name << m_formatParams.m_formatName << L" ";
            name << m_tileModeParams.m_tileModeName << L" ";
            name << L'(' << m_memoryParams.m_typeName << L')';

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;
            auto bytesExpected = (m_write ? 2 : 1) * m_sizeBytes * m_instances;
            auto bytesRead = GetBytesRead(m_memoryParams.m_type);
            auto bytesWritten = GetBytesWritten(m_memoryParams.m_type);
            auto gbPerSecRead = (bytesRead / float(GB)) / timeMs * 1000.0f;
            auto gbPerSecWritten = (bytesWritten / float(GB)) / timeMs * 1000.0f;
            auto gbPerSec = gbPerSecRead + gbPerSecWritten;
            auto gbPerSecReadIdeal = GetGBPerSecReadIdeal(m_memoryParams.m_type);
            auto gbPerSecWrittenIdeal = GetGBPerSecWrittenIdeal(m_memoryParams.m_type);
            auto timeMsReadIdeal = bytesRead ? (bytesRead / float(gbPerSecReadIdeal * GB) * 1000.0f) : 0.0f;
            auto timeMsWrittenIdeal = bytesWritten ? (bytesWritten / float(gbPerSecWrittenIdeal * GB) * 1000.0f) : 0.0f;
            auto timeMsIdeal = timeMsReadIdeal + timeMsWrittenIdeal;
            auto gbPerSecIdeal = (bytesRead + bytesWritten) / float(GB) / timeMsIdeal * 1000.0f;

            report->AddRowData(m_write ? L"WriteDepth " : L"TestDepth ");
            report->AddRowData(m_formatParams.m_formatName);
            report->AddRowData(m_tileModeParams.m_tileModeName);
            report->AddRowData(m_memoryParams.m_typeName);
            report->AddRowData(m_write ? m_memoryParams.m_typeName : L"");
            report->AddRowData(timeMs);
            report->AddRowData(bytesExpected);
            report->AddRowData(bytesRead + bytesWritten);
            report->AddRowData(gbPerSecRead + gbPerSecWritten);
            report->AddRowData(100.0f * gbPerSec / gbPerSecIdeal);

            DirectX::XMVECTOR color;
            switch (m_formatParams.m_formatTex)
            {
            case DXGI_FORMAT_R16_TYPELESS:
                color = DirectX::Colors::Tan;
                break;
            case DXGI_FORMAT_R32_TYPELESS:
                color = DirectX::Colors::White;
                break;
            case DXGI_FORMAT_R32G8X24_TYPELESS:
            default:
                color = DirectX::Colors::Silver;
                break;
            }
            report->EndRow(color);
        }

        // This activity occurs outside the timing brackets
        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            // No barriers needed, because we are already in DEPTH_WRITE state, and Start(...) is isolated from Run(...)
            commandList->ClearDepthStencilView(m_descriptorDsvCpu, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0U, 0U, nullptr);
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            commandList->SetGraphicsRootSignature(m_rootSignatureGraphics.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

            commandList->RSSetViewports(1, &m_viewport);
            commandList->RSSetScissorRects(1, &m_scissorRect);

            commandList->OMSetRenderTargets(0U, nullptr, FALSE, &m_descriptorDsvCpu);

            commandList->DrawInstanced(1, m_instances, 0, 0);

            // Include the full bandwidth by flushing caches
            commandList->FlushPipelineX(D3D12XBOX_FLUSH_BOP_MASK | D3D12XBOX_FLUSH_TOP_MASK, D3D12_GPU_VIRTUAL_ADDRESS_NULL, D3D12XBOX_FLUSH_RANGE_ALL);
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>  m_rootSignatureGraphics;

    private:
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>         m_pipelineState;

        ComPtr<ID3D12Resource>              m_tex;
        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeapDsv;
        D3D12_CPU_DESCRIPTOR_HANDLE         m_descriptorDsvCpu;

        D3D12_VIEWPORT                      m_viewport;
        D3D12_RECT                          m_scissorRect;

        TileModeParams                      m_tileModeParams;

        MemoryParams                        m_memoryParams;

        FormatParams                        m_formatParams;

        bool                                m_write;

        uint32_t                            m_width;
        uint32_t                            m_height;
        uint64_t                            m_sizeBytes;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature> DepthTileBenchmark::DepthReadSRTest::m_rootSignatureCompute;
ComPtr<ID3D12RootSignature> DepthTileBenchmark::DepthWriteDSTest::m_rootSignatureGraphics;

const std::vector<DepthTileBenchmark::DepthTileTest::MemoryParams> DepthTileBenchmark::DepthTileTest::m_allMemoryParams =
{
    { MEMORY_TYPE_GARLIC,  L"Garlic",  },
    { MEMORY_TYPE_ESRAM,   L"ESRAM",   },
};

const std::vector<DepthTileBenchmark::DepthTileTest::TileModeParams> DepthTileBenchmark::DepthTileTest::m_allTileModes = 
{
    { D3D12XBOX_TEXTURE_LAYOUT_TILE_MODE_COMP_DEPTH_0,    L"COMP_DEPTH_0",   true,    },
    { D3D12XBOX_TEXTURE_LAYOUT_TILE_MODE_COMP_DEPTH_1,    L"COMP_DEPTH_1",   false,   },
    { D3D12XBOX_TEXTURE_LAYOUT_TILE_MODE_COMP_DEPTH_2,    L"COMP_DEPTH_2",   false,   },
    { D3D12XBOX_TEXTURE_LAYOUT_TILE_MODE_COMP_DEPTH_3,    L"COMP_DEPTH_3",   false,   },
    { D3D12XBOX_TEXTURE_LAYOUT_TILE_MODE_COMP_DEPTH_4,    L"COMP_DEPTH_4",   false,   },
    { D3D12XBOX_TEXTURE_LAYOUT_TILE_MODE_UNC_DEPTH_5,     L"UNC_DEPTH_5",    true,    },
//    { D3D12XBOX_TEXTURE_LAYOUT_TILE_MODE_UNC_DEPTH_6,     L"UNC_DEPTH_6",    false,   },  // D3D12 doesn't support this tile mode for depth
    { D3D12XBOX_TEXTURE_LAYOUT_TILE_MODE_UNC_DEPTH_7,     L"UNC_DEPTH_7",    false,   },
};

const std::vector<DepthTileBenchmark::DepthTileTest::FormatParams> DepthTileBenchmark::DepthTileTest::m_allFormatParams = 
{
    { L"D32_FLOAT",             DXGI_FORMAT_R32_TYPELESS,       DXGI_FORMAT_D32_FLOAT,            DXGI_FORMAT_R32_FLOAT,            4,  0,  false,  },      
    { L"D16_UNORM",             DXGI_FORMAT_R16_TYPELESS,       DXGI_FORMAT_D16_UNORM,            DXGI_FORMAT_R16_UNORM,            2,  0,  false,  }, 
    { L"D32S8X24",              DXGI_FORMAT_R32G8X24_TYPELESS,  DXGI_FORMAT_D32_FLOAT_S8X24_UINT, DXGI_FORMAT_X24_TYPELESS_G8_UINT, 4,  1,  true,   },      
};

ComPtr<ID3D12CommandAllocator>      DepthTileBenchmark::DepthDmaTest::m_commandAllocatorDma;
ComPtr<ID3D12XboxDmaCommandList>    DepthTileBenchmark::DepthDmaTest::m_commandListDma;
ComPtr<ID3D12CommandQueue>          DepthTileBenchmark::DepthDmaTest::m_commandQueueDma;

// ID3D12Fences require access to the graphics command queue, which we don't have.
// So we use manual fences instead.
D3D12_GPU_VIRTUAL_ADDRESS                           DepthTileBenchmark::DepthDmaTest::m_fenceGfxToDma = D3D12_GPU_VIRTUAL_ADDRESS_NULL; 
D3D12_GPU_VIRTUAL_ADDRESS                           DepthTileBenchmark::DepthDmaTest::m_fenceDmaToGfx = D3D12_GPU_VIRTUAL_ADDRESS_NULL; 

// DMA engines only support "GREATER_EQUAL" on waits, so we need increasing fence values.
uint64_t                                            DepthTileBenchmark::DepthDmaTest::m_fenceValueGfxToDma = 0ULL;
uint64_t                                            DepthTileBenchmark::DepthDmaTest::m_fenceValueDmaToGfx = 0ULL;

DepthTileBenchmark benchmark;

