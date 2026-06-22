//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class DepthBenchmark final : public Benchmark
{
public:
    DepthBenchmark() = default;

    ~DepthBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Depth";
    }

    void Initialize(ID3D12Device* device) override
    {
        for (auto format : {DXGI_FORMAT_D16_UNORM, DXGI_FORMAT_D32_FLOAT,})
        {
            for (auto outcome : {DepthTest::DEPTH_TEST_TILE_REJECT,
                DepthTest::DEPTH_TEST_TILE_ACCEPT,
                DepthTest::DEPTH_TEST_EARLY_REJECT,
                DepthTest::DEPTH_TEST_LATE_REJECT,
                DepthTest::DEPTH_TEST_ACCEPT,
                DepthTest::DEPTH_TEST_WRITE_ALWAYS})
            {
                AddTest(new DepthTest(outcome, format));
            }
        }

#if defined(_GAMING_XBOX_SCARLETT)
        AddCounter(GPUPerfCounters::DB_PERF_SEL_SC_DB_tile_tiles);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_DB_SC_tile_hier_kill);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_DB_SC_tile_tile_rate);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_PreZ_Samples_passing_Z);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_PostZ_Samples_passing_Z);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_PreZ_Samples_failing_Z);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_PostZ_Samples_failing_Z);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_tile_rd_sends);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_tile_wr_sends);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_quad_rd_32byte_reqs);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_quad_wr_acks);
#elif defined(_GAMING_XBOX_XBOXONE)
        AddCounter(GPUPerfCounters::DB_PERF_SEL_SC_DB_TILE_TILES);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_DB_SC_TILE_HIER_KILL);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_DB_SC_TILE_TILE_RATE);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_PREZ_SAMPLES_PASSING_Z);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_POSTZ_SAMPLES_PASSING_Z);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_PREZ_SAMPLES_FAILING_Z);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_POSTZ_SAMPLES_FAILING_Z);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_TILE_RD_SENDS);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_TILE_WR_SENDS);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_QUAD_RD_32BYTE_REQS);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_QUAD_WR_ACKS);
        // Use this to detect display bandwidth
        //AddCounter(GPUPerfCounters::MC_CITF_PERF_MCC_MCB_READ_RETURN_EOP_MATCHING_CID);
