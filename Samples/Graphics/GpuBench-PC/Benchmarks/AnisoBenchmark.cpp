//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

#ifdef _GAMING_XBOX
static constexpr D3D12_RESOURCE_STATES kDefaultTexState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
#else // #ifdef _GAMING_DESKTOP
static constexpr D3D12_RESOURCE_STATES kDefaultTexState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
#endif

class AnisoBenchmark final : public Benchmark
{
public:
    AnisoBenchmark() = default;

    ~AnisoBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Aniso";
    }

    void Initialize(ID3D12Device* device) override
    {
#ifdef _GAMING_XBOX
        // Aniso bias tests
        for (auto anisoLevel : { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, })
        {
            for (auto anisoBias : { 0.0f, 0.5f, 1.0f, 1.5f, 1.0f + 31.0f/32.0f, })
            {
                AddTest(new AnisoTest(anisoLevel, anisoBias, 0));
            }
        }

        // Aniso threshold tests
        for (auto anisoLevel : { 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f, })
        {
            for (auto anisoThreshold : { 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U })
            {
                AddTest(new AnisoTest(anisoLevel, 0, anisoThreshold));
            }
        }
#else
        for (auto anisoLevel : { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f, })
        {
            AddTest(new AnisoTest(anisoLevel));
        }
#endif

#if defined(_GAMING_XBOX_SCARLETT)
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_TEX);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_gt1_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_2_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_4_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_6_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_8_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_10_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_12_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_14_cycle_quads);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_aniso_16_cycle_quads);
#elif defined(_GAMING_XBOX_XBOXONE)
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_VMEM);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_GT1_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_2_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_4_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_6_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_8_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_10_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_12_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_14_CYCLE_QUADS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_ANISO_16_CYCLE_QUADS);
#else // defined(_GAMING_DESKTOP)
        AddCounter_ElapsedTime();
        AddCounter_VMemInstructions();
        AddCounter_PsWaveSize();

        const auto & gpuProps = GpuProperties::Get();
        if (gpuProps.IsSupportedAmdGpu())
        {
            // This counter doesn't report correctly for RX7000 series in GPA 4.0. The issue has been reported
            AddCounter("TexAveAnisotropy");
        }
