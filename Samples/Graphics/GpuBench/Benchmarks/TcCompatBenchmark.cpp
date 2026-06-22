//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class TcCompatBenchmark final : public Benchmark
{
public:
    TcCompatBenchmark() = default;

    ~TcCompatBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"TcCompat";
    }

    void Initialize(ID3D12Device* device) override
    {
        for (auto formatParams : TcCompatDccTest::m_allFormatParams)
        {
            for (auto fetchPattern : {TcCompatTest::FETCH_PATTERN_COHERENT, TcCompatTest::FETCH_PATTERN_SCATTERED,})
            {
                for (auto textureCompatible : {false, true,})
                {
                    if (textureCompatible && IsDurangoClass())
                    {
                        continue;   // TcCompat is only supported on Scorpio and above
                    }

                    for (auto optimizedClear : {false, true,})
                    {
                        if (optimizedClear && !textureCompatible)
                        {
                            continue;   // this combination does nothing
                        }

                        if (TcCompatTest::FETCH_PATTERN_COHERENT != fetchPattern && optimizedClear)
                        {
                            continue;   // optimizedClear doesn't care what the fetch pattern is
                        }

                        AddTest(new TcCompatDccSrvTest(formatParams, fetchPattern, textureCompatible, optimizedClear));
                    }
                }
            }
        }

#ifdef _GAMING_XBOX_SCARLETT
        for (auto formatParams : TcCompatDccTest::m_allFormatParams)
        {
            for (auto fetchPattern : {TcCompatTest::FETCH_PATTERN_COHERENT, TcCompatTest::FETCH_PATTERN_SCATTERED,})
            {
                for (auto textureCompatible : {false, true,})
                {
                    if (textureCompatible && IsDurangoClass())
                    {
                        continue;   // TcCompat is only supported on Scorpio and above
                    }

                    for (auto optimizedClear : {false, true,})
                    {
                        if (optimizedClear && !textureCompatible)
                        {
                            continue;   // this combination does nothing
                        }

                        if (TcCompatTest::FETCH_PATTERN_COHERENT != fetchPattern && optimizedClear)
                        {
                            continue;   // optimizedClear doesn't care what the fetch pattern is
                        }

                        AddTest(new TcCompatDccUavTest(formatParams, fetchPattern, textureCompatible, optimizedClear));
                    }
                }
            }
        }
#endif

        for (const auto& formatParams : TcCompatHtileTest::m_allFormatParams)
        {
            for (auto fetchPattern : {TcCompatTest::FETCH_PATTERN_COHERENT, TcCompatTest::FETCH_PATTERN_SCATTERED,})
            {
                for (auto textureCompatible : {false, true,})
                {
                    if (textureCompatible && IsDurangoClass())
                    {
                        continue;   // TcCompat is only supported on Scorpio and above
                    }

                    for (auto optimizedClear : {false, true,})
                    {
                        if (optimizedClear && !textureCompatible)
                        {
                            continue;   // this combination does nothing
                        }

                        if (TcCompatTest::FETCH_PATTERN_COHERENT != fetchPattern && optimizedClear)
                        {
                            continue;   // optimizedClear doesn't care what the fetch pattern is
                        }

                        if (!optimizedClear && textureCompatible && DXGI_FORMAT_D16_UNORM == formatParams.m_formatDsv)
                        {
                            continue;   // D16_UNORM only supports fast clear, and no other compression mode
                        }

                        AddTest(new TcCompatHtileTest(formatParams, fetchPattern, textureCompatible, optimizedClear));
                    }
                }
            }
        }

#ifdef _GAMING_XBOX_SCARLETT
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_REQ_READ);
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_REQ_WRITE);
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_GL1_REQ_READ);
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_GL1_REQ_WRITE);
        AddCounter(GPUPerfCounters::GL1C_PERF_SEL_GL2_REQ_READ);
        AddCounter(GPUPerfCounters::GL1C_PERF_SEL_GL2_REQ_WRITE);

        AddCounter(GPUPerfCounters::GL2C_PERF_SEL_HIT);
        AddCounter(GPUPerfCounters::GL2C_PERF_SEL_MISS);
        AddCounter(GPUPerfCounters::GL2C_PERF_SEL_EA_RDREQ_32B);
        AddCounter(GPUPerfCounters::GL2C_PERF_SEL_EA_RDREQ_64B);
        AddCounter(GPUPerfCounters::GL2C_PERF_SEL_EA_RDREQ_96B);
        AddCounter(GPUPerfCounters::GL2C_PERF_SEL_EA_RDREQ_128B);
        AddCounter(GPUPerfCounters::GL2C_PERF_SEL_MC_WRREQ);
        AddCounter(GPUPerfCounters::GL2C_PERF_SEL_EA_WRREQ_64B);
#else
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_TCC_REQ);
        AddCounter(GPUPerfCounters::TCP_PERF_SEL_TOTAL_ACCESSES);

        AddCounter(GPUPerfCounters::TCC_PERF_SEL_HIT);
        AddCounter(GPUPerfCounters::TCC_PERF_SEL_MISS);
        AddCounter(GPUPerfCounters::TCC_PERF_SEL_MC_RDREQ);
        AddCounter(GPUPerfCounters::TCC_PERF_SEL_MC_WRREQ);
#endif

        // This error fires erroneously when the resource is texture-compatible and created with DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN.
        // DecompressResource - The multisample quality of the source and destination resource must match unless decompressing depth to a color destination, in which case the destination cannot be multisampled.
        m_scopedErrorFilters.push_back(new ScopedErrorFilter(device, 0xD73AF158));

        // ID312Device::CreateCommittedResource: D3D12_RESOURCE_DESC::Flags cannot have D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY set with a DXGI format that includes both depth and stencil.
        m_scopedErrorFilters.push_back(new ScopedErrorFilter(device, 0xF75A7315));

        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(TcCompatTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(TcCompatTest::m_rootSignature);

        TcCompatTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        TcCompatTest::m_rootSignature.Reset();

        for (auto filter : m_scopedErrorFilters)
        {
            delete filter;
        }
        m_scopedErrorFilters.clear();
    }