#else // #if defined(_GAMING_DESKTOP)
        const auto & gpuProps = GpuProperties::Get();
        AddCounter_ElapsedTime();
        if (gpuProps.IsSupportedNvidiaGpu())
        {
            // PROP:
            //      The Pre-ROP unit orchestrates the flow of depth and color pixels (fragments)
            //      and samples, for final output. PROP enforces the API ordering of pixel shading,
            //      depth testing, and color blending. Early-Z and Late-Z modes are handled in PROP.
            //
            // ZROP:
            //      The Depth Raster Operation unit performs depth tests, stencil tests,
            //      and depth/stencil buffer updates.
            //
            AddCounter("prop__earlyz_output_samples.sum"); // # of EarlyZ samples output by ZROP
            AddCounter("prop__earlyz_killed_samples.sum"); // # of EarlyZ samples killed by ZROP

            // NOTE: None of the below counters work for TU 116 (GTX 1660Ti MaxQ)
            AddCounter("prop__latez_output_samples.sum"); // # of LateZ samples output by ZROP
            AddCounter("prop__latez_killed_samples.sum"); // # of LateZ killed by ZROP

            // RASTER:
            //          The Raster units receives primitives from the world pipe,
            //          and outputs pixels (fragments) and samples (coverage masks) for the PROP,
            //          Pixel Shader, and ROP to process.
            //
            AddCounter("raster__zcull_input_samples_op_rejected.sum"); // # of samples discarded by ZCULL
            AddCounter("raster__zcull_input_samples_op_accepted.sum"); // # of samples output by ZCULL
            AddCounter("raster__zcull_input_samples_op_accepted_reason_trivial_accept.sum"); // # of samples output by ZCULL, trivially accepted
            AddCounter("raster__zcull_input_samples_op_rejected_reason_depth_test.sum"); // # of samples discarded by ZCULL, failing depth test (non-exclusively)

            // FBPA:
            //      The FrameBuffer Partition is a memory controller which sits between
            //      the level 2 cache (LTC) and the DRAM. The number of FBPAs varies across GPUs.
            //
            AddCounter("fbpa__dram_read_bytes.sum"); // # of DRAM read bytes
            AddCounter("fbpa__dram_write_bytes.sum"); // # of DRAM write bytes

            AddCounter("gpu__cycles_elapsed.sum");
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            AddCounter("HiZTilesAcceptedCount"); // maps to SC_DB_TILE_TILES - DB_SC_TILE_CULLED
            AddCounter("HiZTilesRejectedCount"); // maps to DB_SC_TILE_CULLED

            AddCounter("PreZSamplesPassing");   // maps to PREZ_SAMPLES_PASSING_Z
            AddCounter("PostZSamplesPassing");  // maps to POSTZ_SAMPLES_PASSING_Z
            AddCounter("PreZSamplesFailingZ");  // maps to PREZ_SAMPLES_FAILING_Z
            AddCounter("PostZSamplesFailingZ"); // maps to POSTZ_SAMPLES_FAILING_Z

            AddCounter("DBMemRead");    // maps to TILE_RD_SENDS * 256 + QUAD_RD_32BYTE_REQS * 32
            AddCounter("DBMemWritten"); // maps to TILE_RD_SENDS * 32 + QUAD_WR_SENDS * 32

            AddCounter("GPUBusyCycles");
        }
#endif

        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(DepthTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(DepthTest::m_rootSignature);

        DepthTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        DepthTest::m_rootSignature.Reset();
    }