#endif

        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(AnisoTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(AnisoTest::m_rootSignature);

        CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        // Use 1 mip to eliminate trilinear cost
        auto descTex = CD3DX12_RESOURCE_DESC::Tex2D(
            AnisoTest::m_format,
            AnisoTest::m_standardWidth,  
            AnisoTest::m_standardHeight, 
            1, 
            1);
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
            D3D12_HEAP_FLAG_NONE, 
            &descTex, 
            kDefaultTexState,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(AnisoTest::m_tex.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(AnisoTest::m_tex);

        auto descTexUpload = CD3DX12_RESOURCE_DESC::Buffer(GetRequiredIntermediateSize(AnisoTest::m_tex.Get(), 0, 1));
        DX::ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, 
            D3D12_HEAP_FLAG_NONE, 
            &descTexUpload, 
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(AnisoTest::m_texUpload.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(AnisoTest::m_texUpload);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapSrv = 
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,          // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapSrv, IID_GRAPHICS_PPV_ARGS(AnisoTest::m_descriptorHeap.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(AnisoTest::m_descriptorHeap);

        // Use extensions in order to set PerfModulation to its maximum value of 7.
        // The default value is PerfModulation = 4, which reduces the effect of sampler tweaks to 4/7.
        auto descriptorSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(AnisoTest::m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
            0U, 
            device->GetDescriptorHandleIncrementSize(descHeapSrv.Type));
        AnisoTest::m_descriptorSrvGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(AnisoTest::m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
            0U, 
            device->GetDescriptorHandleIncrementSize(descHeapSrv.Type));

        D3D12_TEX2D_SRV srvTexture2D =
        {
            0,                                                  // UINT MostDetailedMip;
            UINT(-1),                                           // UINT MipLevels;
                                                                // UINT PlaneSlice;
                                                                // FLOAT ResourceMinLODClamp;
        };
        // The items which are left unspecified should be overridden by the resource values

#ifdef _GAMING_XBOX
        D3D12XBOX_SHADER_RESOURCE_VIEW_DESC descSrv = 
        {
            DXGI_FORMAT_UNKNOWN,                                // DXGI_FORMAT Format;
            D3D12_SRV_DIMENSION_TEXTURE2D,                      // D3D12_SRV_DIMENSION ViewDimension;
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,           // UINT Shader4ComponentMapping;
            {},                                                 // D3D12_TEX2D_SRV Texture2D;
            D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN,                  // D3D12_GPU_VIRTUAL_ADDRESS ResourceLocation;
#ifdef _GAMING_XBOX_SCARLETT
            D3D12XBOX_IMAGE_FORMAT_INVALID,                     // D3D12XBOX_IMAGE_FORMAT ImageFormat;
#else
            D3D12XBOX_DATA_FORMAT_INVALID,                      // D3D12XBOX_DATA_FORMAT DataFormat;
            D3D12XBOX_NUMBER_FORMAT_INVALID,                    // D3D12XBOX_NUMBER_FORMAT NumberFormat;
#endif
            0,                                                  // UINT MemoryType;
            0.0f,                                               // FLOAT TextureWarnLevelOfDetail;
            3 /* driver will add 4 to this */,                  // INT TexturePerfModulation;
        };  
        descSrv.Texture2D = srvTexture2D;

        device->CreatePlacedShaderResourceViewX(AnisoTest::m_tex.Get(), &descSrv, descriptorSrv);
#else
        D3D12_SHADER_RESOURCE_VIEW_DESC descSrv =
        {
            DXGI_FORMAT_UNKNOWN,                                // DXGI_FORMAT Format;
            D3D12_SRV_DIMENSION_TEXTURE2D,                      // D3D12_SRV_DIMENSION ViewDimension;
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,           // UINT Shader4ComponentMapping;
            {}                                                  // D3D12_TEX2D_SRV Texture2D;
        };
        descSrv.Texture2D = srvTexture2D;

        device->CreateShaderResourceView(AnisoTest::m_tex.Get(), &descSrv, descriptorSrv);
#endif

        // We require a render target in order to set viewport dimensions.
        auto descTexOut = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM,
            AnisoTest::m_standardWidth,  
            AnisoTest::m_standardHeight, 
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
            IID_GRAPHICS_PPV_ARGS(AnisoTest::m_texOut.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(AnisoTest::m_texOut);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapRtv = 
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapRtv, IID_GRAPHICS_PPV_ARGS(AnisoTest::m_descriptorHeapRtv.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(AnisoTest::m_descriptorHeapRtv);

        AnisoTest::m_descriptorRtvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(AnisoTest::m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(), 
            0U, 
            device->GetDescriptorHandleIncrementSize(descHeapRtv.Type));

        device->CreateRenderTargetView(AnisoTest::m_texOut.Get(), nullptr, AnisoTest::m_descriptorRtvCpu);

        D3D12_VIEWPORT viewport = 
        {
            0,                                                          // FLOAT TopLeftX;
            0,                                                          // FLOAT TopLeftY;
            FLOAT(AnisoTest::m_standardWidth),                          // FLOAT Width;
            FLOAT(AnisoTest::m_standardHeight),                         // FLOAT Height;
            0.0f,                                                       // FLOAT MinDepth;
            1.0f,                                                       // FLOAT MaxDepth;
        };
        AnisoTest::m_viewport = viewport;

        D3D12_RECT scissorRect =
        {
            0,                                                          // LONG    left;
            0,                                                          // LONG    top;
            LONG(AnisoTest::m_standardWidth),                           // LONG    right;
            LONG(AnisoTest::m_standardHeight),                          // LONG    bottom;
        };
        AnisoTest::m_scissorRect = scissorRect;

        AnisoTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        AnisoTest::m_rootSignature.Reset();

        AnisoTest::m_texUpload.Reset();
        AnisoTest::m_tex.Reset();
        AnisoTest::m_descriptorHeap.Reset();

        AnisoTest::m_texOut.Reset();
        AnisoTest::m_descriptorHeapRtv.Reset();
    }

private:
    class AnisoTest final : public Test
    {
    public:
        static constexpr uint32_t m_standardWidth = 256;
        static constexpr uint32_t m_standardHeight = 256;
        static constexpr auto m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
        static constexpr uint32_t m_instances = 4;
        static constexpr uint32_t m_threadPerWave = 64; // Change this to per-test if we ever go to wave32 on Scarlett

#ifdef _GAMING_XBOX
        AnisoTest(float anisoLevel, float anisoBias, uint32_t anisoThreshold) :
            m_anisoLevel(anisoLevel),
            m_anisoBias(anisoBias), 
            m_anisoThreshold(anisoThreshold),
            m_descriptorSamplerGpu{}
        { }
#else
        AnisoTest(float anisoLevel) :
            m_anisoLevel(anisoLevel),
            m_descriptorSamplerGpu{}
        {}
#endif

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto pixelShaderBlob = DX::ReadData(L"AnisoPs.cso");

            std::wostringstream geometryShaderName;
            geometryShaderName << L"Aniso" << std::setfill(L'0') << std::setw(2) << (10.0f * m_anisoLevel) << L"Gs.cso";
            auto geometryShaderBlob = DX::ReadData(geometryShaderName.str().c_str());

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

#ifdef _GAMING_XBOX
            D3D12XBOX_SAMPLER_DESC descSampler =
            {
                D3D12XBOX_SAMPLER_FLAG_NONE,                    // D3D12XBOX_SAMPLER_FLAGS Flags;
                D3D12XBOX_TEXTURE_XY_FILTER_ANISO_BILINEAR,     // D3D12XBOX_TEXTURE_XY_FILTER FilterMag;
                D3D12XBOX_TEXTURE_XY_FILTER_ANISO_BILINEAR,     // D3D12XBOX_TEXTURE_XY_FILTER FilterMin;
                D3D12XBOX_TEXTURE_MIP_FILTER_LINEAR,            // D3D12XBOX_TEXTURE_MIP_FILTER FilterMip;
                D3D12XBOX_TEXTURE_Z_FILTER_POINT,               // D3D12XBOX_TEXTURE_Z_FILTER FilterZ;
                D3D12XBOX_TEXTURE_FILTER_MODE_LERP,             // D3D12XBOX_TEXTURE_FILTER_MODE FilterMode;
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,                // D3D12_TEXTURE_ADDRESS_MODE AddressU;
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,                // D3D12_TEXTURE_ADDRESS_MODE AddressV;
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,                // D3D12_TEXTURE_ADDRESS_MODE AddressW;
                0.0f,                                           // FLOAT MinLOD;
                1.0f,                                           // FLOAT MaxLOD;
                0.0f,                                           // FLOAT MipLODBias;
                0.0f,                                           // FLOAT MipLODBiasSecondary;
                16U,                                            // UINT MaxAnisotropy;
                m_anisoBias,                                    // FLOAT AnisotropyBias;
                m_anisoThreshold,                               // UINT AnisotropyThreshold;
                D3D12_COMPARISON_FUNC_ALWAYS,                   // D3D12_COMPARISON_FUNC ComparisonFunc;
                {},                                             // FLOAT BorderColor[4];
                0,                                              // UINT PerformanceMip;
                0,                                              // UINT PerformanceZ;
            };
            device->CreateSamplerX(&descSampler, descriptorSamplerCpu);
#else
            D3D12_SAMPLER_DESC descSampler =
            {
                D3D12_FILTER_ANISOTROPIC,                       // D3D12_FILTER Filter;
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,                // D3D12_TEXTURE_ADDRESS_MODE AddressU;
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,                // D3D12_TEXTURE_ADDRESS_MODE AddressV;
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,                // D3D12_TEXTURE_ADDRESS_MODE AddressW;
                0.0f,                                           // FLOAT MipLODBias;
                16U,                                            // UINT MaxAnisotropy;
                D3D12_COMPARISON_FUNC_NONE,                     // D3D12_COMPARISON_FUNC ComparisonFunc;
                {},                                             // FLOAT BorderColor[ 4 ];
                0.0f,                                           // FLOAT MinLOD;
                1.0f                                            // FLOAT MaxLOD;
            };
            device->CreateSampler(&descSampler, descriptorSamplerCpu);
#endif
        }

        void Uninitialize() override
        {
            m_pipelineState.Reset();

            m_descriptorHeapSampler.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Aniso: ";
            name << m_anisoLevel;
#ifdef _GAMING_XBOX
            name << L", ";
            name << L"Bias: ";
            name << m_anisoBias;
            name << L", ";
            name << L"Thresh: ";
            name << m_anisoThreshold;
#endif

            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Aniso", L"", 6, 1);
#ifdef _GAMING_XBOX
            report->AddColumn(L"Bias", L"", 6, 2);
            report->AddColumn(L"Threshold", L"", 6);
#endif
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Avg aniso", L"", 4, 2);
            report->AddColumn(L"VMEM/clock", L"", 5, 2);
            report->AddColumn(L"% of max", L"%", 6, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            const auto & gpuProperties = GpuProperties::Get();

            auto timeMs = GetElapsedTimeMs();
#if defined(_GAMING_XBOX)

#ifdef _GAMING_XBOX_SCARLETT
            auto vmemGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_INSTS_TEX) * m_threadPerWave;
#else
            auto vmemGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_INSTS_VMEM) * m_threadPerWave;
#endif
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;
            auto vmemPerClock = (1000.0f * vmemGPU) / (timeMs * clockSpeed);

            auto clockPerVmemBase = 1;
            auto clockPerVmemIdeal = clockPerVmemBase;

#ifdef _GAMING_XBOX_SCARLETT
            auto aniso = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_gt1_cycle_quads);
            auto aniso2 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_2_cycle_quads);
            auto aniso4 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_4_cycle_quads);
            auto aniso6 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_6_cycle_quads);
            auto aniso8 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_8_cycle_quads);
            auto aniso10 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_10_cycle_quads);
            auto aniso12 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_12_cycle_quads);
            auto aniso14 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_14_cycle_quads);
            auto aniso16 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_aniso_16_cycle_quads);
