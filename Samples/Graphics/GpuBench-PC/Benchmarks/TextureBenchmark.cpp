//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class TextureBenchmark final : public Benchmark
{
public:
    TextureBenchmark() = default;

    ~TextureBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Texture";
    }

    void Initialize(ID3D12Device* device) override
    {
        // Test for all formats, msaa
        for (auto& formatParams : TextureTest::m_allFormatParams)
        {
#ifdef _GAMING_XBOX_SCARLETT
            AddTest(new TextureTest(formatParams, TextureTest::m_filterParamsLoad));
            AddTest(new TextureTest(formatParams, TextureTest::m_filterParamsLoadD16));
#endif
            AddTest(new TextureTest(formatParams, TextureTest::m_filterParamsGather));
            AddTest(new TextureTest(formatParams, TextureTest::m_filterParamsPoint));
            AddTest(new TextureTest(formatParams, TextureTest::m_filterParamsSamplePointD16));
            AddTest(new TextureTest(formatParams, TextureTest::m_filterParamsSampleLinearD16));
            if (formatParams.m_filterable)
            {
                for (auto& filterParams : TextureTest::m_allFilterParams)
                {
                    AddTest(new TextureTest(formatParams, filterParams));
                }
            }
        }

        // TA only has 2 counter slots, so these counters will slow things down tremendously
#if defined(_GAMING_XBOX_SCARLETT)
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_TA_REQ_READ);
        AddCounter(GPUPerfCounters::TD_PERF_SEL_four_comp_wavefront);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_TEX);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_image_sampler_total_cycles);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_mipmap_lod_0_samples);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_mipmap_lod_1_samples);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_bilin_point_1_cycle_pixels);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_gt1_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_mip_1_cycle_pixels);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_mip_2_cycle_pixels);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_2_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_4_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_6_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_8_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_10_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_12_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_14_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_16_cycle_quads);
        // AddCounter(GPUPerfCounters::TA_PERF_SEL_COLOR_1_CYCLE_PIXELS);
        // AddCounter(GPUPerfCounters::TA_PERF_SEL_COLOR_2_CYCLE_PIXELS);
        // AddCounter(GPUPerfCounters::TA_PERF_SEL_COLOR_3_CYCLE_PIXELS);
        // AddCounter(GPUPerfCounters::TA_PERF_SEL_COLOR_4_CYCLE_PIXELS);
#elif defined(_GAMING_XBOX_XBOXONE)
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_VMEM);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_IMAGE_READ_WAVEFRONTS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_MIPMAP_LOD_0_SAMPLES);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_MIPMAP_LOD_1_SAMPLES);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_BILIN_POINT_1_CYCLE_PIXELS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_GT1_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_MIP_1_CYCLE_PIXELS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_MIP_2_CYCLE_PIXELS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_2_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_4_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_6_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_8_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_10_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_12_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_14_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_16_CYCLE_QUADS);
        // AddCounter(GPUPerfCounters::TA_PERF_SEL_COLOR_1_CYCLE_PIXELS);
        // AddCounter(GPUPerfCounters::TA_PERF_SEL_COLOR_2_CYCLE_PIXELS);
        // AddCounter(GPUPerfCounters::TA_PERF_SEL_COLOR_3_CYCLE_PIXELS);
        // AddCounter(GPUPerfCounters::TA_PERF_SEL_COLOR_4_CYCLE_PIXELS);
#else // defined(_GAMING_DESKTOP)
        AddCounter_ElapsedTime();
        AddCounter_VMemInstructions();
        AddCounter_PsWaveSize();