private:
    class DepthTest final : public Test
    {
    public:
        enum DepthTestOutcome : uint32_t
        {
            DEPTH_TEST_TILE_REJECT, 
            DEPTH_TEST_TILE_ACCEPT, 
            DEPTH_TEST_EARLY_REJECT, 
            DEPTH_TEST_LATE_REJECT, 
            DEPTH_TEST_ACCEPT,
            DEPTH_TEST_WRITE_ALWAYS,
        };

        DepthTest(DepthTestOutcome outcome, DXGI_FORMAT format) :
#ifdef _GAMING_XBOX
            m_resourceAllocator(),
#endif
            m_descriptorRtvCpu{},
            m_descriptorDsvCpu{},
            m_viewport{},
            m_scissorRect{},
            m_outcome(outcome),
            m_format(format),
            m_width(4096),
            m_height(4096),
            m_primitives(1)
        {
            if (DEPTH_TEST_TILE_REJECT == m_outcome || DEPTH_TEST_TILE_ACCEPT == m_outcome)
            {
                // This mode is set up to be run multiple passes without changing the outcome.
                // We need to run more than one pass in order to take appreciable time.
                m_primitives = 16;
#ifdef _GAMING_XBOX
                const auto& gpuProperties = GpuProperties::Get();
                uint64_t htileCacheSizeInPixels = gpuProperties.m_numDb * 8192U * 8U * 8U;

                // Keep close to a square size to fit in htile cache
                while (m_width * m_height >= htileCacheSizeInPixels)
                {
                    if (m_width * m_height >= htileCacheSizeInPixels)
                    {
                        m_height /= 2;
                        m_primitives *= 2;
                    }
                    if (m_width * m_height >= htileCacheSizeInPixels)
                    {
                        m_width /= 2;
                        m_primitives *= 2;
                    }
                }
                assert(512U > m_primitives);    // the GS divides the depth range into 512U slices
#endif // #ifdef _GAMING_XBOX
            }

#ifdef _GAMING_XBOX
            if (IsDurangoClass())
            {
                switch (m_outcome)
                {
                case DEPTH_TEST_TILE_ACCEPT:
                case DEPTH_TEST_TILE_REJECT:
                    // Leave a little ESRAM for htile
                    m_height -= 128U;
                    break;
                }
                if (DXGI_FORMAT_D32_FLOAT == m_format)
                {
                    m_height /= 2;
                }
            }
#endif // #ifdef _GAMING_XBOX
        }

        const wchar_t* GetOutcomeName() const
        {
            switch (m_outcome)
            {
            case DEPTH_TEST_TILE_REJECT:
                return L"Tile reject";
            case DEPTH_TEST_TILE_ACCEPT:
                return L"Tile accept";
            case DEPTH_TEST_EARLY_REJECT:
                return L"Early reject";
            case DEPTH_TEST_LATE_REJECT:
                return L"Late reject";
            case DEPTH_TEST_ACCEPT:
                return L"Accept";
            case DEPTH_TEST_WRITE_ALWAYS:
                return L"Write always";
            default:
                return L"";
            }
        }

        const wchar_t* GetFormatName() const
        {
            switch (m_format)
            {
            case DXGI_FORMAT_D16_UNORM:
                return L"D16_UNORM";
            case DXGI_FORMAT_D32_FLOAT:
                return L"D32_FLOAT";
            default:
                return L"";
            }
        }

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto geometryShaderBlob = DX::ReadData(L"DepthGs.cso");

            auto depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            switch (m_outcome)
            {
                // Depth is cleared to 0.0f, and our geometry is at 0.5f - 1.0f, increasing for each primitive
            case DEPTH_TEST_TILE_REJECT:
            case DEPTH_TEST_EARLY_REJECT:
            case DEPTH_TEST_LATE_REJECT:
                depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
                break;
            case DEPTH_TEST_TILE_ACCEPT:
            case DEPTH_TEST_ACCEPT:
                depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
                break;
            case DEPTH_TEST_WRITE_ALWAYS:
                depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                break;
            }

            // EARLY_REJECT requires a pixel shader (or else DB will choose late z), but it should never run.
            // LATE_REJECT requires a pixel shader (or else it's nearly the same as early z), and it will run.
            // Other modes run faster without a pixel shader.
            //
            // If the pixel shader has no side effects, the GPU will disable it. So we bind a render target too.
            UINT numRenderTargets = 0U;
            DXGI_FORMAT rtvFormat = DXGI_FORMAT_UNKNOWN;
            std::vector<uint8_t> pixelShaderBlob = {};
            if (DEPTH_TEST_EARLY_REJECT == m_outcome || DEPTH_TEST_LATE_REJECT == m_outcome)
            {
                pixelShaderBlob = DX::ReadData((DEPTH_TEST_LATE_REJECT == m_outcome) ? L"DepthLateZPs.cso" : L"DepthPs.cso");

                numRenderTargets = 1U; 
                rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            }

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
                depthStencilState,                              // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
                {},                                             // D3D12_INPUT_LAYOUT_DESC InputLayout;
                D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,    // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,            // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
                numRenderTargets,                               // UINT NumRenderTargets;
                { rtvFormat, },                                 // DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
                m_format,                                       // DXGI_FORMAT DSVFormat;
                { 
                    1U, 
#ifdef _GAMING_XBOX
                    DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
#else
                    0
#endif
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

            CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

            auto descTexColor = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_R8G8B8A8_UNORM,
                m_width,  
                m_height, 
                1U,
                1U,
                1U,
#ifdef _GAMING_XBOX
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
#else
                0,
#endif
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
                D3D12_HEAP_FLAG_NONE, 
                &descTexColor, 
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_texColor.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_texColor);

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

            device->CreateRenderTargetView(m_texColor.Get(), nullptr, m_descriptorRtvCpu);

            auto flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
#ifdef _GAMING_XBOX
            switch (m_outcome)
            {
                // For per-pixel cases, do not allocate htile
            case DEPTH_TEST_EARLY_REJECT:
            case DEPTH_TEST_LATE_REJECT:
            case DEPTH_TEST_ACCEPT:
            case DEPTH_TEST_WRITE_ALWAYS:
                flags |= D3D12XBOX_RESOURCE_FLAG_DENY_COMPRESSION_DATA | D3D12XBOX_RESOURCE_FLAG_DENY_DEPTH_STENCIL_HTILE;
                break;
            }
#endif // #ifndef _GAMING_XBOX

            auto descTexDepth = CD3DX12_RESOURCE_DESC::Tex2D(
                m_format,
                m_width,  
                m_height, 
                1, 
                1, 
                1, 
                0, 
                flags);

#ifdef _GAMING_XBOX
            auto memoryType = IsDurangoClass() ? MEMORY_TYPE_ESRAM : MEMORY_TYPE_GARLIC;
            auto address = m_resourceAllocator.AllocateResourceMemory(device, &descTexDepth, memoryType, MEM_2MB_PAGES);
            // Try to not be bandwidth bound, by using ESRAM when it's available
            DX::ThrowIfFailed(device->CreatePlacedResourceX(address, 
#else // #ifdef _GAMING_DESKTOP
            D3D12_CLEAR_VALUE clearValue = {};
            clearValue.Format = descTexDepth.Format;

            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE,
#endif // #ifdef _GAMING_XBOX
                &descTexDepth, 
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
#ifdef _GAMING_XBOX
                nullptr,
#else // #ifdef _GAMING_DESKTOP
                &clearValue,
#endif
                IID_GRAPHICS_PPV_ARGS(m_texDepth.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_texDepth);

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

            device->CreateDepthStencilView(m_texDepth.Get(), nullptr, m_descriptorDsvCpu);
        }

        void Uninitialize() override
        {
            m_pipelineState.Reset();

            m_texColor.Reset();
            m_descriptorHeapRtv.Reset();

#ifdef _GAMING_XBOX
            auto memoryType = IsDurangoClass() ? MEMORY_TYPE_ESRAM : MEMORY_TYPE_GARLIC;
            m_resourceAllocator.FreeResourceMemory(m_texDepth->GetGPUVirtualAddress(), memoryType);
#endif

            m_texDepth.Reset();
            m_descriptorHeapDsv.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Format: ";
            name << GetFormatName();
            name << L", Outcome: ";
            name << GetOutcomeName();

            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Format", L"", 10);
            report->AddColumn(L"Outcome", L"", 12);
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Tile pass", L"", 6);
            report->AddColumn(L"Tile fail", L"", 6);
            report->AddColumn(L"Depth pass", L"", 10);
            report->AddColumn(L"Depth fail", L"", 10);
            report->AddColumn(L"Read bytes", L"", 12);
            report->AddColumn(L"Write bytes", L"", 12);
            report->AddColumn(L"Depths/clock", L"", 4, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = GetElapsedTimeMs();
#ifdef _GAMING_XBOX
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;

#ifdef _GAMING_XBOX_SCARLETT
            //auto tiles = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_SC_DB_TILE_TILES);
            auto tilesHiZReject = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_DB_SC_tile_hier_kill);
            auto tilesHiZAccept = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_DB_SC_tile_tile_rate);
            auto earlyZPass = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_PreZ_Samples_passing_Z);
            auto lateZPass = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_PostZ_Samples_passing_Z);
            auto earlyZFail = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_PreZ_Samples_failing_Z);
            auto lateZFail = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_PostZ_Samples_failing_Z);
            auto hTileReadBytes = 256 * GetCounterValue(GPUPerfCounters::DB_PERF_SEL_tile_rd_sends);
            auto hTileWriteBytes = 256 * GetCounterValue(GPUPerfCounters::DB_PERF_SEL_tile_wr_sends);
            auto depthReadBytes = 32 * GetCounterValue(GPUPerfCounters::DB_PERF_SEL_quad_rd_32byte_reqs);
            auto depthWriteBytes = 32 * GetCounterValue(GPUPerfCounters::DB_PERF_SEL_quad_wr_acks);

            // HW counter bug on Scarlett: late z passes include trivial accept passes
            lateZPass -= std::min(lateZPass, tilesHiZAccept * 8U * 8U);
