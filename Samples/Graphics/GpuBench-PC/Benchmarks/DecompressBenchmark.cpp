//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class DecompressBenchmark final : public Benchmark
{
public:
    DecompressBenchmark() = default;

    ~DecompressBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Decompress";
    }

    void Initialize(ID3D12Device* device) override
    {
        for (auto isDcc : { false, true, })
        {
            if (IsDurangoClass() && isDcc)
            {
                // Durango does not support DCC compression
                continue;
            }

            for (const auto& formatParams : DecompressTestColor::m_allFormatParams)
            {
                for (auto compressionState : { DecompressTest::COMPRESSION_STATE_CLEAR, DecompressTest::COMPRESSION_STATE_SINGLE_VALUE, DecompressTest::COMPRESSION_STATE_UNCOMPRESSED, })
                {
                    AddTest(new DecompressTestColor(isDcc, formatParams, compressionState));
                }
            }
        }

        for (const auto& formatParams : DecompressTestDepth::m_allFormatParams)
        {
            for (auto compressionState : { DecompressTest::COMPRESSION_STATE_CLEAR, DecompressTest::COMPRESSION_STATE_SINGLE_VALUE, DecompressTest::COMPRESSION_STATE_UNCOMPRESSED, })
            {
                AddTest(new DecompressTestDepth(formatParams, compressionState));
            }
        }

#ifdef _GAMING_XBOX_SCARLETT
        AddCounter(GPUPerfCounters::GL2C_PERF_SEL_EA_RDREQ_DRAM_32B);
        AddCounter(GPUPerfCounters::GL2C_PERF_SEL_EA_WRREQ_DRAM_32B);
#else
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
#endif
        // This error fires erroneously when the resource is texture-compatible and created with DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN.
        // DecompressResource - The multisample quality of the source and destination resource must match unless decompressing depth to a color destination, in which case the destination cannot be multisampled.
        m_scopedErrorFilters.push_back(new ScopedErrorFilter(device, 0xD73AF158));

        // D3D12 validation doesn't know that Xbox One supports SV_Stencil
        // ID3D12Device::CreateGraphicsPipelineState: Shader uses output Stencil Ref, but the device does not support this. To check for support, check device caps via the CheckFeatureSupport() API [ STATE_CREATION ERROR #93 ]
        m_scopedErrorFilters.push_back(new ScopedErrorFilter(device, 0x16DA826B));


        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0,
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(DecompressTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(DecompressTest::m_rootSignature);

        DecompressTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        for (auto filter : m_scopedErrorFilters)
        {
            delete filter;
        }
        m_scopedErrorFilters.clear();
    }

private:
    std::vector<ScopedErrorFilter*>                     m_scopedErrorFilters;

    class DecompressTest : public Test
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

        enum CompressionState : uint32_t
        {
            COMPRESSION_STATE_CLEAR,
            COMPRESSION_STATE_SINGLE_VALUE,
            COMPRESSION_STATE_UNCOMPRESSED,

            COMPRESSION_STATE_COUNT
        };

        const wchar_t* GetCompressionStateName() const
        {
            switch (m_compressionState)
            {
            case COMPRESSION_STATE_CLEAR:
                return L"Clear";
            case COMPRESSION_STATE_SINGLE_VALUE:
                return L"Single-value";
            case COMPRESSION_STATE_UNCOMPRESSED:
                return L"Uncompressed";
            default:
                return L"Invalid";
            }
        }

        DecompressTest(CompressionState compressionState ) :
            m_viewport{},
            m_scissorRect{},
            m_compressionState(compressionState)
        {
        }

        void Initialize(ID3D12Device* /*device*/) override
        {
        }

        void Uninitialize() override
        {
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Decompress", L"", 20);
            report->AddColumn(L"State", L"", 12);
            report->AddColumn(L"Format", L"", 20);
            report->AddColumn(L"Width", L"", 7);
            report->AddColumn(L"Height", L"", 7);
            report->AddColumn(L"Time", L" ms", 7, 4);
            report->AddColumn(L"Read", L" MB", 6, 2);
            report->AddColumn(L"Write", L" MB", 6, 2);
            report->AddColumn(L"BW", L" GB/s", 6, 2);

            report->AddHeader();
        }

        static ComPtr<ID3D12RootSignature>  m_rootSignature;

        D3D12_VIEWPORT                      m_viewport;
        D3D12_RECT                          m_scissorRect;

    protected:
        static constexpr uint32_t m_width = 4096;
        static constexpr uint32_t m_height = 2048;

        CompressionState                    m_compressionState;
    };

    class DecompressTestColor final : public DecompressTest
    {
    public:
        struct FormatParams
        {
            const wchar_t* m_formatName;
            DXGI_FORMAT                                 m_format;
            uint32_t                                    m_bytesPerPixel;
            uint32_t                                    m_bytesPerChannel;  // Needed for forcing dcckey uncompressed
        };
        static const std::vector<FormatParams> m_allFormatParams;

        DecompressTestColor(bool isDcc, const FormatParams& formatParams, CompressionState compressionState)
            : DecompressTest(compressionState)
            , m_descriptorRtvCpu{}
            , m_metadataAddress(0)
            , m_isDcc(isDcc)
            , m_formatParams(formatParams)
        {
        }

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto geometryShaderBlob = DX::ReadData(L"FullScreenGs.cso");

            std::vector<uint8_t> pixelShaderBlob = {};
            switch (m_compressionState)
            {
            case COMPRESSION_STATE_CLEAR:
                // No pixel shader needed
                break;
            case COMPRESSION_STATE_SINGLE_VALUE:
                // Pixel shader which writes a single value
                pixelShaderBlob = DX::ReadData(L"DecompressWriteColorSingleValuePs.cso");
                break;
            case COMPRESSION_STATE_UNCOMPRESSED:
            {    // Pixel shader which writes a uncompressible data
                std::wostringstream pixelShaderName;
                pixelShaderName << L"DecompressWriteColorUncompressed" << m_formatParams.m_bytesPerChannel << L"BpcPs.cso";
                pixelShaderBlob = DX::ReadData(pixelShaderName.str().c_str());
            }
            break;
            default:
                assert(false);
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
                m_formatParams.m_format,
                m_width,
                m_height,
                1U,
                1U,
                1U,
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

            if (m_isDcc)
            {
                descTexColor.Flags |= D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC;
            }

            auto optimizedClearValue = CD3DX12_CLEAR_VALUE(m_formatParams.m_format,
                g_optimizedClearValue);
            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &descTexColor,
                D3D12_RESOURCE_STATE_COMMON,
                &optimizedClearValue,
                IID_GRAPHICS_PPV_ARGS(m_renderTarget.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_renderTarget);

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

            D3D12_TEX2D_RTV tex2DRtv =
            {
                // UINT MipSlice;
                // UINT PlaneSlice;
            };
            D3D12_RENDER_TARGET_VIEW_DESC descRtv =
            {
                m_formatParams.m_format,                            // DXGI_FORMAT Format;
                D3D12_RTV_DIMENSION_TEXTURE2D,                      // D3D12_RTV_DIMENSION ViewDimension;
            };
            descRtv.Texture2D = tex2DRtv;
            device->CreateRenderTargetView(m_renderTarget.Get(), &descRtv, m_descriptorRtvCpu);

            // Find the address of the metadata plane which does the compression
            m_metadataAddress = m_renderTarget->GetGPUVirtualAddress();
            bool found = false;
            XGTextureAddressComputer* addressComputer = nullptr;
            DX::ThrowIfFailed(XGCreateTextureComputer(reinterpret_cast<XG_RESOURCE_DESC*>(&descTexColor), &addressComputer));
            XG_RESOURCE_LAYOUT layout = {};
            DX::ThrowIfFailed(addressComputer->GetResourceLayout(&layout));
            for (auto i = 0U; i < layout.Planes; ++i)
            {
                auto& plane = layout.Plane[i];
                switch (plane.Usage)
                {
                case XG_PLANE_USAGE_COLOR_MASK:
                    if (!m_isDcc)
                    {
                        m_metadataAddress += plane.BaseOffsetBytes;
                        found = true;
                    }
                    break;
                case XG_PLANE_USAGE_DELTA_COLOR_COMPRESSION:
                    if (m_isDcc)
                    {
                        m_metadataAddress += plane.BaseOffsetBytes;
                        found = true;
                    }
                    break;
                case XG_PLANE_USAGE_DEFAULT:
                    break;
                default:
                    assert(false);
                }
            }
#ifdef _DEBUG
            assert(found || (!m_isDcc && (m_formatParams.m_bytesPerPixel > 8)));    // no cmask compression for 128 bit types
#else
            (void)found;
#endif
        }

        void Uninitialize() override
        {
            m_renderTarget.Reset();
            m_descriptorHeapRtv.Reset();
            m_pipelineState.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Decompress";
            name << (m_isDcc ? L"(Color<--Dcc) " : L"(Color<--Cmask) ");
            name << L"from ";
            name << GetCompressionStateName();
            name << L": " << m_formatParams.m_formatName;

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;

#ifdef _GAMING_XBOX_SCARLETT
            auto reads32 = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_EA_RDREQ_DRAM_32B);
            auto writes32 = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_EA_WRREQ_DRAM_32B);
            auto bytesRead = 32 * reads32;
            auto bytesWritten = 32 * writes32;