#endif

        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(TextureTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(TextureTest::m_rootSignature);

        CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        // We require a render target in order to set viewport dimensions.
        // Texture benchmarks must go through the graphics pipeline in order to get automatic derivatives.
        auto descTexOut = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM,
            TextureTest::m_standardWidth,
            TextureTest::m_standardHeight,
            1U,
            1U,
            1U,
            DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
            D3D12_HEAP_FLAG_NONE, 
            &descTexOut, 
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(TextureTest::m_texOut.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(TextureTest::m_texOut);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapRtv = 
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapRtv, IID_GRAPHICS_PPV_ARGS(TextureTest::m_descriptorHeapRtv.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(TextureTest::m_descriptorHeapRtv);

        TextureTest::m_descriptorRtvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(TextureTest::m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(), 
            0U, 
            device->GetDescriptorHandleIncrementSize(descHeapRtv.Type));

        device->CreateRenderTargetView(TextureTest::m_texOut.Get(), nullptr, TextureTest::m_descriptorRtvCpu);

        D3D12_VIEWPORT viewport = 
        {
            0,                                                          // FLOAT TopLeftX;
            0,                                                          // FLOAT TopLeftY;
            FLOAT(TextureTest::m_standardWidth),                        // FLOAT Width;
            FLOAT(TextureTest::m_standardHeight),                       // FLOAT Height;
            0.0f,                                                       // FLOAT MinDepth;
            1.0f,                                                       // FLOAT MaxDepth;
        };
        TextureTest::m_viewport = viewport;

        D3D12_RECT scissorRect =
        {
            0,                                                          // LONG    left;
            0,                                                          // LONG    top;
            LONG(TextureTest::m_standardWidth),                         // LONG    right;
            LONG(TextureTest::m_standardHeight),                        // LONG    bottom;
        };
        TextureTest::m_scissorRect = scissorRect;

        TextureTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        TextureTest::m_rootSignature.Reset();

        TextureTest::m_texOut.Reset();
        TextureTest::m_descriptorHeapRtv.Reset();
    }

private:
    class TextureTest final : public Test
    {
    public:
#ifdef _GAMING_XBOX_SCARLETT
        static constexpr uint32_t m_standardWidth = 512;
        static constexpr uint32_t m_standardHeight = 512;
#else
        static constexpr uint32_t m_standardWidth = 256;
        static constexpr uint32_t m_standardHeight = 256;
#endif
        static constexpr uint32_t m_threadPerWave = 64; // Change this to per-test if we ever go to wave32 on Scarlett

        struct FormatParams
        {
            const wchar_t*                              m_formatName;
            DXGI_FORMAT                                 m_format;
            uint32_t                                    m_pixelPerClockIdeal;
            uint32_t                                    m_channels;
            bool                                        m_filterable;
        };

        static const std::vector<FormatParams> m_allFormatParams;

        enum TextureMethod : uint32_t
        {
            TEXTURE_METHOD_SAMPLE,
            TEXTURE_METHOD_SAMPLE_D16,
            TEXTURE_METHOD_LOAD,
            TEXTURE_METHOD_LOAD_D16,
            TEXTURE_METHOD_GATHER,

            TEXTURE_METHOD_COUNT
        };

        const wchar_t* GetTextureMethodName() const
        {
            switch (m_filterParams.m_method)
            {
            case TEXTURE_METHOD_SAMPLE:
                return L"Sample";
            case TEXTURE_METHOD_SAMPLE_D16:
                return L"SampleD16";
            case TEXTURE_METHOD_LOAD:
                return L"Load";
            case TEXTURE_METHOD_LOAD_D16:
                return L"LoadD16";
            case TEXTURE_METHOD_GATHER:
                return L"Gather";
            default:
                return L"Invalid";
            }
        };

        struct FilterParams
        {
            const wchar_t*                              m_filterName;
            D3D12_FILTER                                m_filter;
            TextureMethod                               m_method;
            uint32_t                                    m_anisoLevel;
            uint32_t                                    m_internalFetches;
        };

        static const FilterParams m_filterParamsLoad;
        static const FilterParams m_filterParamsLoadD16;
        static const FilterParams m_filterParamsGather;
        static const FilterParams m_filterParamsPoint;
        static const FilterParams m_filterParamsSamplePointD16;
        static const FilterParams m_filterParamsSampleLinearD16;
        static const std::vector<FilterParams> m_allFilterParams;

        TextureTest(const FormatParams& formatParams, const FilterParams& filterParams) :
            m_descriptorSrvGpu{},
            m_descriptorSamplerGpu{},
            m_formatParams(formatParams),
            m_filterParams(filterParams)
        {
            // Don't try to combine Load or Gather with a filter mode
            assert((TEXTURE_METHOD_SAMPLE == m_filterParams.m_method) || (TEXTURE_METHOD_SAMPLE_D16 == m_filterParams.m_method) || (D3D12_FILTER_MIN_MAG_MIP_POINT == m_filterParams.m_filter));

            auto defaultInstances = 4U;
            auto formatScale = m_formatParams.m_pixelPerClockIdeal;
            auto filterScale = m_filterParams.m_internalFetches;

            m_instances = defaultInstances;
            m_instances *= formatScale;
            m_instances /= filterScale;
            m_instances = std::max(1U, m_instances);
        }

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");

            std::wostringstream geometryShaderName;
            if (m_filterParams.m_anisoLevel > 1 && D3D12_FILTER_ANISOTROPIC == m_filterParams.m_filter)
            {
                geometryShaderName << L"TextureAniso" << m_filterParams.m_anisoLevel << L"Gs.cso";

            }
            else
            {
                geometryShaderName << L"TextureTrilinearGs.cso";
            }
            auto geometryShaderBlob = DX::ReadData(geometryShaderName.str().c_str());

            std::wostringstream pixelShaderName;
            pixelShaderName << L"Texture";
            pixelShaderName << m_formatParams.m_channels;
            pixelShaderName << GetTextureMethodName();
            pixelShaderName << L"Ps.cso";
            auto pixelShaderBlob = DX::ReadData(pixelShaderName.str().c_str());

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
                    },                                          // D3D12_SHADER_BYTECODE PS;
                {},                                             // D3D12_SHADER_BYTECODE DS;
                {},                                             // D3D12_SHADER_BYTECODE HS;
                    {
                        geometryShaderBlob.data(),
                        geometryShaderBlob.size(),
                    },                                          // D3D12_SHADER_BYTECODE GS;
                {},                                             // D3D12_STREAM_OUTPUT_DESC StreamOutput;
                CD3DX12_BLEND_DESC(D3D12_DEFAULT),              // D3D12_BLEND_DESC BlendState;
                UINT_MAX,                                       // UINT SampleMask;
                CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),         // D3D12_RASTERIZER_DESC RasterizerState;
                CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),      // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
                {},                                             // D3D12_INPUT_LAYOUT_DESC InputLayout;
                D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,    // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,            // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
                1U,                                             // UINT NumRenderTargets;
                { DXGI_FORMAT_R8G8B8A8_UNORM, },                // DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
                DXGI_FORMAT_UNKNOWN,                            // DXGI_FORMAT DSVFormat;
                { 
                    1U, 
                    DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN, 
                },                                              // DXGI_SAMPLE_DESC SampleDesc;
                0U,                                             // UINT NodeMask;
                {},                                             // D3D12_CACHED_PIPELINE_STATE CachedPSO;
                D3D12_PIPELINE_STATE_FLAG_NONE,                 // D3D12_PIPELINE_STATE_FLAGS Flags;
            };