#else
            //auto tiles = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_SC_DB_TILE_TILES);
            auto tilesHiZReject = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_DB_SC_TILE_HIER_KILL);
            auto tilesHiZAccept = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_DB_SC_TILE_TILE_RATE);
            auto earlyZPass = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_PREZ_SAMPLES_PASSING_Z);
            auto lateZPass = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_POSTZ_SAMPLES_PASSING_Z);
            auto earlyZFail = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_PREZ_SAMPLES_FAILING_Z);
            auto lateZFail = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_POSTZ_SAMPLES_FAILING_Z);
            auto hTileReadBytes = 256 * GetCounterValue(GPUPerfCounters::DB_PERF_SEL_TILE_RD_SENDS);
            auto hTileWriteBytes = 256 * GetCounterValue(GPUPerfCounters::DB_PERF_SEL_TILE_WR_SENDS);
            auto depthReadBytes = 32 * GetCounterValue(GPUPerfCounters::DB_PERF_SEL_QUAD_RD_32BYTE_REQS);
            auto depthWriteBytes = 32 * GetCounterValue(GPUPerfCounters::DB_PERF_SEL_QUAD_WR_ACKS);
#endif

            auto totalClocks = timeMs * clockSpeed / 1000;
            auto totalDepths = 64 * (tilesHiZAccept + tilesHiZReject) + earlyZPass + lateZPass + earlyZFail + lateZFail;
            auto totalReadBytes = hTileReadBytes + depthReadBytes;
            auto totalWriteBytes = hTileWriteBytes + depthWriteBytes;