private:
    std::vector<ScopedErrorFilter*>                     m_scopedErrorFilters;

    class TcCompatTest : public Test
    {
    protected:
        static constexpr uint32_t m_widthIn = 4096;
        static constexpr uint32_t m_heightIn = 2048;

        static constexpr uint32_t m_widthOut = m_widthIn / 4;
        static constexpr uint32_t m_heightOut = m_heightIn / 4;

        // Better to avoid multiple instances, because only the first instance gets cmask compression benefit
        static constexpr uint32_t m_instances = 16;

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

        enum FetchPattern : uint32_t
        {
            FETCH_PATTERN_COHERENT,
            FETCH_PATTERN_SCATTERED, 

            FETCH_PATTERN_COUNT
        };

        TcCompatTest(FetchPattern fetchPattern, bool textureCompatible, bool optimizedClear) :
            m_fetchPattern(fetchPattern),
            m_textureCompatible(textureCompatible),        
            m_optimizedClear(optimizedClear)        
        {
        }

        const wchar_t* GetFetchPatternName() const
        {
            switch (m_fetchPattern)
            {
            case FETCH_PATTERN_COHERENT:
                return L"Coherent";
            case FETCH_PATTERN_SCATTERED:
                return L"Scattered";
            default:
                return L"Invalid";
            }
        };

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Test", L"", 15);
            report->AddColumn(L"Format", L"", 20);
            report->AddColumn(L"Pattern", L"", 10);
            report->AddColumn(L"Compression", L"", 13);
            report->AddColumn(L"L2 rd/wr", L"", 12);
#ifdef _GAMING_XBOX_SCARLETT
            report->AddColumn(L"L0 %", L"%", 7, 2);
#endif
            report->AddColumn(L"L1 %", L"%", 7, 2);
            report->AddColumn(L"L2 %", L"%", 7, 2);
            report->AddColumn(L"L2 % total", L"%", 7, 2); // % of nominal resource size fetched into L2
            report->AddColumn(L"Time", L" ms", 5, 2);

            report->AddHeader();
        }

    protected:
        virtual std::wstring GetCompressedViewName() const = 0;

        uint64_t GetBytesReadL2() const
        {
#ifdef _GAMING_XBOX_SCARLETT
            auto reads32 = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_EA_RDREQ_32B);
            auto reads64 = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_EA_RDREQ_64B);
            auto reads96 = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_EA_RDREQ_96B);
            auto reads128 = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_EA_RDREQ_128B);
            return 32 * reads32 + 64 * reads64 + 96 * reads96 + 128 * reads128;
#else
            auto reads32 = GetCounterValue(GPUPerfCounters::TCC_PERF_SEL_MC_RDREQ);
            return 32 * reads32;
#endif
        }

        uint64_t GetBytesWrittenL2() const
        {
#ifdef _GAMING_XBOX_SCARLETT
            auto writes = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_MC_WRREQ);
            auto writes64 = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_EA_WRREQ_64B);
            auto writes32 = writes - writes64;
            return 32 * writes32 + 64 * writes64;
#else
            auto writes32 = GetCounterValue(GPUPerfCounters::TCC_PERF_SEL_MC_WRREQ);
            return 32 * writes32;