#else
            auto aniso = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_GT1_CYCLE_QUADS);
            auto aniso2 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_2_CYCLE_QUADS);
            auto aniso4 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_4_CYCLE_QUADS);
            auto aniso6 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_6_CYCLE_QUADS);
            auto aniso8 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_8_CYCLE_QUADS);
            auto aniso10 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_10_CYCLE_QUADS);
            auto aniso12 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_12_CYCLE_QUADS);
            auto aniso14 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_14_CYCLE_QUADS);
            auto aniso16 = 4 * GetCounterValue(GPUPerfCounters::TA_PERF_SEL_ANISO_16_CYCLE_QUADS);
#endif
            auto nonAniso = (vmemGPU >= aniso) ? (vmemGPU - aniso) : 0UL;

            // We don't get exact aniso levels for higher values. That may be due to edge pixels.
            auto avgAniso = 1.0f * nonAniso
                + 2.0f * aniso2
                + 4.0f * aniso4
                + 6.0f * aniso6
                + 8.0f * aniso8
                + 10.0f * aniso10
                + 12.0f * aniso12
                + 14.0f * aniso14
                + 16.0f * aniso16;
            avgAniso /= vmemGPU;
            auto numSe = gpuProperties.m_numSe;
            auto numCuPerSe = gpuProperties.m_numCuPerSe;
            auto numSimdPerCu = gpuProperties.m_numSimdPerCu;
            auto vmemPerClockIdeal = ((numSimdPerCu * numCuPerSe * numSe) / float(clockPerVmemIdeal));
            auto vmemThroughput = 100.0f * vmemPerClock / vmemPerClockIdeal;
