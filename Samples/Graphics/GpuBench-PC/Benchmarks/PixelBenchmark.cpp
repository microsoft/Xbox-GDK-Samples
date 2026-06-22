//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class PixelBenchmark final : public Benchmark
{
public:
    PixelBenchmark() = default;

    ~PixelBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Pixel";
    }

    void Initialize(ID3D12Device* device) override
    {
#ifdef _GAMING_XBOX
        // Single test for "discard" of all pixels
        AddTest(new PixelTest(PixelBenchmark::PixelTest::m_allFormatParams[0], false, 0, 1, false, false));
#else // #ifdef _GAMING_DESKTOP
        AddTest(new PixelTest(PixelBenchmark::PixelTest::m_allFormatParams[0], false, 0, 1));
#endif

        // Test for all formats, msaa
        for (auto msaa : {1U, 2U, 4U, 8U, })
        {
            for (const auto& formatParams : PixelTest::m_allFormatParams)
            {
                for (auto blend : {false, true,})
                {
                    if (formatParams.m_blendAllowed || !blend)
                    {
#ifdef _GAMING_XBOX
                        for (auto stayInCache : {false, true,})
                        {
                            AddTest(new PixelTest(formatParams, blend, 1U, msaa, stayInCache, false));
                        }
#else // #ifdef _GAMING_DESKTOP
                        // potentially we can replace "stayInCache" on Xbox by iterating over multiple resolutions
                        AddTest(new PixelTest(formatParams, blend, 1U, msaa));
#endif
                    }
                }
            }
        }

        // Test for all formats, mrts
        for (auto mrt = 2U; mrt <= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++mrt)
        {
            for (const auto& formatParams : PixelTest::m_mrtFormatParams)
            {
                for (auto blend : {false, true,})
                {
                    if (formatParams.m_blendAllowed || !blend)
                    {
#ifdef _GAMING_XBOX
                        for (auto stayInCache : {false, true,})
                        {
                            for (auto bankRotation : {false, true,})
                            {
                                if (bankRotation && mrt <= 1)
                                {
                                    continue;   // no point
                                }
                                if (bankRotation)
                                {
                                    continue;
                                }
                                AddTest(new PixelTest(formatParams, blend, mrt, 1U, stayInCache, bankRotation));
                            }
                        }
#else // #ifdef _GAMING_DESKTOP
                        // potentially we can replace "stayInCache" on Xbox by iterating over multiple resolutions
                        AddTest(new PixelTest(formatParams, blend, mrt, 1U));
#endif
                    }
                }
            }
        }
#ifdef _GAMING_XBOX
        AddCounter(GPUPerfCounters::CB_PERF_SEL_DRAWN_PIXEL);
        AddCounter(GPUPerfCounters::CB_PERF_SEL_CC_CACHE_HIT);
        AddCounter(GPUPerfCounters::CB_PERF_SEL_CC_CACHE_SECTOR_MISS);
        AddCounter(GPUPerfCounters::CB_PERF_SEL_CC_MC_READ_REQUEST);
        AddCounter(GPUPerfCounters::CB_PERF_SEL_CC_MC_WRITE_REQUEST);
#ifdef _GAMING_XBOX_SCARLETT
        AddCounter(GPUPerfCounters::DB_PERF_SEL_SX_DB_quad_quads);
#else
        AddCounter(GPUPerfCounters::DB_PERF_SEL_SX_DB_QUAD_QUADS);
#endif
#else // #ifdef _GAMING_DESKTOP
        const auto & gpuProps = GpuProperties::Get();

        AddCounter_ElapsedTime();

        if (gpuProps.IsSupportedNvidiaGpu())
        {
            // LTS: A Level 2 (L2) Cache Slice is a sub-partition of the Level 2 cache.
            //      lts__t refers to its Tag stage.
            //      lts__m refers to its Miss stage.
            //      lts__d refers to its Data stage.
            //
            AddCounter("lts__average_t_sector_srcunit_crop_lookup_hit.ratio"); // proportion of L2 sectors from unit CROP that hit
            AddCounter("lts__average_t_sector_srcunit_crop_lookup_miss.ratio"); // proportion of L2 sectors from unit CROP that miss

            AddCounter("lts__average_t_sector_srcunit_crop.ratio"); // proportion of L2 sectors from unit CROP

            // PROP:
            //      The Pre-ROP unit orchestrates the flow of depth and color pixels (fragments)
            //      and samples, for final output. PROP enforces the API ordering of pixel shading,
            //      depth testing, and color blending. Early-Z and Late-Z modes are handled in PROP.
            //
            // CROP:
            //      The Color Raster Operation unit performs the final color blend
            //      and render-target updates. CROP implements the “advanced blend equation”
            //
            AddCounter("prop__prop2xbar_crop_pixels_realtime.sum"); // # of pixels sent to CROP (GTX 1600 series)
            AddCounter("prop__prop2xbar_crop_pixels_realtime.sum.per_cycle_elapsed");
            AddCounter("prop__prop2xbar_crop_pixels_realtime.sum.pct_of_peak_sustained_elapsed");

            AddCounter("prop__prop2crop_pixels_realtime.sum"); // # of pixels sent to CROP (RTX 3000 series+)
            AddCounter("prop__prop2crop_pixels_realtime.sum.per_cycle_elapsed");
            AddCounter("prop__prop2crop_pixels_realtime.sum.pct_of_peak_sustained_elapsed");

            // FBPA:
            //      The FrameBuffer Partition is a memory controller which sits between
            //      the level 2 cache (LTC) and the DRAM. The number of FBPAs varies across GPUs.
            //
            AddCounter("fbpa__dram_read_bytes.sum"); // # of DRAM read bytes
            AddCounter("fbpa__dram_write_bytes.sum"); // # of DRAM write bytes
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            // GPU Perf API v4.0 doesn't expose CBMemRead/CBMemWritten counters for RDNA3 (RX7xxx series)
            // because they were disabled in v3.16 release due to inconsistent results
            //
            // CBMem{Read,Written} are based on:
            //      RDNA2 : CB_PERF_SEL_CC_MC_{READ,WRITE}_REQUEST
            //      RDNA3 : CB_PERF_SEL_CC_MA_{READ,WRITE}_REQUEST
            //      RDNA4 : CB_PERF_SEL_CC_CRW_GLX_REQ_{READ,WRITE}_REQUEST
            //
            AddCounter("CBMemRead");
            AddCounter("CBMemWritten");

            #define AmdGpu_CbPrefix "CB\\d+_PERF_SEL_"
            #define AmdGpu_DbPrefix "DB\\d+_PERF_SEL_"

            // Because CBMem{Read/Write} are disabled for RDNA3, collect their native counters
            AddCounter(AmdGpu_CbPrefix"CC_MA_READ_REQUEST");
            AddCounter(AmdGpu_CbPrefix"CC_MA_WRITE_REQUEST");

            // As of GPA v4.0 -- exist only for RDNA4 (not clear why)
            AddCounter(AmdGpu_CbPrefix"DRAWN_PIXEL");
            AddCounter(AmdGpu_DbPrefix"SX_DB_QUAD_QUADS");

            AddCounter("GPUBusyCycles");
        }
#endif

        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0,
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(PixelTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(PixelTest::m_rootSignature);

        D3D12_DESCRIPTOR_HEAP_DESC descHeap =
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,          // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeap, IID_GRAPHICS_PPV_ARGS(PixelTest::m_descriptorHeap.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(PixelTest::m_descriptorHeap);

#ifdef _GAMING_XBOX
        // A null descriptor used by the Discard case
        // Without this, the PS has no possible side effects, and the GPU will optimize it away
        NullDescriptor(PixelTest::m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), device->GetDescriptorHandleIncrementSize(descHeap.Type));
#else // #ifdef _GAMING_DESKTOP

        NullDescriptorBufferSrv(PixelTest::m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), device);
