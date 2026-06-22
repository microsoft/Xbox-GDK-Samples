//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class TrijuicingBenchmark final : public Benchmark
{
public:
    TrijuicingBenchmark() = default;

    ~TrijuicingBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Trijuicing";
    }

    void Initialize(ID3D12Device* device) override
    {
        // Test for all formats, msaa
        for (auto mipLevel : { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f,  })
        {
            for (auto trijuicingLevel : { 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U, 13U, 14U, 15U, })
            {
                AddTest(new TrijuicingTest(trijuicingLevel, mipLevel));
            }
        }

#ifdef _GAMING_XBOX_SCARLETT
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_TEX);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_mipmap_lod_0_samples);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_mipmap_lod_1_samples);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_bilin_point_1_cycle_pixels);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_mip_1_cycle_pixels);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_mip_2_cycle_pixels);
#else
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_VMEM);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_MIPMAP_LOD_0_SAMPLES);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_MIPMAP_LOD_1_SAMPLES);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_BILIN_POINT_1_CYCLE_PIXELS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_MIP_1_CYCLE_PIXELS);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_MIP_2_CYCLE_PIXELS);
#endif

        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(TrijuicingTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(TrijuicingTest::m_rootSignature);

        CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        // Need 2 mips for non-zero trilinear cost
        auto descTex = CD3DX12_RESOURCE_DESC::Tex2D(
            TrijuicingTest::m_format,
            TrijuicingTest::m_standardWidth,  
            TrijuicingTest::m_standardHeight, 
            1, 
            2);
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
            D3D12_HEAP_FLAG_NONE, 
            &descTex, 
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(TrijuicingTest::m_tex.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(TrijuicingTest::m_tex);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapSrv = 
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,          // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapSrv, IID_GRAPHICS_PPV_ARGS(TrijuicingTest::m_descriptorHeap.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(TrijuicingTest::m_descriptorHeap);

        // Use extensions in order to set PerfModulation to its maximum value of 7.
        // The default value is PerfModulation = 4, which reduces the effect of sampler tweaks to 4/7.
        auto descriptorSrvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(TrijuicingTest::m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
            0U, 
            device->GetDescriptorHandleIncrementSize(descHeapSrv.Type));
        TrijuicingTest::m_descriptorSrvGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(TrijuicingTest::m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
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

        device->CreatePlacedShaderResourceViewX(TrijuicingTest::m_tex.Get(), &descSrv, descriptorSrvCpu);

        // We require a render target in order to set viewport dimensions.
        auto descTexOut = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM,
            TrijuicingTest::m_standardWidth,  
            TrijuicingTest::m_standardHeight, 
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
            IID_GRAPHICS_PPV_ARGS(TrijuicingTest::m_texOut.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(TrijuicingTest::m_texOut);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapRtv = 
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapRtv, IID_GRAPHICS_PPV_ARGS(TrijuicingTest::m_descriptorHeapRtv.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(TrijuicingTest::m_descriptorHeapRtv);

        TrijuicingTest::m_descriptorRtvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(TrijuicingTest::m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(), 
            0U, 
            device->GetDescriptorHandleIncrementSize(descHeapRtv.Type));

        device->CreateRenderTargetView(TrijuicingTest::m_texOut.Get(), nullptr, TrijuicingTest::m_descriptorRtvCpu);

        D3D12_VIEWPORT viewport = 
        {
            0,                                                          // FLOAT TopLeftX;
            0,                                                          // FLOAT TopLeftY;
            FLOAT(TrijuicingTest::m_standardWidth),                     // FLOAT Width;
            FLOAT(TrijuicingTest::m_standardHeight),                    // FLOAT Height;
            0.0f,                                                       // FLOAT MinDepth;
            1.0f,                                                       // FLOAT MaxDepth;
        };
        TrijuicingTest::m_viewport = viewport;

        D3D12_RECT scissorRect =
        {
            0,                                                          // LONG    left;
            0,                                                          // LONG    top;
            LONG(TrijuicingTest::m_standardWidth),                      // LONG    right;
            LONG(TrijuicingTest::m_standardHeight),                     // LONG    bottom;
        };
        TrijuicingTest::m_scissorRect = scissorRect;

        TrijuicingTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        TrijuicingTest::m_rootSignature.Reset();

        TrijuicingTest::m_tex.Reset();
        TrijuicingTest::m_descriptorHeap.Reset();

        TrijuicingTest::m_texOut.Reset();
        TrijuicingTest::m_descriptorHeapRtv.Reset();
    }

private:
    class TrijuicingTest final : public Test
    {
    public:
        static constexpr uint32_t m_standardWidth = 256;
        static constexpr uint32_t m_standardHeight = 256;
        static constexpr auto m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
        static constexpr uint32_t m_instances = 32;
        static constexpr uint32_t m_threadPerWave = 64; // Change this to per-test if we ever go to wave32 on Scarlett

        TrijuicingTest(uint32_t trijuicingLevel, float mipLevel) :
            m_descriptorSamplerGpu{},
            m_trijuicingLevel(trijuicingLevel),
            m_mipLevel(mipLevel)
        {
        }

        void Initialize(ID3D12Device* device) override
        {
            std::wostringstream geometryShaderName;
            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto pixelShaderBlob = DX::ReadData(L"TrijuicingPs.cso");

            geometryShaderName << L"Trijuicing" << std::setfill(L'0') << std::setw(3) << (100.0f * m_mipLevel) << L"Gs.cso";
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
                0.0f,                                           // FLOAT AnisotropyBias;
                0U,                                             // UINT AnisotropyThreshold;
                D3D12_COMPARISON_FUNC_ALWAYS,                   // D3D12_COMPARISON_FUNC ComparisonFunc;
                {},                                             // FLOAT BorderColor[4];
                m_trijuicingLevel,                              // UINT PerformanceMip;
                0,                                              // UINT PerformanceZ;
            };
            device->CreateSamplerX(&descSampler, descriptorSamplerCpu);
        }

        void Uninitialize() override
        {
            m_pipelineState.Reset();

            m_tex.Reset();
            m_descriptorHeap.Reset();

            m_descriptorHeapSampler.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Mip: ";
            name << m_mipLevel;
            name << L", ";
            name << L"Trijuicing: ";
            name << m_trijuicingLevel;

            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Mip", L"", 6, 2);
            report->AddColumn(L"Trijuicing", L"", 6);
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Fetch(API)", L"", 12, 0);
            report->AddColumn(L"Fetch(GPU)", L"", 12, 0);
            report->AddColumn(L"Bilinear", L"", 12);
            report->AddColumn(L"Trilinear", L"", 12);
            report->AddColumn(L"VMEM/clock", L"", 5, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;
#ifdef _GAMING_XBOX_SCARLETT
            auto vmemGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_INSTS_TEX) * m_threadPerWave;
#else
            auto vmemGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_INSTS_VMEM) * m_threadPerWave;
#endif
            auto vmemInShader = 64U;
            auto vmemAPI = vmemInShader * m_instances * m_standardWidth * m_standardHeight;
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;
            auto vmemPerClock = (1000.0f * vmemGPU) / (timeMs * clockSpeed); 

#ifdef _GAMING_XBOX_SCARLETT
            auto bilinPoint = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_bilin_point_1_cycle_pixels);
            auto trilinear = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_mip_2_cycle_pixels);
#else
            auto bilinPoint = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_BILIN_POINT_1_CYCLE_PIXELS);
            auto trilinear = GetCounterValue(GPUPerfCounters::TA_PERF_SEL_MIP_2_CYCLE_PIXELS);
#endif
            bilinPoint -= std::min(bilinPoint, trilinear);

            report->AddRowData(m_mipLevel);
            report->AddRowData(m_trijuicingLevel);
            report->AddRowData(timeMs);
            report->AddRowData(vmemAPI);
            report->AddRowData(vmemGPU);
            report->AddRowData(bilinPoint);
            report->AddRowData(trilinear);
            report->AddRowData(vmemPerClock);

            DirectX::XMVECTOR color = (uint32_t(m_mipLevel * 10.0f) % 2) ? DirectX::Colors::White : DirectX::Colors::Tan;
            report->EndRow(color);
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

        ComPtr<ID3D12DescriptorHeap>            m_descriptorHeapSampler;
        D3D12_GPU_DESCRIPTOR_HANDLE             m_descriptorSamplerGpu;

        uint32_t                                m_trijuicingLevel;
        float                                   m_mipLevel;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature>     TrijuicingBenchmark::TrijuicingTest::m_rootSignature;

ComPtr<ID3D12Resource>          TrijuicingBenchmark::TrijuicingTest::m_tex;
ComPtr<ID3D12DescriptorHeap>    TrijuicingBenchmark::TrijuicingTest::m_descriptorHeap;
D3D12_GPU_DESCRIPTOR_HANDLE     TrijuicingBenchmark::TrijuicingTest::m_descriptorSrvGpu;

ComPtr<ID3D12Resource>          TrijuicingBenchmark::TrijuicingTest::m_texOut;
ComPtr<ID3D12DescriptorHeap>    TrijuicingBenchmark::TrijuicingTest::m_descriptorHeapRtv;
D3D12_CPU_DESCRIPTOR_HANDLE     TrijuicingBenchmark::TrijuicingTest::m_descriptorRtvCpu;

D3D12_VIEWPORT                  TrijuicingBenchmark::TrijuicingTest::m_viewport;
D3D12_RECT                      TrijuicingBenchmark::TrijuicingTest::m_scissorRect;

TrijuicingBenchmark benchmark;