#else
            auto reads32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_ALL_READ_RESPONSE_DATA_BEATS_CH0)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_ALL_READ_RESPONSE_DATA_BEATS_CH1);
            auto bytesRead = 32 * reads32;

            auto writes32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_STALLABLE_WRITE_CH0)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_STALLABLE_WRITE_CH1)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_NONSTALLABLE_WRITE_CH0)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_NONSTALLABLE_WRITE_CH1);
            auto writes64 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_STALLABLE_WRITE_CH0)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_STALLABLE_WRITE_CH1)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_NONSTALLABLE_WRITE_CH0)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_NONSTALLABLE_WRITE_CH1);
            auto bytesWritten = 32 * writes32 + 64 * writes64;
#endif
            auto bytes = bytesRead + bytesWritten;
            auto gbPerSec = (bytes / float(GB)) / timeMs * 1000.0f;

            report->AddRowData(m_isDcc ? L"Color<--Dcc" : L"Color<--Cmask");
            report->AddRowData(GetCompressionStateName());
            report->AddRowData(m_formatParams.m_formatName);
            report->AddRowData(m_width);
            report->AddRowData(m_height);
            report->AddRowData(timeMs);
            report->AddRowData(bytesRead / float(MB));
            report->AddRowData(bytesWritten / float(MB));
            report->AddRowData(gbPerSec);

            DirectX::XMVECTOR color;
            switch (m_formatParams.m_bytesPerPixel)
            {
            case 1:
                color = DirectX::Colors::White;
                break;
            case 2:
                color = DirectX::Colors::Tan;
                break;
            case 4:
                color = DirectX::Colors::White;
                break;
            case 8:
                color = DirectX::Colors::Tan;
                break;
            case 16:
            default:
                color = DirectX::Colors::White;
                break;
            }
            report->EndRow(color);
        }

        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            auto TransitionBarrier = [commandList](ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)->void
                {
                    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
                    commandList->ResourceBarrier(1U, &barrier);
                };

            // Transition to compressible state
            TransitionBarrier(m_renderTarget.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

            // Fast clear
            commandList->ClearRenderTargetView(m_descriptorRtvCpu, g_optimizedClearValue, 0, nullptr);

            // Do some rendering
            switch (m_compressionState)
            {
            case COMPRESSION_STATE_CLEAR:
                break;  // do nothing
            case COMPRESSION_STATE_SINGLE_VALUE:
                // Fill the render target with a single color (most compressible data besides clear color)
                // fallthrough
            case COMPRESSION_STATE_UNCOMPRESSED:
                // Fullscreen draw which writes uncompressible color data
                commandList->SetGraphicsRootSignature(m_rootSignature.Get());
                commandList->SetPipelineState(m_pipelineState.Get());

                commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

                commandList->OMSetRenderTargets(1, &m_descriptorRtvCpu, FALSE, nullptr);

                commandList->RSSetViewports(1, &m_viewport);
                commandList->RSSetScissorRects(1, &m_scissorRect);

                commandList->DrawInstanced(1, 1, 0, 0);
                break;
            default:
                assert(false);
            };

            // Clear caches --- happens automatically
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            // Test metadata for expected values
#ifdef _DEBUG
            if (m_isDcc)
            {
                auto dcckey = reinterpret_cast<const uint8_t*>(m_metadataAddress)[0];
                switch (m_compressionState)
                {
                case COMPRESSION_STATE_CLEAR:
                    assert((dcckey & 0x1f) == 0x00);    // all the clear codes which read nothing from color use only the top 3 bits
                    break;
                case COMPRESSION_STATE_SINGLE_VALUE:
                    switch (m_formatParams.m_bytesPerPixel)
                    {
                    case 1:
                        assert(dcckey == 0x08 || dcckey == 0x10 || dcckey == 0x55);   // Sometimes only get 8:4 compressed in practice
                        break;
#ifdef _GAMING_XBOX_XBOXONE
                    case 2:
                        assert(dcckey == 0x08 || dcckey == 0x10 || dcckey == 0x22);   // Sometimes only get 8:2 compressed in practice
                        break;
                    case 16:
                        assert(dcckey == 0x08 || dcckey == 0x10 || dcckey == 0x28);   // Sometimes only get 8:2 compressed in practice
                        break;
#endif
                    default:
                        assert(dcckey == 0x08 || dcckey == 0x10);   // Either 8:1 compressed, or single-value compressed
                    }
                    break;
                case COMPRESSION_STATE_UNCOMPRESSED:
                    assert(dcckey == 0xff);           // if this fails, the data is somehow not uncompressible enough
                    break;
                default:
                    assert(false);
                };
            }
            else if (m_formatParams.m_bytesPerPixel <= 8)    // no cmask compression for 128 bit types
            {
                // Todo: Support cmask with fmask
                // One byte is really 2 cmask values
                auto cmask = reinterpret_cast<const uint8_t*>(m_metadataAddress)[0];
                switch (m_compressionState)
                {
                case COMPRESSION_STATE_CLEAR:
                    assert(cmask == 0x00);
                    break;
                case COMPRESSION_STATE_SINGLE_VALUE:
                    assert(cmask == 0xff);
                    break;
                case COMPRESSION_STATE_UNCOMPRESSED:
                    assert(cmask == 0xff);
                    break;
                default:
                    assert(false);
                };
            }
#endif

            auto TransitionBarrier = [commandList](ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)->void
                {
                    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
                    commandList->ResourceBarrier(1U, &barrier);
                };

            // Force a decompress
            TransitionBarrier(m_renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
        }

    protected:
        static const DirectX::XMVECTORF32   g_optimizedClearValue;

        ComPtr<ID3D12PipelineState>         m_pipelineState;

        ComPtr<ID3D12Resource>              m_renderTarget;
        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeapRtv;
        D3D12_CPU_DESCRIPTOR_HANDLE         m_descriptorRtvCpu;
        D3D12_GPU_VIRTUAL_ADDRESS           m_metadataAddress;

        bool                                m_isDcc;
        FormatParams                        m_formatParams;
    };

    class DecompressTestDepth final : public DecompressTest
    {
        public:
        struct FormatParams
        {
            const wchar_t* m_formatName;
            DXGI_FORMAT                                 m_formatTex;
            DXGI_FORMAT                                 m_formatDsv;
            DXGI_FORMAT                                 m_formatSrv;
            uint32_t                                    m_bytesPerPixel;
        };
        static const std::vector<FormatParams> m_allFormatParams;

        DecompressTestDepth(const FormatParams& formatParams, CompressionState compressionState)
            : DecompressTest(compressionState)
            , m_descriptorDsvCpu{}
            , m_metadataAddress(0)
            , m_formatParams(formatParams)
        {
        }

        void Initialize(ID3D12Device* device) override
        {
            bool isStencil = (1U == m_formatParams.m_bytesPerPixel);

            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto geometryShaderBlob = DX::ReadData(L"FullScreenGs.cso");

            std::vector<uint8_t> pixelShaderBlob = {};
            auto depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            switch (m_compressionState)
            {
            case COMPRESSION_STATE_CLEAR:
                // No pixel shader needed
                break;
            case COMPRESSION_STATE_SINGLE_VALUE:
                // No pixel shader needed
            {
                // Write uncompressed depth or stencil always
                if (isStencil)
                {
                    depthStencilState.DepthEnable = FALSE;
                    depthStencilState.StencilEnable = TRUE;
                    depthStencilState.StencilWriteMask = 0xff;
                    depthStencilState.BackFace.StencilFunc = depthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                    depthStencilState.BackFace.StencilPassOp = depthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
                }
                else
                {
                    depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                }
            }
                break;
            case COMPRESSION_STATE_UNCOMPRESSED:
            {
                std::wostringstream pixelShaderName;
                pixelShaderName << (isStencil ? L"DecompressWriteStencilUncompressedPs.cso" : L"DecompressWriteDepthUncompressedPs.cso");
                pixelShaderBlob = DX::ReadData(pixelShaderName.str().c_str());

                // Write uncompressed depth or stencil always
                if (isStencil)
                {
                    depthStencilState.DepthEnable = FALSE;
                    depthStencilState.StencilEnable = TRUE;
                    depthStencilState.StencilWriteMask = 0xff;
                    depthStencilState.BackFace.StencilFunc = depthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                    depthStencilState.BackFace.StencilPassOp = depthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
                }
                else
                {
                    depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                }
            }
                break;
            default:
                assert(false);
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

            CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

            auto descTexDepth = CD3DX12_RESOURCE_DESC::Tex2D(
                m_formatParams.m_formatTex,
                m_width,
                m_height,
                1U,
                1U,
                1U,
                DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
                | D3D12XBOX_RESOURCE_FLAG_DENY_DEPTH_EXPCLEAR); // or else a decompress may skip work

            auto optimizedClearValue = CD3DX12_CLEAR_VALUE(m_formatParams.m_formatDsv,
                g_depthClearValue,
                g_stencilClearValue);
            DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &descTexDepth,
                D3D12_RESOURCE_STATE_COMMON,
                &optimizedClearValue,
                IID_GRAPHICS_PPV_ARGS(m_depthBuffer.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_depthBuffer);

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
            device->CreateDepthStencilView(m_depthBuffer.Get(), &descDsv, m_descriptorDsvCpu);

            // Find the address of the metadata plane which does the compression
            m_metadataAddress = m_depthBuffer->GetGPUVirtualAddress();
            bool found = false;
            XGTextureAddressComputer* addressComputer = nullptr;
            DX::ThrowIfFailed(XGCreateTextureComputer(reinterpret_cast<XG_RESOURCE_DESC*>(&descTexDepth), &addressComputer));
            XG_RESOURCE_LAYOUT layout = {};
            DX::ThrowIfFailed(addressComputer->GetResourceLayout(&layout));
            for (auto i = 0U; i < layout.Planes; ++i)
            {
                auto& plane = layout.Plane[i];
                switch (plane.Usage)
                {
                case XG_PLANE_USAGE_HTILE:
                    m_metadataAddress += plane.BaseOffsetBytes;
                    found = true;
                    break;
                case XG_PLANE_USAGE_DEPTH:
                case XG_PLANE_USAGE_STENCIL:
                    break;
                default:
                    assert(false);
                }
            }
#ifdef _DEBUG
            assert(found);
#else
            (void)found;
#endif
        }

        void Uninitialize() override
        {
            m_depthBuffer.Reset();
            m_descriptorHeapDsv.Reset();
            m_pipelineState.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            bool isStencil = (1U == m_formatParams.m_bytesPerPixel);

            name << L"Decompress";
            name << (isStencil ? L"(Stencil<--Htile) " : L"(Depth<--Htile) ");
            name << L"from ";
            name << GetCompressionStateName();
            name << L": " << m_formatParams.m_formatName;

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;

#ifdef _GAMING_XBOX_SCARLETT
            auto reads32 = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_EA_RDREQ_DRAM_32B);
            auto writes32 = GetCounterValue(GPUPerfCounters::GL2C_PERF_SEL_EA_WRREQ_DRAM_32B);
            auto bytesRead = 32 * reads32;
            auto bytesWritten = 32 * writes32;
#else
            auto reads32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_ALL_READ_RESPONSE_DATA_BEATS_CH0)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_ALL_READ_RESPONSE_DATA_BEATS_CH1);
            auto bytesRead = 32 * reads32;

            auto writes32 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_STALLABLE_WRITE_CH0)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_STALLABLE_WRITE_CH1)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_NONSTALLABLE_WRITE_CH0)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_32B_NONSTALLABLE_WRITE_CH1);
            auto writes64 = GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_STALLABLE_WRITE_CH0)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_STALLABLE_WRITE_CH1)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_NONSTALLABLE_WRITE_CH0)
                + GetCounterValue(GPUPerfCounters::MC_ARB_PERF_GN_64B_NONSTALLABLE_WRITE_CH1);
            auto bytesWritten = 32 * writes32 + 64 * writes64;