#endif

        PixelTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        PixelTest::m_rootSignature.Reset();

        PixelTest::m_descriptorHeap.Reset();
    }

private:
    static constexpr uint32_t                       m_maxMrt = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;

    class PixelTest final : public Test
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

        static constexpr uint32_t m_standardWidth = 1024;
        static constexpr uint32_t m_standardHeight = 1024;

        struct FormatParams
        {
            const wchar_t*                              m_formatName;
            DXGI_FORMAT                                 m_format;
            const wchar_t*                              m_psOutputName;
            uint32_t                                    m_bytesPerPixel;
            bool                                        m_blendAllowed;
            uint32_t                                    m_idealPixelsPerCbPerClock;
            uint32_t                                    m_idealBlendsPerCbPerClock;
        };

        static const std::vector<FormatParams> m_allFormatParams;
        static const std::vector<FormatParams> m_mrtFormatParams;

#ifdef _GAMING_XBOX
        PixelTest(const FormatParams& formatParams, bool blend, uint32_t mrt, uint32_t msaa, bool stayInCache, bool bankRotation) :
            m_resourceAllocator(),
#else // #ifdef _GAMING_DESKTOP
        PixelTest(const FormatParams& formatParams, bool blend, uint32_t mrt, uint32_t msaa) :
#endif
            m_descriptorRtvCpu{},
            m_viewport{},
            m_scissorRect{},
            m_formatParams(formatParams),
            m_blend(blend),
            m_mrt(mrt),