#else // #if defined(_GAMING_DESKTOP)
            auto avgAniso = 0.0;
            auto vmemPerClock = 0.0;
            auto vmemThroughput = 0.0;
            GetVMemInstructions(GetPsWaveSize(), nullptr, &vmemPerClock, &vmemThroughput);
            if (gpuProperties.IsSupportedAmdGpu())
            {
                avgAniso = GetCounterValue("TexAveAnisotropy");
            }
#endif // #if defined(_GAMING_XBOX)

            report->AddRowData(m_anisoLevel);
#ifdef _GAMING_XBOX
            report->AddRowData(m_anisoBias);
            report->AddRowData(m_anisoThreshold);
#endif
            report->AddRowData(timeMs);
            report->AddRowData(avgAniso);
            report->AddRowData(vmemPerClock);
            report->AddRowData(vmemThroughput);

            DirectX::XMVECTOR colors[] =
            {
                DirectX::Colors::White,
                DirectX::Colors::Tan,
                DirectX::Colors::Silver,
                DirectX::Colors::Wheat,
            };
            DirectX::XMVECTOR color = colors[uint32_t(m_anisoLevel * 2.0f) % 4];
            report->EndRow(color);
        }

        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            // Sample a checkerboard pattern, where the squares are wider than the anisotropy
            auto initialBytes = new uint8_t[AnisoTest::m_standardHeight][AnisoTest::m_standardWidth][4];
            for (auto y = 0U; y < AnisoTest::m_standardHeight; ++y)
            {
                for (auto x = 0U; x < AnisoTest::m_standardWidth; ++x)
                {
                    uint8_t parity = (x / 16 + y / 16) % 2;
                    initialBytes[y][x][0] = 
                        initialBytes[y][x][1] = 
                        initialBytes[y][x][2] = 
                        initialBytes[y][x][3] = (parity ? uint8_t(0xff) : uint8_t(0x00));
                }
            }

            D3D12_SUBRESOURCE_DATA initialData =
            {
                initialBytes,                                                   // const void* pSysMem;
                AnisoTest::m_standardWidth * 4U,                                // UINT SysMemPitch;
                AnisoTest::m_standardHeight * AnisoTest::m_standardWidth * 4U,  // UINT SysMemSlicePitch;
            };

            auto TransitionBarrier = [commandList] (ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)->void
            {
                auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
                commandList->ResourceBarrier(1U, &barrier);
            };

            TransitionBarrier(m_tex.Get(), 
                kDefaultTexState,
                D3D12_RESOURCE_STATE_COPY_DEST);

            UpdateSubresources(commandList, m_tex.Get(), m_texUpload.Get(), 0ULL, 0U, 1U, &initialData);

            TransitionBarrier(m_tex.Get(), 
                D3D12_RESOURCE_STATE_COPY_DEST, 
                kDefaultTexState);

            delete[] initialBytes;
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

            commandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_INDEX_SRV, m_descriptorSrvGpu);

            commandList->OMSetRenderTargets(1, &m_descriptorRtvCpu, FALSE, nullptr);

            commandList->RSSetViewports(1, &m_viewport);
            commandList->RSSetScissorRects(1, &m_scissorRect);

            commandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_INDEX_SAMPLER, m_descriptorSamplerGpu);

            commandList->DrawInstanced(1, m_instances, 0, 0);
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>      m_rootSignature;

        static ComPtr<ID3D12Resource>           m_texUpload;
        static ComPtr<ID3D12Resource>           m_tex;
        static ComPtr<ID3D12DescriptorHeap>     m_descriptorHeap;
        static D3D12_GPU_DESCRIPTOR_HANDLE      m_descriptorSrvGpu;

        static ComPtr<ID3D12Resource>           m_texOut;
        static ComPtr<ID3D12DescriptorHeap>     m_descriptorHeapRtv;
        static D3D12_CPU_DESCRIPTOR_HANDLE      m_descriptorRtvCpu;

        static D3D12_VIEWPORT                   m_viewport;
        static D3D12_RECT                       m_scissorRect;

    private:                    
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>             m_pipelineState;

        float                                   m_anisoLevel;