#endif
        }

    public:
        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>  m_rootSignature;

    protected:
        // Properties which differ per test
        FetchPattern                        m_fetchPattern;
        bool                                m_textureCompatible;
        bool                                m_optimizedClear;
    };

    class TcCompatDccTest : public TcCompatTest
    {
    public:
        struct FormatParams
        {
            const wchar_t*                              m_formatName;
            DXGI_FORMAT                                 m_format;
            uint32_t                                    m_bytesPerPixel;
        };
        static const std::vector<FormatParams> m_allFormatParams;

        TcCompatDccTest(const FormatParams& formatParams, FetchPattern fetchPattern, bool textureCompatible, bool optimizedClear) :
            TcCompatTest(fetchPattern, textureCompatible, optimizedClear),
            m_viewportIn{},
            m_scissorRectIn{},
            m_descriptorIndexRtv(0U),
            m_descriptorIncrementSizeRtv(0),
            m_numDescriptorsRtv(0),
            m_descriptorRtvInCpu{},
            m_formatParams(formatParams)
        {
        }

        void Initialize(ID3D12Device* device) override
        {
            D3D12_VIEWPORT viewportIn = 
            {
                0,                                                          // FLOAT TopLeftX;
                0,                                                          // FLOAT TopLeftY;
                FLOAT(m_widthIn),                                           // FLOAT Width;
                FLOAT(m_heightIn),                                          // FLOAT Height;
                0.0f,                                                       // FLOAT MinDepth;
                1.0f,                                                       // FLOAT MaxDepth;
            };
            m_viewportIn = viewportIn;

            D3D12_RECT scissorRectIn =
            {
                0,                                                          // LONG    left;
                0,                                                          // LONG    top;
                LONG(m_widthIn),                                            // LONG    right;
                LONG(m_heightIn),                                           // LONG    bottom;
            };
            m_scissorRectIn = scissorRectIn;

            CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

            auto resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
                | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                | (m_textureCompatible ? D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC : D3D12_RESOURCE_FLAG_NONE)
                | (m_textureCompatible ? D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY : D3D12_RESOURCE_FLAG_NONE);

            auto descTexIn = CD3DX12_RESOURCE_DESC::Tex2D(
                m_formatParams.m_format,
                m_widthIn,
                m_heightIn,
                1U,
                1U,
                1U,
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
                resourceFlags);

            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
                D3D12_HEAP_FLAG_NONE, 
                &descTexIn, 
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_texIn.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_texIn);

            // Allow space for a 2nd RTV descriptor in the derived class
            m_numDescriptorsRtv = 2U;
            D3D12_DESCRIPTOR_HEAP_DESC descHeapRtv = 
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                m_numDescriptorsRtv,                                // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapRtv, IID_GRAPHICS_PPV_ARGS(m_descriptorHeapRtv.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeapRtv);

            m_descriptorIncrementSizeRtv = device->GetDescriptorHandleIncrementSize(descHeapRtv.Type);
            m_descriptorRtvInCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(), 
                m_descriptorIndexRtv, 
                m_descriptorIncrementSizeRtv);
            ++m_descriptorIndexRtv;
            assert(UINT(m_descriptorIndexRtv) <= m_numDescriptorsRtv);

            device->CreateRenderTargetView(m_texIn.Get(), nullptr, m_descriptorRtvInCpu);
        };

        void Uninitialize() override
        {
            m_texIn.Reset();

            m_descriptorHeapRtv.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"DCC";
            name << L": " << GetCompressedViewName();
            name << L", " << m_formatParams.m_formatName;
            name << L", " << GetFetchPatternName();
            if (m_textureCompatible)
            {
                name << L", tex compat";
            }
            if (m_optimizedClear)
            {
                name << L", opt clear";
            }

            return name.str();
        }

    protected:
        // Resources which are unique per-test.

        // These parameters involve the creation and rendering of the DCC render target,
        // independent of how it's used afterwards.
        D3D12_VIEWPORT                      m_viewportIn;
        D3D12_RECT                          m_scissorRectIn;

        ComPtr<ID3D12Resource>              m_texIn;

        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeapRtv;
        int32_t                             m_descriptorIndexRtv;
        uint32_t                            m_descriptorIncrementSizeRtv;
        uint32_t                            m_numDescriptorsRtv;
        D3D12_CPU_DESCRIPTOR_HANDLE         m_descriptorRtvInCpu;

        FormatParams                        m_formatParams;

        static const DirectX::XMVECTORF32   g_optimizedClearValue;
        static const DirectX::XMVECTORF32   g_nonOptimizedClearValue;
    };


    class TcCompatDccSrvTest final : public TcCompatDccTest
    {
    public:
        TcCompatDccSrvTest(const FormatParams& formatParams, FetchPattern fetchPattern, bool textureCompatible, bool optimizedClear) :
            TcCompatDccTest(formatParams, fetchPattern, textureCompatible, optimizedClear),
            m_descriptorSrvGpu{},
            m_descriptorRtvOutCpu{},
            m_descriptorSamplerGpu{},
            m_viewportOut{},
            m_scissorRectOut{}
        {
        }

        void Initialize(ID3D12Device* device) override
        {
            TcCompatDccTest::Initialize(device);

            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto geometryShaderBlob = DX::ReadData(L"FullScreenGs.cso");

            std::wostringstream pixelShaderName;
            pixelShaderName << L"TcCompat" << GetCompressedViewName() << GetFetchPatternName() << L"Ps.cso";
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

            CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

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

            device->CreateShaderResourceView(m_texIn.Get(), nullptr, descriptorSrvCpu);

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

            D3D12_SAMPLER_DESC descSampler = 
            {
                D3D12_FILTER_MIN_MAG_MIP_LINEAR,                // D3D12_FILTER Filter;
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,                // D3D12_TEXTURE_ADDRESS_MODE AddressU;
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,                // D3D12_TEXTURE_ADDRESS_MODE AddressV;
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,                // D3D12_TEXTURE_ADDRESS_MODE AddressW;
                0.0f,                                           // FLOAT MipLODBias;
                1U,                                             // UINT MaxAnisotropy;
                D3D12_COMPARISON_FUNC_ALWAYS,                   // D3D12_COMPARISON_FUNC ComparisonFunc;
                {},                                             // FLOAT BorderColor[4];
                0.0f,                                           // FLOAT MinLOD;
                D3D12_FLOAT32_MAX,                              // FLOAT MaxLOD;
            };
            device->CreateSampler(&descSampler, descriptorSamplerCpu);

            // Make the render target small dimensions to not be limited by pixel rate
            D3D12_VIEWPORT viewportOut = 
            {
                0,                                                          // FLOAT TopLeftX;
                0,                                                          // FLOAT TopLeftY;
                FLOAT(m_widthOut),                                          // FLOAT Width;
                FLOAT(m_heightOut),                                         // FLOAT Height;
                0.0f,                                                       // FLOAT MinDepth;
                1.0f,                                                       // FLOAT MaxDepth;
            };
            m_viewportOut = viewportOut;

            D3D12_RECT scissorRectOut =
            {
                0,                                                          // LONG    left;
                0,                                                          // LONG    top;
                LONG(m_widthOut),                                           // LONG    right;
                LONG(m_heightOut),                                          // LONG    bottom;
            };
            m_scissorRectOut = scissorRectOut;

            auto descTexOut = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_R8G8B8A8_UNORM,
                m_widthOut,
                m_heightOut,
                1U,
                1U,
                1U,
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12XBOX_RESOURCE_FLAG_DENY_COMPRESSION_DATA);

            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
                D3D12_HEAP_FLAG_NONE, 
                &descTexOut, 
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_texOut.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_texOut);

            m_descriptorRtvOutCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(), 
                m_descriptorIndexRtv, 
                m_descriptorIncrementSizeRtv);
            ++m_descriptorIndexRtv;
            assert(UINT(m_descriptorIndexRtv) <= m_numDescriptorsRtv);

            device->CreateRenderTargetView(m_texOut.Get(), nullptr, m_descriptorRtvOutCpu);
        };

        void Uninitialize() override
        {
            m_pipelineState.Reset();

            m_descriptorHeap.Reset();

            m_descriptorHeapSampler.Reset();

            m_texOut.Reset();
        }
        
        std::wstring GetCompressedViewName() const override
        {
            std::wostringstream name;

            name << L"Srv";
            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;
            auto bytesReadL2 = GetBytesReadL2();
            auto bytesTotal = m_widthIn * m_heightIn * m_instances * m_formatParams.m_bytesPerPixel;

#ifdef _GAMING_XBOX_SCARLETT
            auto tcpRequests = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_REQ_READ);
            auto tcpMisses = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_GL1_REQ_READ);
            tcpMisses = std::min(tcpMisses, tcpRequests);
            auto l1Misses = GetCounterValue(GPUPerfCounters::GL1C_PERF_SEL_GL2_REQ_READ);
            l1Misses = std::min(l1Misses, tcpMisses);
            auto l2CacheHits = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_HIT);
            auto l2CacheMisses = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_MISS);