#endif
            auto bytes = bytesRead + bytesWritten;
            auto gbPerSec = (bytes / float(GB)) / timeMs * 1000.0f;

            bool isStencil = (1U == m_formatParams.m_bytesPerPixel);

            report->AddRowData(isStencil ? L"Stencil<--Htile" : L"Depth<--Htile");
            report->AddRowData(GetCompressionStateName());
            report->AddRowData(m_formatParams.m_formatName);
            report->AddRowData(m_width);
            report->AddRowData(m_height);
            report->AddRowData(timeMs);
            report->AddRowData(bytesRead / float(MB));
            report->AddRowData(bytesWritten / float(MB));
            report->AddRowData(gbPerSec);

            DirectX::XMVECTOR color;
            switch (m_formatParams.m_bytesPerPixel)
            {
            case 2:
                color = DirectX::Colors::White;
                break;
            case 4:
                color = DirectX::Colors::Tan;
                break;
            case 1:
            default:
                color = DirectX::Colors::White;
                break;
            }
            report->EndRow(color);
        }

        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            bool isStencil = (1U == m_formatParams.m_bytesPerPixel);

            auto TransitionBarrier = [commandList](ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)->void
                {
                    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after, subresource);
                    commandList->ResourceBarrier(1U, &barrier);
                };

            // Transition to compressible state
            TransitionBarrier(m_depthBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE, isStencil ? 1U : 0U);

            // Fast clear
            commandList->ClearDepthStencilView(m_descriptorDsvCpu, isStencil ? D3D12_CLEAR_FLAG_STENCIL : D3D12_CLEAR_FLAG_DEPTH, g_depthClearValue, g_stencilClearValue, 0, nullptr);

            // Do some rendering
            switch (m_compressionState)
            {
            case COMPRESSION_STATE_CLEAR:
                break;  // do nothing
            case COMPRESSION_STATE_SINGLE_VALUE:
                // Fill the render target with a single depth or stencil (most compressible data besides clear value)
                // fallthrough
            case COMPRESSION_STATE_UNCOMPRESSED:
            {
                // Fullscreen draw which writes to each pixel of depth or stencil
                commandList->SetGraphicsRootSignature(m_rootSignature.Get());
                commandList->SetPipelineState(m_pipelineState.Get());

                commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

                commandList->OMSetRenderTargets(0, nullptr, FALSE, &m_descriptorDsvCpu);
                commandList->OMSetStencilRef(0x0f); // for single-value stencil

                commandList->RSSetViewports(1, &m_viewport);
                commandList->RSSetScissorRects(1, &m_scissorRect);

                commandList->DrawInstanced(1, 1, 0, 0);
            }
                break;
            default:
                assert(false);
            };

            // Clear caches --- happens automatically
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            bool isStencil = (1U == m_formatParams.m_bytesPerPixel);