#ifdef _GAMING_DESKTOP
            //  [ STATE_CREATION WARNING #680: CREATEGRAPHICSPIPELINESTATE_DEPTHSTENCILVIEW_NOT_SET]
            //      D3D12 WARNING: ID3D12Device::CreateGraphicsPipelineState: The depth stencil unit
            //      or pixel shader expects a Depth Stencil View, but the PSO indicates that none will be bound.
            //      This is OK, as reads of an unbound Depth Stencil View are defined to return 0; and writes are
            //      discarded. It is also possible the developer knows the data will not be used anyway.
            //      This is only a problem if the developer actually intended to bind a Depth Stencil View here.
            descPipelineState.DepthStencilState.DepthEnable = FALSE;
#endif
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineState);

            CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

            // We need 2 mip levels for trilinear
            auto descTex = CD3DX12_RESOURCE_DESC::Tex2D(
                m_formatParams.m_format,
                TextureTest::m_standardWidth,  
                TextureTest::m_standardHeight, 
                1, 
                2);
            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
                D3D12_HEAP_FLAG_NONE, 
                &descTex, 
#ifdef _GAMING_XBOX
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
#else // #ifdef _GAMING_DESKTOP
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
#endif
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_tex.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_tex);

            D3D12_DESCRIPTOR_HEAP_DESC descHeapSrv = 
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                1U,                                                 // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,          // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapSrv, IID_GRAPHICS_PPV_ARGS(m_descriptorHeap.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeap);

            // Use extensions in order to set PerfModulation to its maximum value of 7.
            // The default value is PerfModulation = 4, which reduces the effect of sampler tweaks to 4/7.
            auto descriptorSrvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeapSrv.Type));
            m_descriptorSrvGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeapSrv.Type));

            device->CreateShaderResourceView(m_tex.Get(), nullptr, descriptorSrvCpu);

            D3D12_DESCRIPTOR_HEAP_DESC descHeapSampler = 
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                1U,                                             // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,      // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapSampler, IID_GRAPHICS_PPV_ARGS(m_descriptorHeapSampler.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeapSampler);

            auto descriptorSamplerCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapSampler->GetCPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeapSampler.Type));
            m_descriptorSamplerGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeapSampler->GetGPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeapSampler.Type));

            // The effect of this is to round up to the next power of 2. 
            // The hardware only supports power of 2 max aniso, so the driver rounds down to a power of 2.
            auto maxAnisotropy = std::min(16U, 2 * m_filterParams.m_anisoLevel - 1); 
            D3D12_SAMPLER_DESC descSampler = 
            {
                m_filterParams.m_filter,                        // D3D12_FILTER Filter;
                D3D12_TEXTURE_ADDRESS_MODE_MIRROR,              // D3D12_TEXTURE_ADDRESS_MODE AddressU;
                D3D12_TEXTURE_ADDRESS_MODE_MIRROR,              // D3D12_TEXTURE_ADDRESS_MODE AddressV;
                D3D12_TEXTURE_ADDRESS_MODE_MIRROR,              // D3D12_TEXTURE_ADDRESS_MODE AddressW;
                0.0f,                                           // FLOAT MipLODBias;
                maxAnisotropy,                                  // UINT MaxAnisotropy;
#ifdef _GAMING_XBOX
                D3D12_COMPARISON_FUNC_ALWAYS,                   // D3D12_COMPARISON_FUNC ComparisonFunc;
#else // #ifdef _GAMING_DESKTOP
                D3D12_COMPARISON_FUNC_NONE,                     // D3D12_COMPARISON_FUNC ComparisonFunc;
#endif
                {},                                             // FLOAT BorderColor[4];
                0.0f,                                           // FLOAT MinLOD;
                D3D12_FLOAT32_MAX,                              // FLOAT MaxLOD;
            };
            device->CreateSampler(&descSampler, descriptorSamplerCpu);
        }

        void Uninitialize() override
        {
            m_tex.Reset();
            m_descriptorHeap.Reset();

            m_descriptorHeapSampler.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Format: ";
            name << m_formatParams.m_formatName << L", ";
            name << L"Filter: ";
            name << m_filterParams.m_filterName << L" ";

            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Format", L"", 20);
            report->AddColumn(L"Filter", L"", 10);
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Fetch(API)", L"", 12, 0);
            report->AddColumn(L"Fetch(GPU)", L"", 12, 0);
#ifdef _GAMING_XBOX
            report->AddColumn(L"StDev", L"%", 7, 2);
            report->AddColumn(L"Avg mip", L"", 4, 2);
            report->AddColumn(L"Avg aniso", L"", 4, 2);
            report->AddColumn(L"Op/Fetch", L"", 4, 2);
#endif
            report->AddColumn(L"VMEM/clock", L"", 5, 2);
            report->AddColumn(L"% of max", L"%", 6, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
#ifdef _GAMING_XBOX
            const auto& gpuProperties = GpuProperties::Get();
#endif

            auto timeMs = GetElapsedTimeMs();
#if defined(_GAMING_XBOX_SCARLETT)
            auto vmemGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_INSTS_TEX) * m_threadPerWave;
#elif defined(_GAMING_XBOX_XBOXONE)
            auto vmemGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_INSTS_VMEM) * m_threadPerWave;