#else
            auto tcpRequests = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_TOTAL_ACCESSES);
            auto tcpMisses = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_TCC_REQ);
            tcpMisses = std::min(tcpMisses, tcpRequests);
            auto l2CacheHits = GetCounterValue(GPUPerfCounters::TCC_PERF_SEL_HIT);
            auto l2CacheMisses = GetCounterValue(GPUPerfCounters::TCC_PERF_SEL_MISS);
#endif

            report->AddRowData(L"Read DCC SRV");
            report->AddRowData(m_formatParams.m_formatName);
            report->AddRowData(GetFetchPatternName());
            if (m_textureCompatible)
            {
                if (m_optimizedClear)
                {
                    report->AddRowData(L"dcc/tcc/opt");
                }
                else
                {
                    report->AddRowData(L"dcc/tcc");
                }
            }
            else
            {
                report->AddRowData(L"none");
            }
            report->AddRowData(bytesReadL2);
            report->AddRowData(100.0f * (tcpRequests - tcpMisses) / tcpRequests);
#ifdef _GAMING_XBOX_SCARLETT
            report->AddRowData(100.0f * (tcpMisses - l1Misses) / tcpMisses);
#endif
            report->AddRowData(100.0f * l2CacheHits / (l2CacheHits + l2CacheMisses));
            report->AddRowData(100.0f * bytesReadL2 / bytesTotal);
            report->AddRowData(timeMs);

            DirectX::XMVECTOR color;
            if (m_textureCompatible)
            {
                if (m_optimizedClear)
                {
                    color = DirectX::Colors::Tan;
                }
                else
                {
                    color = DirectX::Colors::Silver;
                }
            }
            else
            {
                color = DirectX::Colors::Wheat;
            }
            report->EndRow(color);
        }

        // This activity occurs outside the timing brackets
        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            auto TransitionBarrier = [commandList] (ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)->void
            {
                auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
                commandList->ResourceBarrier(1U, &barrier);
            };

            // Initialize render target to clear state
            commandList->ClearRenderTargetView(m_descriptorRtvInCpu, m_optimizedClear ? g_optimizedClearValue : g_nonOptimizedClearValue, 0U, nullptr);

            // Pay any required decompress/sync cost now, without counting it as part of the test
            // If the resource was created with D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY, then the decompress should be skipped automatically?
            TransitionBarrier(m_texIn.Get(), 
                D3D12_RESOURCE_STATE_RENDER_TARGET, 
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            ID3D12DescriptorHeap* descriptorHeaps[] =
            {
                m_descriptorHeap.Get(), 
                m_descriptorHeapSampler.Get(), 
            };
            commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

            commandList->OMSetRenderTargets(1, &m_descriptorRtvOutCpu, FALSE, nullptr);

            commandList->RSSetViewports(1, &m_viewportOut);
            commandList->RSSetScissorRects(1, &m_scissorRectOut);

            commandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_INDEX_SRV, m_descriptorSrvGpu);
            commandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_INDEX_SAMPLER, m_descriptorSamplerGpu);

            commandList->DrawInstanced(1, m_instances, 0, 0);
        }

        // This activity occurs outside the timing brackets
        void Stop(ID3D12GraphicsCommandList* commandList) const override
        {
            auto TransitionBarrier = [commandList] (ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)->void
            {
                auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
                commandList->ResourceBarrier(1U, &barrier);
            };

            // Allow decompress here
            TransitionBarrier(m_texIn.Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 
                D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

    private:
        // These resources involve the use of the DCC render target as an SRV
        ComPtr<ID3D12PipelineState>         m_pipelineState;

        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeap;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorSrvGpu;

        ComPtr<ID3D12Resource>              m_texOut;

        D3D12_CPU_DESCRIPTOR_HANDLE         m_descriptorRtvOutCpu;

        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeapSampler;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorSamplerGpu;

        D3D12_VIEWPORT                      m_viewportOut;
        D3D12_RECT                          m_scissorRectOut;
    };


    class TcCompatDccUavTest final : public TcCompatDccTest
    {
    public:
        TcCompatDccUavTest(const FormatParams& formatParams, FetchPattern fetchPattern, bool textureCompatible, bool optimizedClear) :
            TcCompatDccTest(formatParams, fetchPattern, textureCompatible, optimizedClear),
            m_descriptorUavGpu{}
        {
            switch (m_fetchPattern)
            {
            case FETCH_PATTERN_COHERENT:
            default:
                m_threadGroupX = 8;
                m_threadGroupY = 8;
                m_threadGroupZ = 1;
                break;

            case FETCH_PATTERN_SCATTERED:
                m_threadGroupX = 1;
                m_threadGroupY = 64;
                m_threadGroupZ = 1;
                break;
            }
        }

        void Initialize(ID3D12Device* device) override
        {
            TcCompatDccTest::Initialize(device);

            std::wostringstream computeShaderName;
            computeShaderName << L"TcCompat" << GetCompressedViewName() << GetFetchPatternName() << L"Cs.cso";
            auto computeShaderBlob = DX::ReadData(computeShaderName.str().c_str());

            D3D12_COMPUTE_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignature.Get(),                          // ID3D12RootSignature* pRootSignature;
                {
                    computeShaderBlob.data(),
                    computeShaderBlob.size(),
                },                                              // D3D12_SHADER_BYTECODE VS;
                0,                                              // UINT NodeMask;
                { nullptr, 0, },                                // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            };
            DX::ThrowIfFailed(device->CreateComputePipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineState);

            CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

            D3D12_DESCRIPTOR_HEAP_DESC descHeapUav = 
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                1U,                                                 // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,          // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapUav, IID_GRAPHICS_PPV_ARGS(m_descriptorHeap.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeap);

            auto descriptorUavCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeapUav.Type));
            m_descriptorUavGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeapUav.Type));

            device->CreateUnorderedAccessView(m_texIn.Get(), nullptr, nullptr, descriptorUavCpu);
        };

        void Uninitialize() override
        {
            m_pipelineState.Reset();

            m_descriptorHeap.Reset();
        }

        std::wstring GetCompressedViewName() const override
        {
            std::wostringstream name;

            name << L"Uav";
            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;
            auto bytesWrittenL2 = GetBytesWrittenL2();
            auto bytesTotal = m_widthIn * m_heightIn * m_instances * m_formatParams.m_bytesPerPixel;

#ifdef _GAMING_XBOX_SCARLETT
            auto tcpRequests = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_REQ_WRITE);
            auto tcpMisses = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_GL1_REQ_WRITE);
            tcpMisses = std::min(tcpMisses, tcpRequests);
            auto l1Misses = GetCounterValue(GPUPerfCounters::GL1C_PERF_SEL_GL2_REQ_WRITE);
            l1Misses = std::min(l1Misses, tcpMisses);
            auto l2CacheHits = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_HIT);
            auto l2CacheMisses = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_MISS);
