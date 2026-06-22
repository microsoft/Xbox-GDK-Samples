//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class CubemapBenchmark final : public Benchmark
{
public:
    CubemapBenchmark() = default;

    ~CubemapBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Cubemap";
    }

    void Initialize(ID3D12Device* device) override
    {
        // Test for all formats, msaa
        for (auto fetchType : { CubemapTest::CUBEMAP_FETCH_FACE, CubemapTest::CUBEMAP_FETCH_EDGE, CubemapTest::CUBEMAP_FETCH_CORNER, })
        {
            for (auto sliceFilter : { false, true })
            {
                AddTest(new CubemapTest(fetchType, sliceFilter));
            }
        }

#if defined(_GAMING_XBOX_SCARLETT)
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_TEX);
#elif defined(_GAMING_XBOX_XBOXONE)
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_VMEM);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_TOTAL_CUBEEDGE_CYCLES);
        AddCounter(GPUPerfCounters::TA_PERF_SEL_TOTAL_CUBECORNER_CYCLES);
#else // defined(_GAMING_DESKTOP)
        AddCounter_ElapsedTime();
        AddCounter_VMemInstructions();
        AddCounter_PsWaveSize();
#endif

        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(CubemapTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(CubemapTest::m_rootSignature);

        CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        auto descTex = CD3DX12_RESOURCE_DESC::Tex2D(
            CubemapTest::m_format,
            CubemapTest::m_standardWidth,  
            CubemapTest::m_standardHeight, 
            6, 
            1);
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
            D3D12_HEAP_FLAG_NONE, 
            &descTex, 
#ifdef _GAMING_XBOX
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
#else // #ifdef _GAMING_DESKTOP
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
#endif
            nullptr,
            IID_GRAPHICS_PPV_ARGS(CubemapTest::m_tex.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(CubemapTest::m_tex);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapSrv = 
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,          // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapSrv, IID_GRAPHICS_PPV_ARGS(CubemapTest::m_descriptorHeap.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(CubemapTest::m_descriptorHeap);

        // Use extensions in order to set PerfModulation to its maximum value of 7.
        // The default value is PerfModulation = 4, which reduces the effect of sampler tweaks to 4/7.
        auto descriptorSrvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(CubemapTest::m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
            0U, 
            device->GetDescriptorHandleIncrementSize(descHeapSrv.Type));
        CubemapTest::m_descriptorSrvGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(CubemapTest::m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
            0U, 
            device->GetDescriptorHandleIncrementSize(descHeapSrv.Type));

        D3D12_TEXCUBE_SRV texCubeSrv =
        {
            0U,                                                 // UINT MostDetailedMip;
            UINT(-1),                                           // UINT MipLevels;
                                                                // FLOAT ResourceMinLODClamp;
        };
        D3D12_SHADER_RESOURCE_VIEW_DESC descSrv =
        {
            CubemapTest::m_format,                              // DXGI_FORMAT Format;
            D3D12_SRV_DIMENSION_TEXTURECUBE,                    // D3D12_SRV_DIMENSION ViewDimension;
            D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,           // UINT Shader4ComponentMapping;
        };
        descSrv.TextureCube = texCubeSrv;
        device->CreateShaderResourceView(CubemapTest::m_tex.Get(), &descSrv, descriptorSrvCpu);

        // We require a render target in order to set viewport dimensions.
        auto descTexOut = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM,
            CubemapTest::m_standardWidth,  
            CubemapTest::m_standardHeight, 
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
            IID_GRAPHICS_PPV_ARGS(CubemapTest::m_texOut.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(CubemapTest::m_texOut);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapRtv = 
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapRtv, IID_GRAPHICS_PPV_ARGS(CubemapTest::m_descriptorHeapRtv.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(CubemapTest::m_descriptorHeapRtv);

        CubemapTest::m_descriptorRtvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(CubemapTest::m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(), 
            0U, 
            device->GetDescriptorHandleIncrementSize(descHeapRtv.Type));

        device->CreateRenderTargetView(CubemapTest::m_texOut.Get(), nullptr, CubemapTest::m_descriptorRtvCpu);

        D3D12_VIEWPORT viewport = 
        {
            0,                                                          // FLOAT TopLeftX;
            0,                                                          // FLOAT TopLeftY;
            FLOAT(CubemapTest::m_standardWidth),                        // FLOAT Width;
            FLOAT(CubemapTest::m_standardHeight),                       // FLOAT Height;
            0.0f,                                                       // FLOAT MinDepth;
            1.0f,                                                       // FLOAT MaxDepth;
        };
        CubemapTest::m_viewport = viewport;

        D3D12_RECT scissorRect =
        {
            0,                                                          // LONG    left;
            0,                                                          // LONG    top;
            LONG(CubemapTest::m_standardWidth),                         // LONG    right;
            LONG(CubemapTest::m_standardHeight),                        // LONG    bottom;
        };
        CubemapTest::m_scissorRect = scissorRect;

        CubemapTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        CubemapTest::m_rootSignature.Reset();

        CubemapTest::m_tex.Reset();
        CubemapTest::m_descriptorHeap.Reset();

        CubemapTest::m_texOut.Reset();
        CubemapTest::m_descriptorHeapRtv.Reset();
    }

private:
    class CubemapTest final : public Test
    {
    public:
        static constexpr uint32_t m_standardWidth = 256;
        static constexpr uint32_t m_standardHeight = 256;
#ifdef _GAMING_XBOX_SCARLETT
        static constexpr auto m_format = DXGI_FORMAT_R32G32B32A32_FLOAT;    // We want a slow format, to drown out VALU cost
#else
        static constexpr auto m_format = DXGI_FORMAT_R16G16B16A16_UNORM;    // We want a slow format, to drown out VALU cost
#endif
        static constexpr uint32_t m_instances = 64;
        static constexpr uint32_t m_threadPerWave = 64; // Change this to per-test if we ever go to wave32 on Scarlett

        enum CubemapFetchType : uint32_t
        {
            CUBEMAP_FETCH_FACE, 
            CUBEMAP_FETCH_EDGE, 
            CUBEMAP_FETCH_CORNER, 
        };

        CubemapTest(CubemapFetchType fetchType, bool sliceFilter) :
            m_descriptorSamplerGpu{},
            m_fetchType(fetchType), 
            m_faceWrap(sliceFilter)
        {
        }

        const wchar_t* GetFetchTypeName() const
        {
            switch (m_fetchType)
            {
            case CUBEMAP_FETCH_FACE:
                return L"Face";
            case CUBEMAP_FETCH_EDGE:
                return L"Edge";
            case CUBEMAP_FETCH_CORNER:
                return L"Corner";
            default:
                return L"";
            }
        }

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto pixelShaderBlob = DX::ReadData(L"CubemapPs.cso");

            std::wostringstream geometryShaderName;
            geometryShaderName << L"Cubemap" << GetFetchTypeName() << L"Gs.cso";
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

#ifndef _GAMING_DESKTOP
            auto samplerFlags = m_faceWrap ? D3D12XBOX_SAMPLER_FLAG_NONE : D3D12XBOX_SAMPLER_FLAG_DISABLE_CUBEMAP_WRAP;
            D3D12XBOX_SAMPLER_DESC descSampler =
            {
                samplerFlags,                                   // D3D12XBOX_SAMPLER_FLAGS Flags;
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
            {},                                                 // FLOAT BorderColor[4];
            0U,                                                 // UINT PerformanceMip;
            0U,                                                 // UINT PerformanceZ;
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

            name << L"Type: ";
            name << GetFetchTypeName();
            name << L", Wrap: ";
            name << (m_faceWrap ? L"on" : L"off");

            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Fetch type", L"", 10);
            report->AddColumn(L"Face wrap", L"", 10);
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Fetch(API)", L"", 12, 0);
            report->AddColumn(L"Fetch(GPU)", L"", 12, 0);
            report->AddColumn(L"Wraps(API)", L"", 12, 0);
            report->AddColumn(L"Wraps(GPU)", L"", 12, 0);
            report->AddColumn(L"VMEM/clock", L"", 5, 2);
            report->AddColumn(L"% of max", L"%", 6, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
#ifdef _GAMING_XBOX
            const auto & gpuProperties = GpuProperties::Get();
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
#endif // #if defined(_GAMING_XBOX_SCARLETT)

            auto vmemInShader = 64U;
            auto vmemAPI = vmemInShader * m_instances * m_standardWidth * m_standardHeight;
#ifdef _GAMING_XBOX
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;
            auto vmemPerClock = (1000.0f * vmemGPU) / (timeMs * clockSpeed); 

#ifdef _GAMING_XBOX_SCARLETT
            auto pixelPerClockIdeal = 2;    // 2x penalty for 4xfloat32
            auto edgePenalty = 3;           // empirically determined value
            auto cornerPenalty = 5;         // empirically determined value
#else
            auto pixelPerClockIdeal = 2;    // 2x penalty for 4xunorm16
            auto edgePenalty = 2;           // documented value
            auto cornerPenalty = 6;         // documented value
#endif

            auto numSe = gpuProperties.m_numSe;
            auto numCuPerSe = gpuProperties.m_numCuPerSe;
            auto vmemPerClockIdeal = float(numCuPerSe * numSe * pixelPerClockIdeal);

            if (CUBEMAP_FETCH_EDGE == m_fetchType && m_faceWrap)
            {
                vmemPerClockIdeal /= edgePenalty;
            }
            else if (CUBEMAP_FETCH_CORNER == m_fetchType && m_faceWrap)
            {
                vmemPerClockIdeal /= cornerPenalty;
            }
            auto vmemThroughput = 100.0f * vmemPerClock / vmemPerClockIdeal;
#endif // #ifdef _GAMING_XBOX

#if defined(_GAMING_XBOX_SCARLETT) || defined(_GAMING_DESKTOP)
            auto wrap = 0;
#else // defined(_GAMING_XBOX_XBOXONE)
            auto wrap = pixelPerClockIdeal *
                (GetCounterValue(GPUPerfCounters::TA_PERF_SEL_TOTAL_CUBEEDGE_CYCLES) / edgePenalty
                + GetCounterValue(GPUPerfCounters::TA_PERF_SEL_TOTAL_CUBECORNER_CYCLES) / cornerPenalty);
#endif
            auto wrapIdeal = (CUBEMAP_FETCH_FACE == m_fetchType || !m_faceWrap) ? 0 : vmemAPI;

            report->AddRowData(GetFetchTypeName());
            report->AddRowData(m_faceWrap ? L"on" : L"off");
            report->AddRowData(timeMs);
            report->AddRowData(vmemAPI);
            report->AddRowData(vmemGPU);
            report->AddRowData(wrapIdeal);
            // These counters don't exist on Durango or Scarlett
#ifdef _GAMING_XBOX
            if (IsDurangoClass() || IsScarlettClass())
            {
                report->AddRowData(L"unavailable");
            }
            else
#endif // _GAMING_XBOX
            {
                report->AddRowData(wrap);
            }
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

        CubemapFetchType                        m_fetchType;
        bool                                    m_faceWrap;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature>      CubemapBenchmark::CubemapTest::m_rootSignature;

ComPtr<ID3D12Resource>           CubemapBenchmark::CubemapTest::m_tex;
ComPtr<ID3D12DescriptorHeap>     CubemapBenchmark::CubemapTest::m_descriptorHeap;
D3D12_GPU_DESCRIPTOR_HANDLE      CubemapBenchmark::CubemapTest::m_descriptorSrvGpu;

ComPtr<ID3D12Resource>           CubemapBenchmark::CubemapTest::m_texOut;
ComPtr<ID3D12DescriptorHeap>     CubemapBenchmark::CubemapTest::m_descriptorHeapRtv;
D3D12_CPU_DESCRIPTOR_HANDLE      CubemapBenchmark::CubemapTest::m_descriptorRtvCpu;

D3D12_VIEWPORT                   CubemapBenchmark::CubemapTest::m_viewport;
D3D12_RECT                       CubemapBenchmark::CubemapTest::m_scissorRect;

CubemapBenchmark benchmark;

