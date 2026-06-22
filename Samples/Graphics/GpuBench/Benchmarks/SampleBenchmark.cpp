//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class SampleBenchmark final : public Benchmark
{
public:
    SampleBenchmark() = default;

    ~SampleBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Sample";
    }

    void Initialize(ID3D12Device* device) override
    {
        // Test for all formats, msaa
        for (const auto& textureParams : SampleTest::m_allTextureParams)
        {
            for (const auto& sampleParams : SampleTest::m_allSampleParams)
            {
                // Some combinations don't exist in HLSL
                if (D3D_SRV_DIMENSION_TEXTURECUBE == textureParams.m_dimensionType && sampleParams.m_offset)
                {
                    continue;
                }
                if (D3D_SRV_DIMENSION_TEXTURE3D == textureParams.m_dimensionType && 
                    (SampleTest::SAMPLE_CMP == sampleParams.m_type || SampleTest::SAMPLE_CMP_LEVEL_ZERO == sampleParams.m_type))
                {
                    continue;
                }

                // This pixel shader instance crashes fxc (works in dxdc though)
                if (D3D_SRV_DIMENSION_TEXTURECUBE == textureParams.m_dimensionType && sampleParams.m_clamp)
                {
                    continue;
                }

                AddTest(new SampleTest(sampleParams, textureParams));
            }
        }

#ifdef _GAMING_XBOX_SCARLETT
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_TEX);
#else
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_VMEM);
#endif

        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(SampleTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(SampleTest::m_rootSignature);

        CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        // We require a render target in order to set viewport dimensions.
        // Texture benchmarks must go through the graphics pipeline in order to get automatic derivatives.
        auto descTexOut = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM,
            SampleTest::m_standardWidth,  
            SampleTest::m_standardHeight, 
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
            IID_GRAPHICS_PPV_ARGS(SampleTest::m_texOut.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(SampleTest::m_texOut);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapRtv = 
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapRtv, IID_GRAPHICS_PPV_ARGS(SampleTest::m_descriptorHeapRtv.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(SampleTest::m_descriptorHeapRtv);

        SampleTest::m_descriptorRtvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(SampleTest::m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(), 
            0U, 
            device->GetDescriptorHandleIncrementSize(descHeapRtv.Type));

        device->CreateRenderTargetView(SampleTest::m_texOut.Get(), nullptr, SampleTest::m_descriptorRtvCpu);

        D3D12_VIEWPORT viewport = 
        {
            0,                                                          // FLOAT TopLeftX;
            0,                                                          // FLOAT TopLeftY;
            FLOAT(SampleTest::m_standardWidth),                        // FLOAT Width;
            FLOAT(SampleTest::m_standardHeight),                       // FLOAT Height;
            0.0f,                                                       // FLOAT MinDepth;
            1.0f,                                                       // FLOAT MaxDepth;
        };
        SampleTest::m_viewport = viewport;

        D3D12_RECT scissorRect =
        {
            0,                                                          // LONG    left;
            0,                                                          // LONG    top;
            LONG(SampleTest::m_standardWidth),                         // LONG    right;
            LONG(SampleTest::m_standardHeight),                        // LONG    bottom;
        };
        SampleTest::m_scissorRect = scissorRect;

        SampleTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        SampleTest::m_rootSignature.Reset();

        SampleTest::m_texOut.Reset();
        SampleTest::m_descriptorHeapRtv.Reset();
    }

private:
    class SampleTest final : public Test
    {
    public:
        static constexpr uint32_t m_standardWidth = 256;
        static constexpr uint32_t m_standardHeight = 256;
        static constexpr uint32_t m_standardDepth = 16;
        static constexpr auto m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
        static constexpr uint32_t m_instances = 32;
        static constexpr uint32_t m_threadPerWave = 64; // Change this to per-test if we ever go to wave32 on Scarlett

        // These are orthogonal options on the GPU, but they are exclusive in HLSL semantics
        enum SampleType : uint32_t
        {
            SAMPLE_DEFAULT,
            SAMPLE_BIAS,
            SAMPLE_GRAD,
            SAMPLE_LEVEL,
            SAMPLE_CMP,
            SAMPLE_CMP_LEVEL_ZERO,

            SAMPLE_COUNT
        };

        struct TextureParams
        {
            D3D_SRV_DIMENSION                           m_dimensionType;
            const wchar_t*                              m_dimensionName;

            uint32_t                                    m_gradientBlocks;
        };

        static const std::vector<TextureParams>         m_allTextureParams;

        // Some but not all combinations of these options are supported in HLSL semantics
        struct SampleParams
        {
            SampleType                                  m_type;
            const wchar_t*                              m_typeName;
            const wchar_t*                              m_shortName;

            bool                                        m_offset;
            bool                                        m_clamp;
            bool                                        m_status;

            float                                       m_clockPerVmemIdeal;
            float                                       m_clockPerDim;
        };

        static const std::vector<SampleParams>          m_allSampleParams;

        SampleTest(const SampleParams& sampleParams, const TextureParams& textureParams) :
            m_descriptorSrvGpu{},
            m_descriptorSamplerGpu{},
            m_textureParams(textureParams),
            m_sampleParams(sampleParams)
        {
        }

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto geometryShaderBlob = DX::ReadData(L"SampleGs.cso");

            std::wostringstream pixelShaderName;
            pixelShaderName << m_sampleParams.m_typeName;
            pixelShaderName << (m_sampleParams.m_clamp ? L"Clamp" : L"");
            pixelShaderName << (m_sampleParams.m_offset ? L"Offset" : L"");
            pixelShaderName << (m_sampleParams.m_status ? L"Status" : L"");
            pixelShaderName << L"_";
            pixelShaderName << m_textureParams.m_dimensionName;
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

            // To be safe and omit filtering cost, set everything to point, and turn off cubemap wrap
            D3D12XBOX_SAMPLER_DESC descSampler =
            {
                D3D12XBOX_SAMPLER_FLAG_DISABLE_CUBEMAP_WRAP,    // D3D12XBOX_SAMPLER_FLAGS Flags;
                D3D12XBOX_TEXTURE_XY_FILTER_POINT,              // D3D12XBOX_TEXTURE_XY_FILTER FilterMag;
                D3D12XBOX_TEXTURE_XY_FILTER_POINT,              // D3D12XBOX_TEXTURE_XY_FILTER FilterMin;
                D3D12XBOX_TEXTURE_MIP_FILTER_POINT,             // D3D12XBOX_TEXTURE_MIP_FILTER FilterMip;
                D3D12XBOX_TEXTURE_Z_FILTER_POINT,               // D3D12XBOX_TEXTURE_Z_FILTER FilterZ;
                D3D12XBOX_TEXTURE_FILTER_MODE_LERP,             // D3D12XBOX_TEXTURE_FILTER_MODE FilterMode;
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,               // D3D12_TEXTURE_ADDRESS_MODE AddressU;
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,               // D3D12_TEXTURE_ADDRESS_MODE AddressV;
                D3D12_TEXTURE_ADDRESS_MODE_CLAMP,               // D3D12_TEXTURE_ADDRESS_MODE AddressW;
                0.0f,                                           // FLOAT MinLOD;
                0.0f,                                           // FLOAT MaxLOD;
                0.0f,                                           // FLOAT MipLODBias;
                0.0f,                                           // FLOAT MipLODBiasSecondary;
                1U,                                             // UINT MaxAnisotropy;
                0.0f,                                           // FLOAT AnisotropyBias;
                0U,                                             // UINT AnisotropyThreshold;
                D3D12_COMPARISON_FUNC_ALWAYS,                   // D3D12_COMPARISON_FUNC ComparisonFunc;
                {},                                             // FLOAT BorderColor[4];
                0U,                                             // UINT PerformanceMip;
                0U,                                             // UINT PerformanceZ;
            };
            device->CreateSamplerX(&descSampler, descriptorSamplerCpu);

            CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

            D3D12_RESOURCE_DESC descTex = {};

            // Create the appropriate type of texture for the test
            switch (m_textureParams.m_dimensionType)
            {
            case D3D_SRV_DIMENSION_TEXTURE1D:
            {
                descTex = CD3DX12_RESOURCE_DESC::Tex1D(
                    m_format,
                    m_standardWidth,
                    1,
                    1);
                break;
            }

            case D3D_SRV_DIMENSION_TEXTURE2D:
            {
                descTex = CD3DX12_RESOURCE_DESC::Tex2D(
                    m_format,
                    m_standardWidth,
                    SampleTest::m_standardHeight,
                    1,
                    1);
                break;
            }

            case D3D_SRV_DIMENSION_TEXTURECUBE:
            {
                descTex = CD3DX12_RESOURCE_DESC::Tex2D(
                    m_format,
                    m_standardWidth,
                    SampleTest::m_standardHeight,
                    6,
                    1);
                break;
            }

            case D3D_SRV_DIMENSION_TEXTURE3D:
            {
                descTex = CD3DX12_RESOURCE_DESC::Tex3D(
                    m_format,
                    m_standardWidth,
                    SampleTest::m_standardHeight,
                    SampleTest::m_standardDepth,
                    1);
                break;
            }

            default:
                throw(std::exception("Add support for unrecognized texture dimension."));
                break;
            }

            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
                D3D12_HEAP_FLAG_NONE, 
                &descTex, 
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
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

            auto descriptorSrvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeapSrv.Type));
            m_descriptorSrvGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeapSrv.Type));

            if (D3D_SRV_DIMENSION_TEXTURECUBE == m_textureParams.m_dimensionType)
            {
                // Cubemap views require a DESC
                D3D12_TEXCUBE_SRV texCubeSrv =
                {
                    0U,                                                 // UINT MostDetailedMip;
                    UINT(-1),                                           // UINT MipLevels;
                                                                        // FLOAT ResourceMinLODClamp;
                };
                D3D12_SHADER_RESOURCE_VIEW_DESC descSrv =
                {
                    m_format,                                           // DXGI_FORMAT Format;
                    D3D12_SRV_DIMENSION_TEXTURECUBE,                    // D3D12_SRV_DIMENSION ViewDimension;
                    D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,           // UINT Shader4ComponentMapping;
                };
                descSrv.TextureCube = texCubeSrv;
                device->CreateShaderResourceView(m_tex.Get(), &descSrv, descriptorSrvCpu);
            }
            else
            {
                device->CreateShaderResourceView(m_tex.Get(), nullptr, descriptorSrvCpu);
            }
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

            name << m_sampleParams.m_shortName;
            name << (m_sampleParams.m_clamp ? L"Clamp" : L"");
            name << (m_sampleParams.m_offset ? L"Offset" : L"");
            name << (m_sampleParams.m_status ? L"Status" : L"");
            name << L", " << m_textureParams.m_dimensionName;

            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Op", L"", 12, 2);
            report->AddColumn(L"Dimension", L"", 12, 2);
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Fetch(API)", L"", 12, 0);
            report->AddColumn(L"Fetch(GPU)", L"", 12, 0);
            report->AddColumn(L"VMEM/clock", L"", 5, 2);
            report->AddColumn(L"% of max", L"%", 6, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            const auto& gpuProperties = GpuProperties::Get();

            std::wostringstream name;

            name << m_sampleParams.m_shortName;
            name << (m_sampleParams.m_clamp ? L"Clamp" : L"");
            name << (m_sampleParams.m_offset ? L"Offset" : L"");
            name << (m_sampleParams.m_status ? L"Status" : L"");

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

            auto clockPerVmemIdeal = m_sampleParams.m_clockPerVmemIdeal + m_sampleParams.m_clockPerDim * m_textureParams.m_gradientBlocks;
            auto numSe = gpuProperties.m_numSe;
            auto numCuPerSe = gpuProperties.m_numCuPerSe;
            auto numSimdPerCu = gpuProperties.m_numSimdPerCu;
            auto vmemPerClockIdeal = (numSimdPerCu * numCuPerSe * numSe) / float(clockPerVmemIdeal);

            report->AddRowData(name.str());
            report->AddRowData(m_textureParams.m_dimensionName);
            report->AddRowData(timeMs);
            report->AddRowData(vmemAPI);
            report->AddRowData(vmemGPU);
            report->AddRowData(vmemPerClock);
            report->AddRowData(100.0f * vmemPerClock / vmemPerClockIdeal);

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

            commandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_INDEX_SRV, m_descriptorSrvGpu);

            commandList->OMSetRenderTargets(1, &m_descriptorRtvCpu, FALSE, nullptr);

            commandList->RSSetViewports(1, &m_viewport);
            commandList->RSSetScissorRects(1, &m_scissorRect);

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

        TextureParams                           m_textureParams;
        SampleParams                            m_sampleParams;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature>     SampleBenchmark::SampleTest::m_rootSignature;