#else
            auto tcpRequests = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_TOTAL_ACCESSES);
            auto tcpMisses = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_TCC_REQ);
            tcpMisses = std::min(tcpMisses, tcpRequests);
            auto l2CacheHits = GetCounterValue(GPUPerfCounters::TCC_PERF_SEL_HIT);
            auto l2CacheMisses = GetCounterValue(GPUPerfCounters::TCC_PERF_SEL_MISS);
#endif

            report->AddRowData(L"Write DCC UAV");
            report->AddRowData(m_formatParams.m_formatName);
            report->AddRowData(GetFetchPatternName());
            if (m_textureCompatible)
            {
                if (m_optimizedClear)
                {
                    report->AddRowData(L"dcc/tcc/opt");
                }
                else
                {
                    report->AddRowData(L"dcc/tcc");
                }
            }
            else
            {
                report->AddRowData(L"none");
            }
            report->AddRowData(bytesWrittenL2);
            report->AddRowData(100.0f * (tcpRequests - tcpMisses) / tcpRequests);
#ifdef _GAMING_XBOX_SCARLETT
            report->AddRowData(100.0f * (tcpMisses - l1Misses) / tcpMisses);
#endif
            report->AddRowData(100.0f * l2CacheHits / (l2CacheHits + l2CacheMisses));
            report->AddRowData(100.0f * bytesWrittenL2 / bytesTotal);
            report->AddRowData(timeMs);

            DirectX::XMVECTOR color;
            if (m_textureCompatible)
            {
                if (m_optimizedClear)
                {
                    color = DirectX::Colors::Tan;
                }
                else
                {
                    color = DirectX::Colors::Silver;
                }
            }
            else
            {
                color = DirectX::Colors::Wheat;
            }
            report->EndRow(color);
        }

        // This activity occurs outside the timing brackets
        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            auto TransitionBarrier = [commandList] (ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)->void
            {
                auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
                commandList->ResourceBarrier(1U, &barrier);
            };

            // Initialize render target to clear state
            commandList->ClearRenderTargetView(m_descriptorRtvInCpu, m_optimizedClear ? g_optimizedClearValue : g_nonOptimizedClearValue, 0U, nullptr);

            // Pay any required decompress/sync cost now, without counting it as part of the test
            // If the resource was created with D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY, then the decompress should be skipped automatically?
            TransitionBarrier(m_texIn.Get(), 
                D3D12_RESOURCE_STATE_RENDER_TARGET, 
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
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

            commandList->SetComputeRootDescriptorTable(ROOT_PARAMETER_INDEX_UAV, m_descriptorUavGpu);

            assert(m_widthIn % m_threadGroupX == 0);
            assert(m_heightIn % m_threadGroupY == 0);
            assert(1U % m_threadGroupZ == 0);
            commandList->Dispatch(m_widthIn / m_threadGroupX, m_heightIn / m_threadGroupY, 1U / m_threadGroupZ);
        }

        // This activity occurs outside the timing brackets
        void Stop(ID3D12GraphicsCommandList* commandList) const override
        {
            auto TransitionBarrier = [commandList] (ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)->void
            {
                auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
                commandList->ResourceBarrier(1U, &barrier);
            };

            // Allow decompress here
            TransitionBarrier(m_texIn.Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 
                D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

    private:
        // These resources involve the use of the DCC render target as an SRV
        ComPtr<ID3D12PipelineState>     m_pipelineState;

        ComPtr<ID3D12DescriptorHeap>    m_descriptorHeap;
        D3D12_GPU_DESCRIPTOR_HANDLE     m_descriptorUavGpu;

        uint32_t                        m_threadGroupX;
        uint32_t                        m_threadGroupY;
        uint32_t                        m_threadGroupZ;
    };


    class TcCompatHtileTest final : public TcCompatTest
    {
    public:
        struct FormatParams
        {
            const wchar_t*                              m_formatName;
            DXGI_FORMAT                                 m_formatTex;
            DXGI_FORMAT                                 m_formatDsv;
            DXGI_FORMAT                                 m_formatSrv;
            uint32_t                                    m_bytesPerPixel;
        };
        static const std::vector<FormatParams> m_allFormatParams;

        TcCompatHtileTest(const FormatParams& formatParams, FetchPattern fetchPattern, bool textureCompatible, bool optimizedClear) :
            TcCompatTest(fetchPattern, textureCompatible, optimizedClear),
            m_viewportIn{},
            m_scissorRectIn{},
            m_descriptorSrvGpu{},
            m_descriptorDsvCpu{},
            m_descriptorRtvOutCpu{},
            m_descriptorSamplerGpu{},
            m_viewportOut{},
            m_scissorRectOut{},
            m_formatParams(formatParams)
        {
        }

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto geometryShaderBlob = DX::ReadData(L"FullScreenGs.cso");

            std::wostringstream pixelShaderName;
            pixelShaderName << L"TcCompat" << GetCompressedViewName() << GetFetchPatternName() << L"Ps.cso";
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

            auto descDssWriteAlways = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            descDssWriteAlways.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            descDssWriteAlways.StencilEnable = TRUE;
            descDssWriteAlways.StencilWriteMask = 0xff;
            descDssWriteAlways.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            descDssWriteAlways.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
            descDssWriteAlways.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
            descDssWriteAlways.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineStateConstantDepth =
            {
                m_rootSignature.Get(),                          // ID3D12RootSignature* pRootSignature;
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
                descDssWriteAlways,                             // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
                {},                                             // D3D12_INPUT_LAYOUT_DESC InputLayout;
                D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,    // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,            // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
                0U,                                             // UINT NumRenderTargets;
                { DXGI_FORMAT_UNKNOWN, },                       // DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
                m_formatParams.m_formatDsv,                     // DXGI_FORMAT DSVFormat;
                { 
                    1U, 
                    DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN, 
                },                                              // DXGI_SAMPLE_DESC SampleDesc;
                0U,                                             // UINT NodeMask;
                {},                                             // D3D12_CACHED_PIPELINE_STATE CachedPSO;
                D3D12_PIPELINE_STATE_FLAG_NONE,                 // D3D12_PIPELINE_STATE_FLAGS Flags;
            };
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineStateConstantDepth, IID_GRAPHICS_PPV_ARGS(m_pipelineStateConstantDepthStencil.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineStateConstantDepthStencil);

            D3D12_VIEWPORT viewportIn = 
            {
                0,                                                          // FLOAT TopLeftX;
                0,                                                          // FLOAT TopLeftY;
                FLOAT(m_widthIn),                                           // FLOAT Width;
                FLOAT(m_heightIn),                                          // FLOAT Height;
                0.0f,                                                       // FLOAT MinDepth;
                1.0f,                                                       // FLOAT MaxDepth;
            };
            m_viewportIn = viewportIn;

            D3D12_RECT scissorRectIn =
            {
                0,                                                          // LONG    left;
                0,                                                          // LONG    top;
                LONG(m_widthIn),                                            // LONG    right;
                LONG(m_heightIn),                                           // LONG    bottom;
            };
            m_scissorRectIn = scissorRectIn;

            CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

            auto resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
                | (m_textureCompatible ? D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY : D3D12_RESOURCE_FLAG_NONE);

            auto descTexIn = CD3DX12_RESOURCE_DESC::Tex2D(
                m_formatParams.m_formatTex,
                m_widthIn,
                m_heightIn,
                1U,
                1U,
                1U,
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
                resourceFlags);

            auto optimizedClearValue = CD3DX12_CLEAR_VALUE(m_formatParams.m_formatDsv,
                g_depthClearValue,
                g_stencilClearValue);
            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
                D3D12_HEAP_FLAG_NONE, 
                &descTexIn, 
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &optimizedClearValue,
                IID_GRAPHICS_PPV_ARGS(m_texIn.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_texIn);

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

            bool isStencil = (1U == m_formatParams.m_bytesPerPixel);
            UINT planeSlice = isStencil ? 1U : 0U;
            D3D12_TEX2D_SRV tex2DSrvIn =
            {
                0U,                                                 // UINT MostDetailedMip;
                UINT(-1),                                           // UINT MipLevels;
                planeSlice,                                         // UINT PlaneSlice;
                                                                    // FLOAT ResourceMinLODClamp;
            };
            D3D12_SHADER_RESOURCE_VIEW_DESC descSrvIn =
            {
                m_formatParams.m_formatSrv,                         // DXGI_FORMAT Format;
                D3D12_SRV_DIMENSION_TEXTURE2D,                      // D3D12_SRV_DIMENSION ViewDimension;
                D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,           // UINT Shader4ComponentMapping;
            };
            descSrvIn.Texture2D = tex2DSrvIn;
            device->CreateShaderResourceView(m_texIn.Get(), &descSrvIn, descriptorSrvCpu);

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
            device->CreateDepthStencilView(m_texIn.Get(), &descDsv, m_descriptorDsvCpu);

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

            D3D12_SAMPLER_DESC descSampler = 
            {
                D3D12_FILTER_MIN_MAG_MIP_LINEAR,                // D3D12_FILTER Filter;
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,                // D3D12_TEXTURE_ADDRESS_MODE AddressU;
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,                // D3D12_TEXTURE_ADDRESS_MODE AddressV;
                D3D12_TEXTURE_ADDRESS_MODE_WRAP,                // D3D12_TEXTURE_ADDRESS_MODE AddressW;
                0.0f,                                           // FLOAT MipLODBias;
                1U,                                             // UINT MaxAnisotropy;
                D3D12_COMPARISON_FUNC_ALWAYS,                   // D3D12_COMPARISON_FUNC ComparisonFunc;
                {},                                             // FLOAT BorderColor[4];
                0.0f,                                           // FLOAT MinLOD;
                D3D12_FLOAT32_MAX,                              // FLOAT MaxLOD;
            };
            device->CreateSampler(&descSampler, descriptorSamplerCpu);

            // Make the render target small dimensions to not be limited by pixel rate
            D3D12_VIEWPORT viewportOut = 
            {
                0,                                                          // FLOAT TopLeftX;
                0,                                                          // FLOAT TopLeftY;
                FLOAT(m_widthOut),                                          // FLOAT Width;
                FLOAT(m_heightOut),                                         // FLOAT Height;
                0.0f,                                                       // FLOAT MinDepth;
                1.0f,                                                       // FLOAT MaxDepth;
            };
            m_viewportOut = viewportOut;

            D3D12_RECT scissorRectOut =
            {
                0,                                                          // LONG    left;
                0,                                                          // LONG    top;
                LONG(m_widthOut),                                           // LONG    right;
                LONG(m_heightOut),                                          // LONG    bottom;
            };
            m_scissorRectOut = scissorRectOut;

            auto descTexOut = CD3DX12_RESOURCE_DESC::Tex2D(
                DXGI_FORMAT_R8G8B8A8_UNORM,
                m_widthOut,
                m_heightOut,
                1U,
                1U,
                1U,
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12XBOX_RESOURCE_FLAG_DENY_COMPRESSION_DATA);

            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
                D3D12_HEAP_FLAG_NONE, 
                &descTexOut, 
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_texOut.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_texOut);

            D3D12_DESCRIPTOR_HEAP_DESC descHeapRtv = 
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                1U,                                                 // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapRtv, IID_GRAPHICS_PPV_ARGS(m_descriptorHeapRtv.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeapRtv);

            m_descriptorRtvOutCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeapRtv.Type));

            device->CreateRenderTargetView(m_texOut.Get(), nullptr, m_descriptorRtvOutCpu);
        };

        void Uninitialize() override
        {
            m_pipelineState.Reset();
            m_pipelineStateConstantDepthStencil.Reset();

            m_texIn.Reset();
            m_descriptorHeap.Reset();
            m_descriptorHeapDsv.Reset();

            m_descriptorHeapSampler.Reset();

            m_texOut.Reset();
            m_descriptorHeapRtv.Reset();
        }

        std::wstring GetCompressedViewName() const override
        {
            std::wostringstream name;

            name << L"Srv";
            return name.str();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Htile";
            name << L": " << GetCompressedViewName();
            name << L": " << m_formatParams.m_formatName;
            name << L", " << GetFetchPatternName();
            if (m_textureCompatible)
            {
                name << L", tex compat";
            }
            if (m_optimizedClear)
            {
                name << L", opt clear";
            }

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;
            auto bytesReadL2 = GetBytesReadL2();
            auto bytesTotal = m_widthIn * m_heightIn * m_instances * m_formatParams.m_bytesPerPixel;

#ifdef _GAMING_XBOX_SCARLETT
            auto tcpRequests = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_REQ_READ);
            auto tcpMisses = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_GL1_REQ_READ);
            tcpMisses = std::min(tcpMisses, tcpRequests);
            auto l1Misses = GetCounterValue(GPUPerfCounters::GL1C_PERF_SEL_GL2_REQ_READ);
            l1Misses = std::min(l1Misses, tcpMisses);
            auto l2CacheHits = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_HIT);
            auto l2CacheMisses = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_MISS);