#ifdef _GAMING_XBOX
        float                                   m_anisoBias;
        uint32_t                                m_anisoThreshold;
#endif

        ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapSampler;
        D3D12_GPU_DESCRIPTOR_HANDLE             m_descriptorSamplerGpu;
   };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature>      AnisoBenchmark::AnisoTest::m_rootSignature;

ComPtr<ID3D12Resource>           AnisoBenchmark::AnisoTest::m_texUpload;
ComPtr<ID3D12Resource>           AnisoBenchmark::AnisoTest::m_tex;
ComPtr<ID3D12DescriptorHeap>     AnisoBenchmark::AnisoTest::m_descriptorHeap;
D3D12_GPU_DESCRIPTOR_HANDLE      AnisoBenchmark::AnisoTest::m_descriptorSrvGpu;

ComPtr<ID3D12Resource>           AnisoBenchmark::AnisoTest::m_texOut;
ComPtr<ID3D12DescriptorHeap>     AnisoBenchmark::AnisoTest::m_descriptorHeapRtv;
D3D12_CPU_DESCRIPTOR_HANDLE      AnisoBenchmark::AnisoTest::m_descriptorRtvCpu;

D3D12_VIEWPORT                   AnisoBenchmark::AnisoTest::m_viewport;
D3D12_RECT                       AnisoBenchmark::AnisoTest::m_scissorRect;

AnisoBenchmark benchmark;