#ifdef _DEBUG
            // Test metadata for expected values
            auto htile = reinterpret_cast<const uint32_t*>(m_metadataAddress)[0];
            if (isStencil)
            {
                auto smem = (htile & 0x300) >> 8;
                switch (m_compressionState)
                {
                case COMPRESSION_STATE_CLEAR:
                    assert(smem == 0x00);
                    break;
                case COMPRESSION_STATE_SINGLE_VALUE:
                    assert(smem == 0x01);          // single-value compression
                    break;
                case COMPRESSION_STATE_UNCOMPRESSED:
                    assert(smem == 0x03);          // if this fails, the data is somehow compressed
                    break;
                default:
                    assert(false);
                };
            }
            else
            {
                auto zmask = htile & 0x0f;
                switch (m_compressionState)
                {
                case COMPRESSION_STATE_CLEAR:
                    assert(zmask == 0x00);
                    break;
                case COMPRESSION_STATE_SINGLE_VALUE:
                    assert(zmask == 0x01);         // single-plane compression
                    break;
                case COMPRESSION_STATE_UNCOMPRESSED:
                    assert(zmask == 0x0f);         // if this fails, the data is somehow compressed
                    break;
                default:
                    assert(false);
                };
            }
#endif

            auto TransitionBarrier = [commandList](ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)->void
                {
                    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after, subresource);
                    commandList->ResourceBarrier(1U, &barrier);
                };

            // Force a decompress
            TransitionBarrier(m_depthBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON, isStencil ? 1U : 0U);
        }

    protected:
        static const FLOAT                  g_depthClearValue;
        static const UINT8                  g_stencilClearValue;

        ComPtr<ID3D12PipelineState>         m_pipelineState;

        ComPtr<ID3D12Resource>              m_depthBuffer;
        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeapDsv;
        D3D12_CPU_DESCRIPTOR_HANDLE         m_descriptorDsvCpu;
        D3D12_GPU_VIRTUAL_ADDRESS           m_metadataAddress;

        FormatParams                        m_formatParams;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature> DecompressBenchmark::DecompressTest::m_rootSignature;