#else // defined(_GAMING_DESKTOP)
            auto vmemGPU = 0ULL;
            auto vmemPerClock = 0.0;
            auto vmemThroughput = 0.0;
            GetVMemInstructions(GetPsWaveSize(), &vmemGPU, &vmemPerClock, &vmemThroughput);
#endif
            auto vmemInShader = 64U;
            auto vmemAPI = vmemInShader * m_instances * m_standardWidth * m_standardHeight;

#ifdef _GAMING_XBOX
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;
            auto vmemPerClock = (1000.0f * vmemGPU) / (timeMs * clockSpeed);

            auto pixelPerClockIdeal = m_formatParams.m_pixelPerClockIdeal;
            // All textures can be point sampled at full rate (or something like full rate?)
            if (D3D12_FILTER_MIN_MAG_MIP_POINT == m_filterParams.m_filter)
            {
                pixelPerClockIdeal = 4;
            }
            // For non-filterable formats, we do a Load, which is faster than point-sampling on Scarlett
            if (TEXTURE_METHOD_LOAD == m_filterParams.m_method || TEXTURE_METHOD_LOAD_D16 == m_filterParams.m_method)   
            {
#if defined(_GAMING_XBOX_SCARLETT)
                bool halfRate =
                    (m_formatParams.m_pixelPerClockIdeal < 4U)
                    || (m_formatParams.m_channels > 2U && (TEXTURE_METHOD_LOAD_D16 != m_filterParams.m_method));
                pixelPerClockIdeal = halfRate ? 8U : 16U;
#elif defined(_GAMING_XBOX_XBOXONE)
                pixelPerClockIdeal = 4;
#endif
            }
            auto numSe = gpuProperties.m_numSe;
            auto numCuPerSe = gpuProperties.m_numCuPerSe;
            auto vmemPerClockIdeal = float(numCuPerSe * numSe * pixelPerClockIdeal) / float(m_filterParams.m_internalFetches);
            auto vmemThroughput = 100.0f * vmemPerClock / vmemPerClockIdeal;

