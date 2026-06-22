//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class DccBenchmark final : public Benchmark
{
public:
    DccBenchmark() :
        Benchmark()
    {
    }

    ~DccBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"DCC";
    }

    void Initialize(ID3D12Device* device) override
    {
        for (auto formatParams : DccWriteToRTTest::m_allFormatParams)
        {
            for (auto blend : {false, true,})
            {
                for (auto cmask : {false, true,})
                {
                    for (auto dcc : {false, true,})
                    {
                        if (dcc && IsDurangoClass())
                        {
                            continue;   // DCC is only supported on Scorpio and above
                        }

                        if (cmask && dcc)
                        {
                            continue;   // this combination only makes sense for MSAA/EQAA
                        }

                        for (auto textureCompatible : {false, true,})
                        {
                            if (textureCompatible && !dcc)
                            {
                                continue;   // this combination does nothing
                            }

                            for (auto optimizedClearValue : {false, true,})
                            {
                                if (optimizedClearValue && !dcc)
                                {
                                    continue;   // this combination does nothing
                                }

                                AddTest(new DccWriteToRTTest(formatParams, blend, cmask, dcc, textureCompatible, optimizedClearValue));
                            }
                        }
                    }
                }
            }
        }

#ifdef _GAMING_XBOX_SCARLETT
        AddCounter(GPUPerfCounters::GL2C_PERF_SEL_MC_RDREQ);
#else
        AddCounter(GPUPerfCounters::TCC_PERF_SEL_MC_RDREQ);
#endif
        AddCounter(GPUPerfCounters::CB_PERF_SEL_CC_MC_READ_REQUEST);
        AddCounter(GPUPerfCounters::CB_PERF_SEL_CC_MC_WRITE_REQUEST);
        AddCounter(GPUPerfCounters::CB_PERF_SEL_FC_MC_READ_REQUEST);
        AddCounter(GPUPerfCounters::CB_PERF_SEL_FC_MC_WRITE_REQUEST);
        AddCounter(GPUPerfCounters::CB_PERF_SEL_CM_MC_READ_REQUEST);
        AddCounter(GPUPerfCounters::CB_PERF_SEL_CM_MC_WRITE_REQUEST);
        AddCounter(GPUPerfCounters::CB_PERF_SEL_FC_MC_DCC_READ_REQUEST);
        AddCounter(GPUPerfCounters::CB_PERF_SEL_FC_MC_DCC_WRITE_REQUEST);

        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(DccTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(DccTest::m_rootSignature);

        DccTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        DccTest::m_rootSignature.Reset();
    }

private:
    class DccTest : public Test
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

        DccTest() 
        {
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Test", L"", 8);
            report->AddColumn(L"Format", L"", 20);
            report->AddColumn(L"Compression", L"", 24);
            report->AddColumn(L"L2 read", L"", 12);
            report->AddColumn(L"CB read", L"", 12);
            report->AddColumn(L"CB write", L"", 12);
            report->AddColumn(L"CB % of default", L"", 6, 2);
            report->AddColumn(L"Time", L" ms", 5, 2);

            report->AddHeader();
        }

    protected:
        uint64_t GetBytesReadL2() const
        {
#ifdef _GAMING_XBOX_SCARLETT
            auto reads32 = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_MC_RDREQ);
#else
            auto reads32 = GetCounterValue(GPUPerfCounters::TCC_PERF_SEL_MC_RDREQ);
#endif

            return 32 * reads32;
        }

        uint64_t GetBytesReadCB() const
        {
            auto reads32 = GetCounterValue(GPUPerfCounters::CB_PERF_SEL_CC_MC_READ_REQUEST)
                + GetCounterValue(GPUPerfCounters::CB_PERF_SEL_FC_MC_READ_REQUEST)
                + GetCounterValue(GPUPerfCounters::CB_PERF_SEL_CM_MC_READ_REQUEST)
                + GetCounterValue(GPUPerfCounters::CB_PERF_SEL_FC_MC_DCC_READ_REQUEST);

            return 32 * reads32;
        }

        uint64_t GetBytesWrittenCB() const
        {
            auto writes32 = GetCounterValue(GPUPerfCounters::CB_PERF_SEL_CC_MC_WRITE_REQUEST)
                + GetCounterValue(GPUPerfCounters::CB_PERF_SEL_FC_MC_WRITE_REQUEST)
                + GetCounterValue(GPUPerfCounters::CB_PERF_SEL_CM_MC_WRITE_REQUEST)
                + GetCounterValue(GPUPerfCounters::CB_PERF_SEL_FC_MC_DCC_WRITE_REQUEST);

            return 32 * writes32;
        }

    public:
        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature> m_rootSignature;
    };

    class DccWriteToRTTest final : public DccTest
    {
    public:
        static constexpr uint32_t m_width = 4096;
        static constexpr uint32_t m_height = 2048;

        // Better to avoid multiple instances, because only the first instance gets cmask compression benefit
        static constexpr uint32_t m_instances = 1;

        struct FormatParams
        {
            const wchar_t*                              m_formatName;
            DXGI_FORMAT                                 m_format;
            uint32_t                                    m_bytesPerPixel;
        };
        static const std::vector<FormatParams> m_allFormatParams;

        DccWriteToRTTest(const FormatParams& formatParams, bool blend, bool cmask, bool dcc, bool textureCompatible, bool optimizedClearValue) :
            m_texInSizeInBytes(0ULL),
            m_descriptorSrvGpu{},
            m_descriptorSamplerGpu{},
            m_viewport{},
            m_scissorRect{},
            m_texInSizeOutBytes(0ULL),
            m_descriptorRtvCpu{},
            m_formatParams(formatParams),
            m_blend(blend),
            m_cmask(cmask),
            m_dcc(dcc),
            m_textureCompatible(textureCompatible),
            m_optimizedClear(optimizedClearValue)
        {
        }

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto geometryShaderBlob = DX::ReadData(L"FullScreenGs.cso");
            auto pixelShaderBlob = DX::ReadData(L"DccWriteRtPs.cso");

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
                { m_formatParams.m_format, },                   // DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
                DXGI_FORMAT_UNKNOWN,                            // DXGI_FORMAT DSVFormat;
                { 
                    1U, 
                    DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN, 
                },                                              // DXGI_SAMPLE_DESC SampleDesc;
                0U,                                             // UINT NodeMask;
                {},                                             // D3D12_CACHED_PIPELINE_STATE CachedPSO;
                D3D12_PIPELINE_STATE_FLAG_NONE,                 // D3D12_PIPELINE_STATE_FLAGS Flags;
            };
            if (m_blend)
            {
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

                descPipelineState.BlendState.RenderTarget[0] = blendSubtractive;
            }
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

            auto descTexIn = CD3DX12_RESOURCE_DESC::Tex2D(
                m_formatParams.m_format,
                m_width,
                m_height,
                1U, 
                1U);
            m_texInSizeInBytes = device->GetResourceAllocationInfo(0U, 1U, &descTexIn).SizeInBytes;
            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
                D3D12_HEAP_FLAG_NONE, 
                &descTexIn, 
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
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
                D3D12_FILTER_MIN_MAG_MIP_POINT,                 // D3D12_FILTER Filter;
                D3D12_TEXTURE_ADDRESS_MODE_MIRROR,              // D3D12_TEXTURE_ADDRESS_MODE AddressU;
                D3D12_TEXTURE_ADDRESS_MODE_MIRROR,              // D3D12_TEXTURE_ADDRESS_MODE AddressV;
                D3D12_TEXTURE_ADDRESS_MODE_MIRROR,              // D3D12_TEXTURE_ADDRESS_MODE AddressW;
                0.0f,                                           // FLOAT MipLODBias;
                1U,                                             // UINT MaxAnisotropy;
                D3D12_COMPARISON_FUNC_ALWAYS,                   // D3D12_COMPARISON_FUNC ComparisonFunc;
                {},                                             // FLOAT BorderColor[4];
                0.0f,                                           // FLOAT MinLOD;
                D3D12_FLOAT32_MAX,                              // FLOAT MaxLOD;
            };
            device->CreateSampler(&descSampler, descriptorSamplerCpu);

            auto resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
                | ((m_cmask || m_dcc) ? D3D12_RESOURCE_FLAG_NONE : D3D12XBOX_RESOURCE_FLAG_DENY_COLOR_COMPRESSION_DATA)
                | (m_dcc ? D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC : D3D12_RESOURCE_FLAG_NONE)
                | (m_textureCompatible ? D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY : D3D12_RESOURCE_FLAG_NONE);
            auto descTexOut = CD3DX12_RESOURCE_DESC::Tex2D(
                m_formatParams.m_format,
                m_width,
                m_height,
                1U,
                1U,
                1U,
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
                resourceFlags);
            m_texInSizeOutBytes = device->GetResourceAllocationInfo(0U, 1U, &descTexOut).SizeInBytes;
            auto clearValue = CD3DX12_CLEAR_VALUE(m_formatParams.m_format, g_optimizedClearValue);
            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
                D3D12_HEAP_FLAG_NONE, 
                &descTexOut, 
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                &clearValue,
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

            m_descriptorRtvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(), 
                0U, 
                device->GetDescriptorHandleIncrementSize(descHeapRtv.Type));

            device->CreateRenderTargetView(m_texOut.Get(), nullptr, m_descriptorRtvCpu);
        };

        void Uninitialize() override
        {
            m_pipelineState.Reset();

            m_texIn.Reset();
            m_texInSizeInBytes = 0ULL;
            m_descriptorHeap.Reset();

            m_descriptorHeapSampler.Reset();

            m_texOut.Reset();
            m_texInSizeOutBytes = 0ULL;
            m_descriptorHeapRtv.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << (m_blend ? L"BlendRT " : L"WriteRT ");
            name << m_formatParams.m_formatName;
            if (m_cmask)
            {
                name << L": cmask";
            }
            else if (m_dcc)
            {
                name << L": dcc";
                if (m_textureCompatible)
                {
                    name << L", tex compat";
                }
                if (m_optimizedClear)
                {
                    name << L", opt clear";
                }
            }

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;
            //auto bytesReadExpectedL2 = m_width * m_height * m_instances * m_formatParams.m_bytesPerPixel;
            auto bytesReadL2 = GetBytesReadL2();
            auto bytesReadExpectedCB = m_blend ? (m_width * m_height * m_instances * m_formatParams.m_bytesPerPixel) : 0;
            auto bytesReadCB = GetBytesReadCB();
            auto bytesWrittenExpectedCB = m_width * m_height * m_instances * m_formatParams.m_bytesPerPixel;
            auto bytesWrittenCB = GetBytesWrittenCB();

            report->AddRowData(m_blend ? L"BlendRT" : L"WriteRT");
            report->AddRowData(m_formatParams.m_formatName);
            if (m_cmask)
            {
                report->AddRowData(L"cmask");
            }
            else if (m_dcc)
            {
                if (m_textureCompatible)
                {
                    if (m_optimizedClear)
                    {
                        report->AddRowData(L"dcc/tc-compat/opt-clear");
                    }
                    else
                    {
                        report->AddRowData(L"dcc/tc-compat");
                    }
                }
                else
                {
                    if (m_optimizedClear)
                    {
                        report->AddRowData(L"dcc/opt-clear");
                    }
                    else
                    {
                        report->AddRowData(L"dcc");
                    }
                }
            }
            else
            {
                report->AddRowData(L"none");
            }
            report->AddRowData(bytesReadL2);
            report->AddRowData(bytesReadCB);
            report->AddRowData(bytesWrittenCB);
            report->AddRowData(100.0f * (bytesReadCB + bytesWrittenCB) / (bytesReadExpectedCB + bytesWrittenExpectedCB));
            report->AddRowData(timeMs);

            DirectX::XMVECTOR color;
            if (m_cmask)
            {
                color = DirectX::Colors::Tan;
            }
            else if (m_dcc)
            {
                color = DirectX::Colors::Silver;
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
            // Initialize input texture to compressible data!!!
            // Try to use 1.0f, to produce a non-trivial subtractive blend
            auto fillValueIn = 0U;
            switch (m_formatParams.m_format)
            {
            case DXGI_FORMAT_R8_UNORM:
            case DXGI_FORMAT_R8G8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
                fillValueIn = 0xffffffff;
                break;
            case DXGI_FORMAT_R16G16B16A16_FLOAT:
                fillValueIn = 0x3c003c00;
                break;
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
                fillValueIn = 0x3f800000;
                break;
            default:
                throw(std::exception("Need a fill value for the source texture"));
                break;
            }
            commandList->FillMemoryWith32BitValueX(m_texIn->GetGPUVirtualAddress(), m_texInSizeInBytes, fillValueIn, D3D12XBOX_COPY_FLAG_NONE);

            // Initialize render target to clear state
            commandList->ClearRenderTargetView(m_descriptorRtvCpu, m_optimizedClear ? g_optimizedClearValue : g_nonOptimizedClearValue, 0U, nullptr);
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

            commandList->OMSetRenderTargets(1U, &m_descriptorRtvCpu, FALSE, nullptr);

            commandList->RSSetViewports(1, &m_viewport);
            commandList->RSSetScissorRects(1, &m_scissorRect);

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

            // Trigger a decompress
            TransitionBarrier(m_texOut.Get(), 
                D3D12_RESOURCE_STATE_RENDER_TARGET, 
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            // Initialize output texture to garbage for next pass, to make sure we aren't relying on previous contents
            auto fillValueOut = 0x12345678U;
            commandList->FillMemoryWith32BitValueX(m_texOut->GetGPUVirtualAddress(), m_texInSizeOutBytes, fillValueOut, D3D12XBOX_COPY_FLAG_NONE);

            TransitionBarrier(m_texOut.Get(), 
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 
                D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

    private:
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>         m_pipelineState;

        ComPtr<ID3D12Resource>              m_texIn;
        UINT64                              m_texInSizeInBytes;
        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeap;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorSrvGpu;

        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeapSampler;
        D3D12_GPU_DESCRIPTOR_HANDLE         m_descriptorSamplerGpu;

        D3D12_VIEWPORT                      m_viewport;
        D3D12_RECT                          m_scissorRect;

        ComPtr<ID3D12Resource>              m_texOut;
        UINT64                              m_texInSizeOutBytes;
        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeapRtv;
        D3D12_CPU_DESCRIPTOR_HANDLE         m_descriptorRtvCpu;

        FormatParams                        m_formatParams;
        bool                                m_blend;
        bool                                m_cmask;
        bool                                m_dcc;
        bool                                m_textureCompatible;
        bool                                m_optimizedClear;

        static const DirectX::XMVECTORF32                   g_optimizedClearValue;
        static const DirectX::XMVECTORF32                   g_nonOptimizedClearValue;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature>      DccBenchmark::DccTest::m_rootSignature;

const std::vector<DccBenchmark::DccWriteToRTTest::FormatParams> DccBenchmark::DccWriteToRTTest::m_allFormatParams = 
{
    // We want to cover all different bpp values, because DCC has dependencies on bpp
    { L"R8_UNORM",              DXGI_FORMAT_R8_UNORM,           1,  },      
    { L"R8G8_UNORM",            DXGI_FORMAT_R8G8_UNORM,         2,  },      
    { L"R8G8B8A8_UNORM",        DXGI_FORMAT_R8G8B8A8_UNORM,     4,  },      
    { L"R16G16B16A16_FLOAT",    DXGI_FORMAT_R16G16B16A16_FLOAT, 8,  }, 
    { L"R32G32B32A32_FLOAT",    DXGI_FORMAT_R32G32B32A32_FLOAT, 16, }, 
};

const DirectX::XMVECTORF32 DccBenchmark::DccWriteToRTTest::g_optimizedClearValue = DirectX::Colors::White;
const DirectX::XMVECTORF32 DccBenchmark::DccWriteToRTTest::g_nonOptimizedClearValue = DirectX::Colors::Red; // need a value which will produce non-degenerate blend results

DccBenchmark benchmark;