#ifdef _GAMING_XBOX
            m_msaa(msaa),
            m_stayInCache(stayInCache),
            m_bankRotation(bankRotation),
#else // #ifdef _GAMING_DESKTOP
            m_msaa(msaa),
#endif
            m_width(0),
            m_height(0),
            m_rtAlignedSizeBytes(0),
            m_instances(0)
        {
        }

        void Initialize(ID3D12Device* device) override
        {
#ifdef _GAMING_XBOX
            if (m_stayInCache)
            {
                // The CB color cache is 16 KB for each CB on Durpio, 32 KB for each CB on Scarlett.
                // It would be counterproductive to reduce tile size for mrt, because the rts will all alias each other, unless we do bank rotation.
                const auto& gpuProperties = GpuProperties::Get();
                auto cacheTileSizeInPixels = (gpuProperties.m_colorCacheSizeInBytes * gpuProperties.m_numCb) / (m_formatParams.m_bytesPerPixel * m_msaa);
                // For msaa >= 4, we actually need multiple tiles: samples 0-1 fill a tile, samples 2-3 fill another tile, etc.
                auto tileMultiplier = std::max(1U, m_msaa / 2U);
                cacheTileSizeInPixels *= tileMultiplier;
                DWORD logCacheTileSizeInPixels = 0U;
                _BitScanReverse64(&logCacheTileSizeInPixels, cacheTileSizeInPixels);
                auto logCacheTileHeight = logCacheTileSizeInPixels / 2;
                auto logCacheTileWidth = logCacheTileSizeInPixels - logCacheTileHeight;
                auto cacheTileWidth = 1U << logCacheTileWidth;
                auto cacheTileHeight = 1U << logCacheTileHeight;

                // If you exceed the cache size, you'll start to get misses.
                // In that case, the listed Cache % will be low.
                // It's not necessary to fill the cache, but there is some size below which you only hit some of the CBs.
                // In that case, the Stdev numbers will be high.
                // Also, if you get too close to the full cache size, then you get some data evicted and become bandwidth bound.
                // In that case BW numbers will be high (over 200 GB/s on Scorpio).
                // Empirically, this is a happy medium, although the best results depend on format:
                m_width = cacheTileWidth;
                m_height = cacheTileHeight / 4;
            }
            else
#endif // #ifdef _GAMING_XBOX
            {
                m_width = m_standardWidth;
                m_height = m_standardHeight;
            }

            // Fit all MRTs into ESRAM, so that we are less likely to get bound by bandwidth
            auto descTex = CD3DX12_RESOURCE_DESC::Tex2D(
                m_formatParams.m_format,
                m_width,
                m_height,
                1U,
                1U,
                m_msaa,
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
#ifdef _GAMING_XBOX
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12XBOX_RESOURCE_FLAG_DENY_COLOR_COMPRESSION_DATA
#else // #ifdef _GAMING_DESKTOP
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
#endif
            );
            auto allocationInfo = device->GetResourceAllocationInfo(0U, 1U, &descTex);
            m_rtAlignedSizeBytes = DirectX::AlignUp(allocationInfo.SizeInBytes, allocationInfo.Alignment);

#ifdef _GAMING_XBOX
            if (g_esramSizeInBytes < m_mrt * m_rtAlignedSizeBytes)
            {
                // This is just an ad hoc way of making the dimensions probably fit in ESRAM
                auto macroTileHeight = 128U; // Good enough for Scorpio
                m_height = uint32_t((uint64_t(m_height) * g_esramSizeInBytes) / (m_mrt * m_rtAlignedSizeBytes));
                m_height = uint32_t(DirectX::AlignDown(m_height, macroTileHeight));

                descTex.Height = m_height;

                auto allocationInfoAdjusted = device->GetResourceAllocationInfo(0U, 1U, &descTex);
                m_rtAlignedSizeBytes = DirectX::AlignUp(allocationInfoAdjusted.SizeInBytes, allocationInfoAdjusted.Alignment);

                if (g_esramSizeInBytes < m_mrt * m_rtAlignedSizeBytes)
                {
                    throw std::exception("Failed to fit all MRTs in in ESRAM");
                }
            }
#endif // #ifdef _GAMING_XBOX

            auto maxRtAlignedSizeBytes = 16 * m_standardWidth * m_standardHeight;
            auto sizeScale = uint32_t(maxRtAlignedSizeBytes / m_rtAlignedSizeBytes);
            auto mrtScale = m_maxMrt / std::max(1U, m_mrt);
#ifdef _GAMING_XBOX
            auto baseInstanceCount = IsScorpioClass() ? 4 : 1;
#else // #ifdef _GAMING_DESKTOP
            auto baseInstanceCount = 4;
#endif
            m_instances = baseInstanceCount * sizeScale * mrtScale;
            m_instances = std::max(1U, m_instances);

            D3D12_DESCRIPTOR_HEAP_DESC descHeapRtv =
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT,             // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapRtv, IID_GRAPHICS_PPV_ARGS(m_descriptorHeapRtv.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeapRtv);

#ifdef _GAMING_XBOX
            auto memoryType = IsDurangoClass() ? MEMORY_TYPE_ESRAM : MEMORY_TYPE_GARLIC;
#endif
            for (auto rt = 0U; rt < m_mrt; ++rt)
            {
#ifdef _GAMING_XBOX
                auto pageFlag = uint32_t(IsScorpioClass() ? MEM_2MB_PAGES : MEM_64K_PAGES);
                auto address = m_resourceAllocator.AllocateResourceMemory(device, &descTex, memoryType, pageFlag);

                auto descTexRotated = descTex;
                if (m_bankRotation)
                {
#ifdef _GAMING_XBOX_SCARLETT
                    descTexRotated.Layout = D3D12XBOX_BANK_ROTATED_SWIZZLE_MODE(descTexRotated.Layout, rt);
#else
                    descTexRotated.Layout = D3D12XBOX_BANK_ROTATED_TILE_MODE(descTexRotated.Layout, rt);
#endif
                    XGTextureAddressComputer* texComputer = nullptr;
                    DX::ThrowIfFailed(XGCreateTextureComputer((XG_RESOURCE_DESC*)&descTexRotated, &texComputer));

                    XG_RESOURCE_LAYOUT layout = {};
                    DX::ThrowIfFailed(texComputer->GetResourceLayout(&layout));
                    if (false == XGComputeBankRotationAddress((XG_GPU_VIRTUAL_ADDRESS)address, &layout, 0, 0, rt, &address))
                    {
                        throw std::exception("XGComputeBankRotationAddress failed");
                    }
                }

                DX::ThrowIfFailed(device->CreatePlacedResourceX(address,
                    &descTexRotated,
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(m_tex[rt].ReleaseAndGetAddressOf())));
#else // #ifdef _GAMING_DESKTOP
                D3D12_CLEAR_VALUE clearValue = {};
                clearValue.Format   = descTex.Format;
                clearValue.Color[0] = DirectX::Colors::Transparent[0];
                clearValue.Color[1] = DirectX::Colors::Transparent[1];
                clearValue.Color[2] = DirectX::Colors::Transparent[2];
                clearValue.Color[3] = DirectX::Colors::Transparent[3];
                CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
                DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &descTex,
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    &clearValue,
                    IID_GRAPHICS_PPV_ARGS(m_tex[rt].ReleaseAndGetAddressOf())));