#if defined(_GAMING_XBOX_SCARLETT)
            auto samplerCycles = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_image_sampler_total_cycles);
            auto mip0 = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_mipmap_lod_0_samples);
            auto mip1 = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_mipmap_lod_1_samples);
#elif defined(_GAMING_XBOX_XBOXONE)
            auto mip0 = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_MIPMAP_LOD_0_SAMPLES);
            auto mip1 = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_MIPMAP_LOD_1_SAMPLES);
#endif
            // This is not the "average mip" in the sense of a fractional lod between 0 and 1.
            // Instead, it represents the average over all fetches, with each fetch sampling from mip0, mip1, or both. 
            // For trilinear/aniso, we expect it to be exactly 0.5, because every fetch should sample both mips.
            auto avgMip = (0.0f * mip0 + 1.0f * mip1) / (mip0 + mip1);
#ifdef _GAMING_XBOX_SCARLETT
            if (0 == samplerCycles)
            {
                avgMip = 0; // These are image_loads
            }
#endif

#if defined(_GAMING_XBOX_SCARLETT)
            auto bilinPoint = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_bilin_point_1_cycle_pixels);
            auto trilinear = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_mip_2_cycle_pixels);
            auto aniso = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_gt1_cycle_quads);
#elif defined(_GAMING_XBOX_XBOXONE)
            auto bilinPoint = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_BILIN_POINT_1_CYCLE_PIXELS);
            auto trilinear = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_MIP_2_CYCLE_PIXELS);
            auto aniso = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_GT1_CYCLE_QUADS);
#endif
            // There's no way to accurately count "aniso and not trilinear", so there's some cases we don't handle properly.
            bilinPoint -= std::min(bilinPoint, std::max(trilinear, aniso));
            trilinear -= std::min(trilinear, aniso);

#if defined(_GAMING_XBOX_SCARLETT)
            auto aniso2 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_2_cycle_quads);
            auto aniso4 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_4_cycle_quads);
            auto aniso6 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_6_cycle_quads);
            auto aniso8 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_8_cycle_quads);
            auto aniso10 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_10_cycle_quads);
            auto aniso12 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_12_cycle_quads);
            auto aniso14 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_14_cycle_quads);
            auto aniso16 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_16_cycle_quads);
#elif defined(_GAMING_XBOX_XBOXONE)
            auto aniso2 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_2_CYCLE_QUADS);
            auto aniso4 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_4_CYCLE_QUADS);
            auto aniso6 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_6_CYCLE_QUADS);
            auto aniso8 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_8_CYCLE_QUADS);
            auto aniso10 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_10_CYCLE_QUADS);
            auto aniso12 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_12_CYCLE_QUADS);
            auto aniso14 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_14_CYCLE_QUADS);
            auto aniso16 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_16_CYCLE_QUADS);