ComPtr<ID3D12Resource>          SampleBenchmark::SampleTest::m_texOut;
ComPtr<ID3D12DescriptorHeap>    SampleBenchmark::SampleTest::m_descriptorHeapRtv;
D3D12_CPU_DESCRIPTOR_HANDLE     SampleBenchmark::SampleTest::m_descriptorRtvCpu;

D3D12_VIEWPORT                  SampleBenchmark::SampleTest::m_viewport;
D3D12_RECT                      SampleBenchmark::SampleTest::m_scissorRect;

const std::vector<SampleBenchmark::SampleTest::TextureParams> SampleBenchmark::SampleTest::m_allTextureParams =
{
    { D3D_SRV_DIMENSION_TEXTURE1D,      L"Texture1D",   1,  },
    { D3D_SRV_DIMENSION_TEXTURE2D,      L"Texture2D",   2,  },
    { D3D_SRV_DIMENSION_TEXTURECUBE,    L"TextureCube", 2,  },  // At instruction level, cubemap gradients are 2D (face id is not affected by gradients)
    { D3D_SRV_DIMENSION_TEXTURE3D,      L"Texture3D",   4,  },  // 3D gradients are packed with some padding, so they cost 4 not 3
};

// Clamp, Level, Slice, Sample are all included in the base TA data.
// Bias, Offset, Gradients, Cmp-Value are all additional TA data with extra cycle cost.
const std::vector<SampleBenchmark::SampleTest::SampleParams> SampleBenchmark::SampleTest::m_allSampleParams =
{
    { SAMPLE_DEFAULT,           L"Sample",                  L"Sample",          false,  false,  false,  1.0f,   0.0f,   },
    { SAMPLE_DEFAULT,           L"Sample",                  L"Sample",          true,   false,  false,  1.25f,  0.0f,   },  // offset penalty doesn't depend on dimension, due to packing of offsets
    { SAMPLE_DEFAULT,           L"Sample",                  L"Sample",          false,  true,   false,  1.0f,   0.0f,   },
    { SAMPLE_DEFAULT,           L"Sample",                  L"Sample",          false,  false,  true,   1.0f,   0.0f,   },
    { SAMPLE_BIAS,              L"SampleBias",              L"SampleBias",      false,  false,  false,  1.25f,  0.0f,   },
    { SAMPLE_GRAD,              L"SampleGrad",              L"SampleGrad",      false,  false,  false,  1.0f,   0.5f,   },  // grad penalty depends on dimension
    { SAMPLE_LEVEL,             L"SampleLevel",             L"SampleLevel",     false,  false,  false,  1.0f,   0.0f,   },
    { SAMPLE_CMP,               L"SampleCmp",               L"SampleCmp",       false,  false,  false,  1.25f,  0.0f,   },
    { SAMPLE_CMP_LEVEL_ZERO,    L"SampleCmpLevelZero",      L"SampleCmpLZ",     false,  false,  false,  1.25f,  0.0f,   },

    // A couple of combinations, to show how features interact
    { SAMPLE_BIAS,              L"SampleBias",              L"Bias",            true,   false,  false,  1.50f,  0.0f,   },  // Bias and offset: penalties add
    { SAMPLE_GRAD,              L"SampleGrad",              L"Grad",            false,  true,   false,  1.0f,   0.5f,   },  // Gradient and clamp: clamp is still free
};

SampleBenchmark benchmark;