#endif

                assert(rt < descHeapRtv.NumDescriptors);
                m_descriptorRtvCpu[rt] = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(),
                    INT(rt),
                    device->GetDescriptorHandleIncrementSize(descHeapRtv.Type));
                device->CreateRenderTargetView(m_tex[rt].Get(), nullptr, m_descriptorRtvCpu[rt]);

#ifdef _GAMING_XBOX
                address += m_rtAlignedSizeBytes;
#endif
            }

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

            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto geometryShaderBlob = DX::ReadData(L"FullScreenGs.cso");

            std::wostringstream pixelShaderName;
            pixelShaderName << L"Pixel";
            if (0 == m_mrt)
            {
                pixelShaderName << L"Discard";
            }
            else
            {
                pixelShaderName << m_mrt << L"x";
                pixelShaderName << m_formatParams.m_psOutputName;
            }
            pixelShaderName << L"Ps.cso";
            auto pixelShaderBlob = DX::ReadData(pixelShaderName.str().c_str());

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
                m_mrt,                                          // UINT NumRenderTargets;
                { DXGI_FORMAT_UNKNOWN, },                       // DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
                DXGI_FORMAT_UNKNOWN,                            // DXGI_FORMAT DSVFormat;
                {
                    m_msaa,
                    DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
                },                                              // DXGI_SAMPLE_DESC SampleDesc;
                0U,                                             // UINT NodeMask;
                {},                                             // D3D12_CACHED_PIPELINE_STATE CachedPSO;
                D3D12_PIPELINE_STATE_FLAG_NONE,                 // D3D12_PIPELINE_STATE_FLAGS Flags;
            };
            for (auto rt = 0U; rt < descPipelineState.NumRenderTargets; ++rt)
            {
                descPipelineState.BlendState.RenderTarget[rt] = blendSubtractive;
                descPipelineState.RTVFormats[rt] = m_formatParams.m_format;
            }