#endif

            // We don't get exact aniso levels for higher values. That may be due to edge pixels.
            auto avgAniso = 1.0f * bilinPoint
                + 1.0f * trilinear
                + 2.0f * aniso2
                + 4.0f * aniso4
                + 6.0f * aniso6
                + 8.0f * aniso8
                + 10.0f * aniso10
                + 12.0f * aniso12
                + 14.0f * aniso14
                + 16.0f * aniso16;
            avgAniso /= vmemGPU;
            auto opPerVmem = 1.0f * bilinPoint
                + 2.0f * trilinear
                + 2.0f * (
                    + 2.0f * aniso2
                    + 4.0f * aniso4
                    + 6.0f * aniso6
                    + 8.0f * aniso8
                    + 10.0f * aniso10
                    + 12.0f * aniso12
                    + 14.0f * aniso14
                    + 16.0f * aniso16
                    );
            opPerVmem /= vmemGPU;

#if defined(_GAMING_XBOX_SCARLETT)
            auto avg = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_TA_REQ_READ,
                GpuCounter::SHADER_MASK_ALL,
                CounterValueArrayAvg);
            auto stdev = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_TA_REQ_READ,
                GpuCounter::SHADER_MASK_ALL,
                CounterValueArrayStd);
#elif defined(_GAMING_XBOX_XBOXONE)
            auto avg = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_IMAGE_READ_WAVEFRONTS,
                GpuCounter::SHADER_MASK_ALL,
                CounterValueArrayAvg);
            auto stdev = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_IMAGE_READ_WAVEFRONTS,
                GpuCounter::SHADER_MASK_ALL,
                CounterValueArrayStd);
#endif

#endif // #ifdef _GAMING_XBOX

            report->AddRowData(m_formatParams.m_formatName);
            report->AddRowData(m_filterParams.m_filterName);
            report->AddRowData(timeMs);
            report->AddRowData(vmemAPI);
            report->AddRowData(vmemGPU);
#ifdef _GAMING_XBOX
            report->AddRowData(100.0f * stdev / avg);
            report->AddRowData(avgMip);
            report->AddRowData(avgAniso);
            report->AddRowData(opPerVmem);
#endif
            report->AddRowData(vmemPerClock);
            report->AddRowData(vmemThroughput);

            report->EndRow();
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            ID3D12DescriptorHeap* descriptorHeaps[] =
            {
                m_descriptorHeapSampler.Get(), 
                m_descriptorHeap.Get(), 
            };
            commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

            commandList->OMSetRenderTargets(1, &m_descriptorRtvCpu, FALSE, nullptr);

            commandList->RSSetViewports(1, &m_viewport);
            commandList->RSSetScissorRects(1, &m_scissorRect);

            commandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_INDEX_SRV, m_descriptorSrvGpu);

            commandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_INDEX_SAMPLER, m_descriptorSamplerGpu);

            commandList->DrawInstanced(1, m_instances, 0, 0);
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>      m_rootSignature;

        static ComPtr<ID3D12Resource>           m_texOut;
        static ComPtr<ID3D12DescriptorHeap>     m_descriptorHeapRtv;
        static D3D12_CPU_DESCRIPTOR_HANDLE      m_descriptorRtvCpu;

        static D3D12_VIEWPORT                   m_viewport;
        static D3D12_RECT                       m_scissorRect;

    private:
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>             m_pipelineState;

        ComPtr<ID3D12Resource>                  m_tex;
        ComPtr<ID3D12DescriptorHeap>            m_descriptorHeap;
        D3D12_GPU_DESCRIPTOR_HANDLE             m_descriptorSrvGpu;

        ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapSampler;
        D3D12_GPU_DESCRIPTOR_HANDLE             m_descriptorSamplerGpu;

        FormatParams                            m_formatParams;
        FilterParams                            m_filterParams;

        uint32_t                                m_instances;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature>     TextureBenchmark::TextureTest::m_rootSignature;

ComPtr<ID3D12Resource>          TextureBenchmark::TextureTest::m_texOut;
ComPtr<ID3D12DescriptorHeap>    TextureBenchmark::TextureTest::m_descriptorHeapRtv;
D3D12_CPU_DESCRIPTOR_HANDLE     TextureBenchmark::TextureTest::m_descriptorRtvCpu;

D3D12_VIEWPORT                  TextureBenchmark::TextureTest::m_viewport;
D3D12_RECT                      TextureBenchmark::TextureTest::m_scissorRect;