#else // #ifdef _GAMING_DESKTOP
            const auto & gpuProps = GpuProperties::Get();
            auto tilesHiZReject = 0ULL;
            auto tilesHiZAccept = 0ULL;
            auto earlyZPass = 0ULL;
            auto lateZPass = 0ULL;
            auto earlyZFail = 0ULL;
            auto lateZFail = 0ULL;
            auto totalReadBytes = 0ULL;
            auto totalWriteBytes = 0ULL;

            auto totalDepths = 0.0;
            auto totalClocks = 1.0;
            if (gpuProps.IsSupportedNvidiaGpu())
            {
                totalClocks = GetCounterValue("gpu__cycles_elapsed.sum");

                tilesHiZReject = static_cast<uint64_t>(GetCounterValue("raster__zcull_input_samples_op_rejected.sum"));
                tilesHiZAccept = static_cast<uint64_t>(GetCounterValue("raster__zcull_input_samples_op_accepted_reason_trivial_accept.sum"));

                earlyZPass = static_cast<uint64_t>(GetCounterValue("prop__earlyz_output_samples.sum"));
                lateZPass = static_cast<uint64_t>(GetCounterValue("prop__latez_output_samples.sum"));

                earlyZFail = static_cast<uint64_t>(GetCounterValue("prop__earlyz_killed_samples.sum"));
                lateZFail = static_cast<uint64_t>(GetCounterValue("prop__latez_killed_samples.sum"));

                totalReadBytes = static_cast<uint64_t>(GetCounterValue("fbpa__dram_read_bytes.sum"));
                totalWriteBytes = static_cast<uint64_t>(GetCounterValue("fbpa__dram_write_bytes.sum"));
            }
            else if (gpuProps.IsSupportedAmdGpu())
            {
                totalClocks = GetCounterValue("GPUBusyCycles");

                tilesHiZReject = static_cast<uint64_t>(GetCounterValue("HiZTilesRejectedCount"));
                tilesHiZAccept = static_cast<uint64_t>(GetCounterValue("HiZTilesAcceptedCount"));

                earlyZPass = static_cast<uint64_t>(GetCounterValue("PreZSamplesPassing"));
                lateZPass = static_cast<uint64_t>(GetCounterValue("PostZSamplesPassing"));

                earlyZFail = static_cast<uint64_t>(GetCounterValue("PreZSamplesFailingZ"));
                lateZFail = static_cast<uint64_t>(GetCounterValue("PostZSamplesFailingZ"));

                totalDepths = static_cast<double>(64 * (tilesHiZAccept + tilesHiZReject) + earlyZPass + lateZPass + earlyZFail + lateZFail);

                totalReadBytes = static_cast<uint64_t>(GetCounterValue("DBMemRead"));
                totalWriteBytes = static_cast<uint64_t>(GetCounterValue("DBMemWritten"));
            }