#ifdef _GAMING_DESKTOP
            if (descPipelineState.DSVFormat == DXGI_FORMAT_UNKNOWN)
            {
                descPipelineState.DepthStencilState.DepthEnable = FALSE;
                if (descPipelineState.NumRenderTargets == 0)
                {
                    descPipelineState.SampleDesc.Quality = 0;
                }
            }
#endif

            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineState);
        }

        void Uninitialize() override
        {
            m_pipelineState.Reset();

            for (auto rt = 0U; rt < m_mrt; ++rt)
            {
#ifdef _GAMING_XBOX
                auto memoryType = IsDurangoClass() ? MEMORY_TYPE_ESRAM : MEMORY_TYPE_GARLIC;
                m_resourceAllocator.FreeResourceMemory(m_tex[rt]->GetGPUVirtualAddress(), memoryType);
#endif

                m_tex[rt].Reset();
            }
            m_descriptorHeapRtv.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Format: ";
            name << m_formatParams.m_formatName << L", ";
            name << (m_blend ? L"blend, " : L"opaque, ");
            name << L"mrts = " << m_mrt << L", ";
            name << L"msaa = " << m_msaa << L", ";
#ifdef _GAMING_XBOX
            name << (m_stayInCache ? L"tile" : L"screen");
            name << (m_bankRotation ? L" rotated" : L"");
#endif

            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Format", L"", 19);
            report->AddColumn(L"MRTs", L"", 4);
            report->AddColumn(L"MSAA", L"", 4);
            report->AddColumn(L"Blend", L"", 5);
#ifdef _GAMING_XBOX
            report->AddColumn(L"Size", L"", 6);
            report->AddColumn(L"Bank", L"", 4);
#endif
            report->AddColumn(L"Cache%", L"%", 6, 2);
            report->AddColumn(L"BW", L" GB/s", 6, 2);
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Pixels(API)", L"", 10);
            report->AddColumn(L"Pixels(GPU)", L"", 10);
            report->AddColumn(L"Stdev", L"K", 6, 2);
            report->AddColumn(L"#/clock", L"", 5, 2);
            report->AddColumn(L"% of max", L"%", 6, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            const auto & gpuProperties = GpuProperties::Get();

            auto timeMs = GetElapsedTimeMs();
#ifdef _GAMING_XBOX
            auto cbCacheHit = GetCounterValue(GPUPerfCounters::CB_PERF_SEL_CC_CACHE_HIT);
            auto cbCacheMiss = GetCounterValue(GPUPerfCounters::CB_PERF_SEL_CC_CACHE_SECTOR_MISS);
            auto cbCacheRate = (100.0f * cbCacheHit) / std::max(1ULL, cbCacheHit + cbCacheMiss);
            auto reads32 = GetCounterValue(GPUPerfCounters::CB_PERF_SEL_CC_MC_READ_REQUEST);
            auto writes32 = GetCounterValue(GPUPerfCounters::CB_PERF_SEL_CC_MC_WRITE_REQUEST);
            auto bytes = 32 * reads32 + 32 * writes32;
            auto gbPerSec = (bytes / float(GB)) / timeMs * 1000.0f;
            auto pixelsAPI = m_instances * m_width * m_height * m_mrt;
            auto pixelsGPU = GetCounterValue(GPUPerfCounters::CB_PERF_SEL_DRAWN_PIXEL);
            auto pixelsStd = GetCounterValue(GPUPerfCounters::CB_PERF_SEL_DRAWN_PIXEL, GpuCounter::SHADER_MASK_ALL, CounterValueArrayStd);
#ifdef _GAMING_XBOX_SCARLETT
            auto exportsGPU = 4 * GetCounterValue(GPUPerfCounters::DB_PERF_SEL_SX_DB_quad_quads);
            auto exportsStd = 4 * GetCounterValue(GPUPerfCounters::DB_PERF_SEL_SX_DB_quad_quads, GpuCounter::SHADER_MASK_ALL, CounterValueArrayStd);
#else
            auto exportsGPU = 4 * GetCounterValue(GPUPerfCounters::DB_PERF_SEL_SX_DB_QUAD_QUADS);
            auto exportsStd = 4 * GetCounterValue(GPUPerfCounters::DB_PERF_SEL_SX_DB_QUAD_QUADS, GpuCounter::SHADER_MASK_ALL, CounterValueArrayStd);
#endif //_GAMING_XBOX_SCARLETT
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;
            auto pixelsPerClock = (1000.0f * pixelsGPU) / (timeMs * clockSpeed);
            auto exportsPerClock = (1000.0f * exportsGPU) / (timeMs * clockSpeed);
            auto numCBs = gpuProperties.m_numCb;
            auto pixelsPerClockIdeal = numCBs * (m_blend ? m_formatParams.m_idealBlendsPerCbPerClock : m_formatParams.m_idealPixelsPerCbPerClock);
            auto numDBs = gpuProperties.m_numDb;
            auto numExportsPerDBPerClock = gpuProperties.m_numPixelPerClockPerDb;
            auto exportsPerClockIdeal = numDBs * numExportsPerDBPerClock;

#ifdef _GAMING_XBOX_SCARLETT
            // Some restrictions for RB+ rates:
            // - 32 bpp or less
            // - no blend
            // - no VRS (that's mutually exclusive)
            // - MSAA requires fmask compression
            auto rbPlus = m_formatParams.m_bytesPerPixel <= 4 && m_msaa <= 1 && !m_blend;
            if (rbPlus)
            {
                pixelsPerClockIdeal *= 2;
                exportsPerClockIdeal *= 2;
            }
#endif // _GAMING_XBOX_SCARLETT
            auto exportsThroughput = 100.0f * exportsPerClock / exportsPerClockIdeal;
            auto pixelsThroughput = 100.0f * pixelsPerClock * m_msaa / pixelsPerClockIdeal;

#else // #ifdef _GAMING_DESKTOP
            auto cbCacheRate = 0.0;
            auto bytes = 0.0;
            auto pixelsAPI = 0;
            auto pixelsGPU = 0.0;
            auto pixelsStd = 0;
            auto pixelsPerClock = 0.0;
            auto pixelsThroughput = 0.0;
            auto exportsGPU = 0.0;
            auto exportsStd = 0;
            auto exportsPerClock = 0.0;
            auto exportsThroughput = 0.0;
            if (gpuProperties.IsSupportedNvidiaGpu())
            {
                auto l2HitRatioFromCropReq = GetCounterValue("lts__average_t_sector_srcunit_crop_lookup_hit.ratio");
                //auto l2MissRatioFromCropReq = GetCounterValue("lts__average_t_sector_srcunit_crop_lookup_miss.ratio");
                auto l2RatioFromCropReq = GetCounterValue("lts__average_t_sector_srcunit_crop.ratio");
                cbCacheRate = 100.0 * l2RatioFromCropReq * l2HitRatioFromCropReq;

                exportsGPU = GetCounterValue("prop__prop2xbar_crop_pixels_realtime.sum")
                           + GetCounterValue("prop__prop2crop_pixels_realtime.sum");

                exportsPerClock = std::max(GetCounterValue("prop__prop2xbar_crop_pixels_realtime.sum.per_cycle_elapsed"),
                                           GetCounterValue("prop__prop2crop_pixels_realtime.sum.per_cycle_elapsed"));

                exportsThroughput = std::max(GetCounterValue("prop__prop2xbar_crop_pixels_realtime.sum.pct_of_peak_sustained_elapsed"),
                                             GetCounterValue("prop__prop2crop_pixels_realtime.sum.pct_of_peak_sustained_elapsed"));

                bytes = GetCounterValue("fbpa__dram_read_bytes.sum")
                      + GetCounterValue("fbpa__dram_write_bytes.sum");
            }
            else if (gpuProperties.IsSupportedAmdGpu())
            {
                auto totalClocks = GetCounterValue("GPUBusyCycles");

                auto readBytes = GetCounterValue("CBMemRead")
                               + 32 * GetCounterValue(AmdGpu_CbPrefix"CC_MA_READ_REQUEST");

                auto writeBytes = GetCounterValue("CBMemWritten")
                               + 32 * GetCounterValue(AmdGpu_CbPrefix"CC_MA_WRITE_REQUEST");

                bytes = readBytes + writeBytes;

                pixelsGPU = GetCounterValue(AmdGpu_CbPrefix"DRAWN_PIXEL");
                exportsGPU = GetCounterValue(AmdGpu_DbPrefix"SX_DB_QUAD_QUADS");

                pixelsPerClock = pixelsGPU / totalClocks;
                exportsPerClock = exportsGPU / totalClocks;
            }
            auto gbPerSec = (bytes / double(GB)) / timeMs * 1000.0;
#endif // #ifdef _GAMING_XBOX

            report->AddRowData(m_formatParams.m_formatName);
            report->AddRowData(m_mrt);
            report->AddRowData(m_msaa);
            report->AddRowData(m_blend ? L"yes" : L"no");
#ifdef _GAMING_XBOX
            report->AddRowData(m_stayInCache ? L"tile" : L"screen");
            report->AddRowData(m_bankRotation ? L"rot." : L" -- ");
#endif
            report->AddRowData(cbCacheRate);
            report->AddRowData(gbPerSec);
            report->AddRowData(timeMs);
            report->AddRowData(pixelsAPI);
            if (0 == pixelsGPU)
            {
                report->AddRowData(exportsGPU);
                report->AddRowData(exportsStd / 1000.0f);
                report->AddRowData(exportsPerClock);
                report->AddRowData(exportsThroughput);
            }
            else
            {
                report->AddRowData(pixelsGPU);
                report->AddRowData(pixelsStd / 1000.0f);
                report->AddRowData(pixelsPerClock);
                report->AddRowData(pixelsThroughput);
            }

            report->EndRow();
        }

        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            // Clear all render targets, to avoid accidental fast paths where the write is a nop.
            // We initialize to 0.0f and then write 0.5f.
            for (auto rt = 0U; rt < m_mrt; ++rt)
            {
                commandList->ClearRenderTargetView(m_descriptorRtvCpu[rt], DirectX::Colors::Transparent, 0U, nullptr);
            }
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            ID3D12DescriptorHeap* descriptorHeaps[] =
            {
                m_descriptorHeap.Get(),
            };
            commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

            commandList->OMSetRenderTargets(m_mrt, m_descriptorRtvCpu, FALSE, nullptr);

            commandList->RSSetViewports(1, &m_viewport);
            commandList->RSSetScissorRects(1, &m_scissorRect);

            commandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_INDEX_SRV, m_descriptorHeap->GetGPUDescriptorHandleForHeapStart());

            commandList->DrawInstanced(1, m_instances, 0, 0);
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>      m_rootSignature;

        static ComPtr<ID3D12DescriptorHeap>     m_descriptorHeap;

    private:
#ifdef _GAMING_XBOX
        // Resources which are unique per-test
        GpuBenchAllocator                       m_resourceAllocator;
#endif

        ComPtr<ID3D12PipelineState>             m_pipelineState;

        ComPtr<ID3D12Resource>                  m_tex[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
        ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapRtv;
        D3D12_CPU_DESCRIPTOR_HANDLE             m_descriptorRtvCpu[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
        D3D12_VIEWPORT                          m_viewport;
        D3D12_RECT                              m_scissorRect;

        FormatParams                            m_formatParams;
        bool                                    m_blend;
        uint32_t                                m_mrt;
        uint32_t                                m_msaa;
#ifdef _GAMING_XBOX
        bool                                    m_stayInCache;
        bool                                    m_bankRotation;
#endif

        uint32_t                                m_width;
        uint32_t                                m_height;
        uint64_t                                m_rtAlignedSizeBytes;
        uint32_t                                m_instances;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature>     PixelBenchmark::PixelTest::m_rootSignature;

ComPtr<ID3D12DescriptorHeap>    PixelBenchmark::PixelTest::m_descriptorHeap;

const std::vector<PixelBenchmark::PixelTest::FormatParams> PixelBenchmark::PixelTest::m_allFormatParams =
{
    { L"R8G8B8A8_UNORM",       DXGI_FORMAT_R8G8B8A8_UNORM,      L"float4",  4,  true,   4,  4,  },
    { L"R8G8B8A8_UNORM_SRGB",  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, L"float4",  4,  true,   4,  4,  },
#if defined(_GAMING_XBOX_SCARLETT) /* || defined(_GAMING_DESKTOP) */
    { L"R9G9B9E5_SHAREDEXP",   DXGI_FORMAT_R9G9B9E5_SHAREDEXP,  L"float4",  4,  true,   4,  4,  },
#endif
    { L"R32_FLOAT",            DXGI_FORMAT_R32_FLOAT,           L"float4",  4,  true,   4,  4,  },
    { L"R16G16B16A16_FLOAT",   DXGI_FORMAT_R16G16B16A16_FLOAT,  L"float4",  8,  true,   4,  4,  },
    { L"R16G16B16A16_UNORM",   DXGI_FORMAT_R16G16B16A16_UNORM,  L"float4",  8,  true,   4,  1,  },
#if defined(_GAMING_XBOX_SCARLETT) || defined(_GAMING_DESKTOP)
    { L"R32G32_FLOAT",         DXGI_FORMAT_R32G32_FLOAT,        L"float4",  8,  true,   4,  2,  },
#else
    { L"R32G32_FLOAT",         DXGI_FORMAT_R32G32_FLOAT,        L"float4",  8,  true,   2,  2,  },
#endif
    { L"R32G32B32A32_FLOAT",   DXGI_FORMAT_R32G32B32A32_FLOAT,  L"float4",  16, true,   2,  1,  },
    { L"R32G32B32A32_UINT",    DXGI_FORMAT_R32G32B32A32_UINT,   L"uint4",   16, false,  2,  0,  },
};

const std::vector<PixelBenchmark::PixelTest::FormatParams> PixelBenchmark::PixelTest::m_mrtFormatParams =
{
    { L"R8G8B8A8_UNORM",       DXGI_FORMAT_R8G8B8A8_UNORM,      L"float4",  4,  true,   4,  4,  },
    { L"R16G16B16A16_FLOAT",   DXGI_FORMAT_R16G16B16A16_FLOAT,  L"float4",  8,  true,   4,  4,  },
    { L"R32G32B32A32_FLOAT",   DXGI_FORMAT_R32G32B32A32_FLOAT,  L"float4",  16, true,   2,  1,  },
};

PixelBenchmark benchmark;