const std::vector<TextureBenchmark::TextureTest::FormatParams> TextureBenchmark::TextureTest::m_allFormatParams = 
{
#ifdef _GAMING_XBOX_SCARLETT
    { L"BC1_UNORM",           DXGI_FORMAT_BC1_UNORM,            4,   4,  true,   }, 
    { L"BC6H_UNORM",          DXGI_FORMAT_BC6H_UF16,            4,   4,  true,   }, 
    { L"R9G9B9E5_SHAREDEXP",  DXGI_FORMAT_R9G9B9E5_SHAREDEXP,   4,   4,  true,   }, 
    { L"R8_UNORM",            DXGI_FORMAT_R8_UNORM,             4,   1,  true,   }, 
    { L"R8G8_UNORM",          DXGI_FORMAT_R8G8_UNORM,           4,   2,  true,   }, 
    { L"R16_UNORM",           DXGI_FORMAT_R16_UNORM,            4,   1,  true,   }, 
    { L"R8G8B8A8_UNORM",      DXGI_FORMAT_R8G8B8A8_UNORM,       4,   4,  true,   }, 
    { L"R8G8B8A8_UNORM_SRGB", DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  4,   4,  true,   }, 
    { L"R10G10B10A2_UNORM",   DXGI_FORMAT_R10G10B10A2_UNORM,    4,   4,  true,   }, 
    { L"R11G11B10_FLOAT",     DXGI_FORMAT_R11G11B10_FLOAT,      4,   4,  true,   }, 
    { L"R16G16_UNORM",        DXGI_FORMAT_R16G16_UNORM,         4,   2,  true,   }, 
    { L"R16G16_FLOAT",        DXGI_FORMAT_R16G16_FLOAT,         4,   2,  true,   }, 
    { L"R32_UINT",            DXGI_FORMAT_R32_UINT,             4,   1,  false   },
    { L"R32_FLOAT",           DXGI_FORMAT_R32_FLOAT,            4,   1,  true,   }, 
    { L"R16G16B16A16_UNORM",  DXGI_FORMAT_R16G16B16A16_UNORM,   4,   4,  true,   }, 
    { L"R16G16B16A16_FLOAT",  DXGI_FORMAT_R16G16B16A16_FLOAT,   4,   4,  true,   }, 
    { L"R32G32_UINT",         DXGI_FORMAT_R32G32_UINT,          4,   2,  false   }, 
    { L"R32G32_FLOAT",        DXGI_FORMAT_R32G32_FLOAT,         4,   2,  true,   }, 
    { L"R32G32B32A32_UINT",   DXGI_FORMAT_R32G32B32A32_UINT,    2,   4,  false   }, 
    { L"R32G32B32A32_FLOAT",  DXGI_FORMAT_R32G32B32A32_FLOAT,   2,   4,  true,   }, 
#else
    { L"BC1_UNORM",           DXGI_FORMAT_BC1_UNORM,            4,   4,  true,   }, 
    { L"BC6H_UNORM",          DXGI_FORMAT_BC6H_UF16,            2,   4,  true,   }, 
    { L"R9G9B9E5_SHAREDEXP",  DXGI_FORMAT_R9G9B9E5_SHAREDEXP,   4,   4,  true,   }, 
    { L"R8_UNORM",            DXGI_FORMAT_R8_UNORM,             4,   1,  true,   }, 
    { L"R8G8_UNORM",          DXGI_FORMAT_R8G8_UNORM,           4,   2,  true,   }, 
    { L"R16_UNORM",           DXGI_FORMAT_R16_UNORM,            4,   1,  true,   }, 
    { L"R8G8B8A8_UNORM",      DXGI_FORMAT_R8G8B8A8_UNORM,       4,   4,  true,   }, 
    { L"R8G8B8A8_UNORM_SRGB", DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  4,   4,  true,   }, 
    { L"R10G10B10A2_UNORM",   DXGI_FORMAT_R10G10B10A2_UNORM,    4,   4,  true,   }, 
    { L"R11G11B10_FLOAT",     DXGI_FORMAT_R11G11B10_FLOAT,      4,   4,  true,   }, 
    { L"R16G16_UNORM",        DXGI_FORMAT_R16G16_UNORM,         4,   2,  true,   }, 
    { L"R16G16_FLOAT",        DXGI_FORMAT_R16G16_FLOAT,         4,   2,  true,   }, 
    { L"R32_UINT",            DXGI_FORMAT_R32_UINT,             4,   1,  false   }, 
    { L"R32_FLOAT",           DXGI_FORMAT_R32_FLOAT,            4,   1,  true,   }, 
    { L"R16G16B16A16_UNORM",  DXGI_FORMAT_R16G16B16A16_UNORM,   2,   4,  true,   }, 
    { L"R16G16B16A16_FLOAT",  DXGI_FORMAT_R16G16B16A16_FLOAT,   2,   4,  true,   }, 
    { L"R32G32_UINT",         DXGI_FORMAT_R32G32_UINT,          4,   2,  false   }, 
    { L"R32G32_FLOAT",        DXGI_FORMAT_R32G32_FLOAT,         2,   2,  true,   }, 
    { L"R32G32B32A32_UINT",   DXGI_FORMAT_R32G32B32A32_UINT,    4,   4,  false   }, 
    { L"R32G32B32A32_FLOAT",  DXGI_FORMAT_R32G32B32A32_FLOAT,   1,   4,  true,   }, 
#endif
};