#else
            auto tcpRequests = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_TOTAL_ACCESSES);
            auto tcpMisses = GetCounterValue(GPUPerfCounters::TCP_PERF_SEL_TCC_REQ);
            tcpMisses = std::min(tcpMisses, tcpRequests);
            auto l2CacheHits = GetCounterValue(GPUPerfCounters::TCC_PERF_SEL_HIT);
            auto l2CacheMisses = GetCounterValue(GPUPerfCounters::TCC_PERF_SEL_MISS);
#endif

            report->AddRowData(L"Read Htile");
            report->AddRowData(m_formatParams.m_formatName);
            report->AddRowData(GetFetchPatternName());
            if (m_textureCompatible)
            {
                if (m_optimizedClear)
                {
                    report->AddRowData(L"htile/tcc/opt");
                }
                else
                {
                    report->AddRowData(L"htile/tcc");
                }
            }
            else
            {
                report->AddRowData(L"none");
            }
            report->AddRowData(bytesReadL2);
            report->AddRowData(100.0f * (tcpRequests - tcpMisses) / tcpRequests);
#ifdef _GAMING_XBOX_SCARLETT
            report->AddRowData(100.0f * (tcpMisses - l1Misses) / tcpMisses);
#endif
            report->AddRowData(100.0f * l2CacheHits / (l2CacheHits + l2CacheMisses));
            report->AddRowData(100.0f * bytesReadL2 / bytesTotal);
            report->AddRowData(timeMs);

            DirectX::XMVECTOR color;
            if (m_textureCompatible)
            {
                if (m_optimizedClear)
                {
                    color = DirectX::Colors::Tan;
                }
                else
                {
                    color = DirectX::Colors::Silver;
                }
            }
            else
            {
                color = DirectX::Colors::Wheat;
            }
            report->EndRow(color);
        }

        void WriteConstantToDepthStencil(ID3D12GraphicsCommandList* commandList) const
        {
            // Write a constant value to depth/stencil, but not as the fast clear value
            // Use a single primitive, so that every tile is one-plane/single-value compressed
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineStateConstantDepthStencil.Get());

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

            commandList->OMSetRenderTargets(0U, nullptr, FALSE, &m_descriptorDsvCpu);
            commandList->OMSetStencilRef(g_stencilNonClearValue);

            commandList->RSSetViewports(1, &m_viewportIn);
            commandList->RSSetScissorRects(1, &m_scissorRectIn);

            commandList->DrawInstanced(1, 1, 0, 0);
        }

        // This activity occurs outside the timing brackets
        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            auto TransitionBarrier = [commandList] (ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)->void
            {
                auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
                commandList->ResourceBarrier(1U, &barrier);
            };

            // Initialize depth buffer to clear state
            commandList->ClearDepthStencilView(m_descriptorDsvCpu,
                D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                g_depthClearValue,
                g_stencilClearValue,
                0U,
                nullptr);

            if (m_textureCompatible)
            {
                if (!m_optimizedClear)
                {
                    // Manually write a constant value, but with compression enabled
                    // This gives the highest possible compression besides "fast-cleared"
                    WriteConstantToDepthStencil(commandList);
                }
            }

            // Pay any required decompress/sync cost now, without counting it as part of the test
            auto preserveFlags = D3D12_RESOURCE_STATES(0U)
                | (m_textureCompatible ? D3D12XBOX_RESOURCE_STATE_PRESERVE_COMPRESSED_DEPTH_STENCIL : D3D12_RESOURCE_STATES(0U));

            TransitionBarrier(m_texIn.Get(), 
                D3D12_RESOURCE_STATE_DEPTH_WRITE | preserveFlags,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | preserveFlags);
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

            commandList->OMSetRenderTargets(1, &m_descriptorRtvOutCpu, FALSE, nullptr);

            commandList->RSSetViewports(1, &m_viewportOut);
            commandList->RSSetScissorRects(1, &m_scissorRectOut);

            commandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_INDEX_SRV, m_descriptorSrvGpu);
            commandList->SetGraphicsRootDescriptorTable(ROOT_PARAMETER_INDEX_SAMPLER, m_descriptorSamplerGpu);

            commandList->DrawInstanced(1, m_instances, 0, 0);
        }

        // This activity occurs outside the timing brackets
        void Stop(ID3D12GraphicsCommandList* commandList) const override
        {
            auto TransitionBarrier = [commandList] (ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)->void
            {
                auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
                commandList->ResourceBarrier(1U, &barrier);
            };

            // Allow decompress here
            TransitionBarrier(m_texIn.Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 
                D3D12_RESOURCE_STATE_DEPTH_WRITE);
        }

    private:
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>         m_pipelineState;
        ComPtr<ID3D12PipelineState>         m_pipelineStateConstantDepthStencil;
        
        D3D12_VIEWPORT                      m_viewportIn;
        D3D12_RECT                          m_scissorRectIn;

        ComPtr<ID3D12Resource>              m_texIn;
        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeap;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorSrvGpu;

        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeapDsv;
        D3D12_CPU_DESCRIPTOR_HANDLE         m_descriptorDsvCpu;

        ComPtr<ID3D12Resource>              m_texOut;

        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeapRtv;
        D3D12_CPU_DESCRIPTOR_HANDLE         m_descriptorRtvOutCpu;

        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeapSampler;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorSamplerGpu;

        D3D12_VIEWPORT                      m_viewportOut;
        D3D12_RECT                          m_scissorRectOut;

        FormatParams                        m_formatParams;

        static const FLOAT                  g_depthClearValue;
        static const FLOAT                  g_depthNonClearValue;
        static const UINT8                  g_stencilClearValue;
        static const UINT8                  g_stencilNonClearValue;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature> TcCompatBenchmark::TcCompatTest::m_rootSignature;