#endif

            report->AddRowData(GetFormatName());
            report->AddRowData(GetOutcomeName());
            report->AddRowData(timeMs);
            report->AddRowData(tilesHiZAccept);
            report->AddRowData(tilesHiZReject);
            report->AddRowData(earlyZPass + lateZPass);
            report->AddRowData(earlyZFail + lateZFail);
            report->AddRowData(totalReadBytes);
            report->AddRowData(totalWriteBytes);
            report->AddRowData(totalDepths / totalClocks);

            DirectX::XMVECTOR color;
            switch (m_format)
            {
            case DXGI_FORMAT_D16_UNORM:
                color = DirectX::Colors::Tan;
                break;
            case DXGI_FORMAT_D32_FLOAT:
            default:
                color = DirectX::Colors::White;
                break;
            }
            report->EndRow(color);
        }

        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            // No barriers needed, because we are already in DEPTH_WRITE state, and Start(...) is isolated from Run(...)
            commandList->ClearDepthStencilView(m_descriptorDsvCpu, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0U, 0U, nullptr);
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

            commandList->RSSetViewports(1, &m_viewport);
            commandList->RSSetScissorRects(1, &m_scissorRect);

            if (DEPTH_TEST_EARLY_REJECT == m_outcome || DEPTH_TEST_LATE_REJECT == m_outcome)
            {
#ifdef _GAMING_XBOX
                commandList->SetPixelShaderDepthForceZOrderX(DEPTH_TEST_LATE_REJECT == m_outcome);
#endif
                commandList->OMSetRenderTargets(1U, &m_descriptorRtvCpu, FALSE, &m_descriptorDsvCpu);
            }
            else
            {
                commandList->OMSetRenderTargets(0U, nullptr, FALSE, &m_descriptorDsvCpu);
            }

            commandList->DrawInstanced(m_primitives, 1, 0, 0);
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>      m_rootSignature;

    private:
#ifdef _GAMING_XBOX
        // Resources which are unique per-test
        GpuBenchAllocator                       m_resourceAllocator;
#endif

        ComPtr<ID3D12PipelineState>             m_pipelineState;

        ComPtr<ID3D12Resource>                  m_texColor;
        ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapRtv;
        D3D12_CPU_DESCRIPTOR_HANDLE             m_descriptorRtvCpu;

        ComPtr<ID3D12Resource>                  m_texDepth;
        ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapDsv;
        D3D12_CPU_DESCRIPTOR_HANDLE             m_descriptorDsvCpu;

        D3D12_VIEWPORT                          m_viewport;
        D3D12_RECT                              m_scissorRect;

        DepthTestOutcome                                        m_outcome;
        DXGI_FORMAT                                             m_format;

        uint32_t                                                m_width;
        uint32_t                                                m_height;
        uint32_t                                                m_primitives;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature> DepthBenchmark::DepthTest::m_rootSignature;

DepthBenchmark benchmark;