const TextureBenchmark::TextureTest::FilterParams TextureBenchmark::TextureTest::m_filterParamsLoad =
{ L"Load",      D3D12_FILTER_MIN_MAG_MIP_POINT,         TEXTURE_METHOD_LOAD,        1,  1,  };
const TextureBenchmark::TextureTest::FilterParams TextureBenchmark::TextureTest::m_filterParamsLoadD16 =
{ L"LoadD16",   D3D12_FILTER_MIN_MAG_MIP_POINT,         TEXTURE_METHOD_LOAD_D16,    1,  1,  };
const TextureBenchmark::TextureTest::FilterParams TextureBenchmark::TextureTest::m_filterParamsGather =
{ L"Gather",    D3D12_FILTER_MIN_MAG_MIP_POINT,         TEXTURE_METHOD_GATHER,      1,  1,  };
const TextureBenchmark::TextureTest::FilterParams TextureBenchmark::TextureTest::m_filterParamsPoint =
{ L"Point",     D3D12_FILTER_MIN_MAG_MIP_POINT,         TEXTURE_METHOD_SAMPLE,      1,  1, };
const TextureBenchmark::TextureTest::FilterParams TextureBenchmark::TextureTest::m_filterParamsSamplePointD16 =
{ L"PointD16",  D3D12_FILTER_MIN_MAG_MIP_POINT,         TEXTURE_METHOD_SAMPLE_D16,  1,  1, };
const TextureBenchmark::TextureTest::FilterParams TextureBenchmark::TextureTest::m_filterParamsSampleLinearD16 =
{ L"LinearD16", D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,  TEXTURE_METHOD_SAMPLE_D16,  1,  1, };
const std::vector<TextureBenchmark::TextureTest::FilterParams> TextureBenchmark::TextureTest::m_allFilterParams =
{
    { L"Bilinear",  D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,  TEXTURE_METHOD_SAMPLE,  1,  1,  },
    { L"Trilinear", D3D12_FILTER_MIN_MAG_MIP_LINEAR,        TEXTURE_METHOD_SAMPLE,  1,  2,  },
    { L"2x Aniso",  D3D12_FILTER_ANISOTROPIC,               TEXTURE_METHOD_SAMPLE,  2,  4,  },
    { L"4x Aniso",  D3D12_FILTER_ANISOTROPIC,               TEXTURE_METHOD_SAMPLE,  4,  8,  },
    { L"6x Aniso",  D3D12_FILTER_ANISOTROPIC,               TEXTURE_METHOD_SAMPLE,  6,  12, },
    { L"8x Aniso",  D3D12_FILTER_ANISOTROPIC,               TEXTURE_METHOD_SAMPLE,  8,  16, },
    { L"10x Aniso", D3D12_FILTER_ANISOTROPIC,               TEXTURE_METHOD_SAMPLE,  10, 20, },
    { L"12x Aniso", D3D12_FILTER_ANISOTROPIC,               TEXTURE_METHOD_SAMPLE,  12, 24, },
    { L"14x Aniso", D3D12_FILTER_ANISOTROPIC,               TEXTURE_METHOD_SAMPLE,  14, 28, },
    { L"16x Aniso", D3D12_FILTER_ANISOTROPIC,               TEXTURE_METHOD_SAMPLE,  16, 32, },
};

TextureBenchmark benchmark;