const std::vector<DecompressBenchmark::DecompressTestColor::FormatParams> DecompressBenchmark::DecompressTestColor::m_allFormatParams =
{
    // We want to cover all different bpp values, because DCC has dependencies on bpp
    // UINT formats are easier to force to be uncompressed
    { L"R8_UNORM",              DXGI_FORMAT_R8_UINT,            1,  1,  },
    { L"R8G8_UNORM",            DXGI_FORMAT_R8G8_UINT,          2,  1,  },
    { L"R8G8B8A8_UNORM",        DXGI_FORMAT_R8G8B8A8_UINT,      4,  1,  },
    { L"R16G16B16A16_FLOAT",    DXGI_FORMAT_R16G16B16A16_UINT,  8,  2,  },
    { L"R32G32B32A32_FLOAT",    DXGI_FORMAT_R32G32B32A32_UINT,  16, 4,  },
};

const DirectX::XMVECTORF32 DecompressBenchmark::DecompressTestColor::g_optimizedClearValue = DirectX::Colors::White;

const std::vector<DecompressBenchmark::DecompressTestDepth::FormatParams> DecompressBenchmark::DecompressTestDepth::m_allFormatParams =
{
    { L"R16_UNORM", DXGI_FORMAT_R16_TYPELESS,       DXGI_FORMAT_D16_UNORM,              DXGI_FORMAT_R16_UNORM,                  2,  },
    { L"R32_FLOAT", DXGI_FORMAT_R32_TYPELESS,       DXGI_FORMAT_D32_FLOAT,              DXGI_FORMAT_R32_FLOAT,                  4,  },
    { L"G8_UINT",   DXGI_FORMAT_R32G8X24_TYPELESS,  DXGI_FORMAT_D32_FLOAT_S8X24_UINT,   DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,    1,  },
};

const FLOAT DecompressBenchmark::DecompressTestDepth::g_depthClearValue = 1.0f;
const UINT8 DecompressBenchmark::DecompressTestDepth::g_stencilClearValue = 0;

DecompressBenchmark benchmark;