const std::vector<TcCompatBenchmark::TcCompatDccTest::FormatParams> TcCompatBenchmark::TcCompatDccTest::m_allFormatParams = 
{
    // We want to cover all different bpp values, because DCC has dependencies on bpp
    { L"R8_UNORM",              DXGI_FORMAT_R8_UNORM,           1,  },      
    { L"R8G8_UNORM",            DXGI_FORMAT_R8G8_UNORM,         2,  },      
    { L"R8G8B8A8_UNORM",        DXGI_FORMAT_R8G8B8A8_UNORM,     4,  },      
    { L"R16G16B16A16_FLOAT",    DXGI_FORMAT_R16G16B16A16_FLOAT, 8,  }, 
    { L"R32G32B32A32_FLOAT",    DXGI_FORMAT_R32G32B32A32_FLOAT, 16, }, 
};

const DirectX::XMVECTORF32 TcCompatBenchmark::TcCompatDccTest::g_optimizedClearValue = DirectX::Colors::White;
const DirectX::XMVECTORF32 TcCompatBenchmark::TcCompatDccTest::g_nonOptimizedClearValue = DirectX::Colors::Red;

const std::vector<TcCompatBenchmark::TcCompatHtileTest::FormatParams> TcCompatBenchmark::TcCompatHtileTest::m_allFormatParams = 
{
    { L"R16_UNORM", DXGI_FORMAT_R16_TYPELESS,       DXGI_FORMAT_D16_UNORM,              DXGI_FORMAT_R16_UNORM,                  2,  },      
    { L"R32_FLOAT", DXGI_FORMAT_R32_TYPELESS,       DXGI_FORMAT_D32_FLOAT,              DXGI_FORMAT_R32_FLOAT,                  4,  },      
    { L"G8_UINT",   DXGI_FORMAT_R32G8X24_TYPELESS,  DXGI_FORMAT_D32_FLOAT_S8X24_UINT,   DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,    1,  },      
};

const FLOAT TcCompatBenchmark::TcCompatHtileTest::g_depthClearValue = 0.0f;     // doesn't match depth in FullScreenGs.hlsl
const FLOAT TcCompatBenchmark::TcCompatHtileTest::g_depthNonClearValue = 1.0f;  // matches depth in FullScreenGs.hlsl (so this gets written automatically)
const UINT8 TcCompatBenchmark::TcCompatHtileTest::g_stencilClearValue = 0;
const UINT8 TcCompatBenchmark::TcCompatHtileTest::g_stencilNonClearValue = 1;

TcCompatBenchmark benchmark;


