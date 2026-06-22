//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

#ifdef _GAMING_XBOX_SCARLETT
class VrsBenchmark final : public Benchmark
{
public:
    VrsBenchmark() = default;

    ~VrsBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"VRS";
    }

    void Initialize(ID3D12Device* device) override
    {
        // Test for all formats, msaa
        for (auto& params : VrsTest::m_allVrsParams)
        {
            AddTest(new VrsTest(params));
        }

        AddCounter(GPUPerfCounters::DB_PERF_SEL_DB_CB_lquad_quads_vrs_rate_1x1);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_DB_CB_lquad_quads_vrs_rate_2x1);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_DB_CB_lquad_quads_vrs_rate_1x2);
        AddCounter(GPUPerfCounters::DB_PERF_SEL_DB_CB_lquad_quads);
        
        AddCounter(GPUPerfCounters::SC_PK_PM_VRS_RATE_X_00_Y_00_QUAD);
        AddCounter(GPUPerfCounters::SC_PK_PM_VRS_RATE_X_01_Y_00_QUAD);
        AddCounter(GPUPerfCounters::SC_PK_PM_VRS_RATE_X_00_Y_01_QUAD);
        AddCounter(GPUPerfCounters::SC_PK_PM_VRS_RATE_X_01_Y_01_QUAD);

        AddCounter(GPUPerfCounters::SQ_PERF_SEL_ITEMS, GpuCounter::SHADER_MASK_PS);

        AddCounter(GPUPerfCounters::CB_PERF_SEL_DRAWN_PIXEL);

        // These errors fire erroneously when RSSetShaderRate is called without some graphics state set up on the command list
        // D3D12 ERROR: [0x65A5C781] ID3D12GraphicsCommandList(Graphics)::RSSetShadingRateImage: The current primitive topology in the Command List is D3D_PRIMITIVE_TOPOLOGY_UNDEFINED. This is invalid since the command list topology must match the Pipeline State topology type which cannot be undefined. [ EXECUTION ERROR #219 ]
        m_scopedErrorFilters.push_back(new ScopedErrorFilter(device, 0x65A5C781));
        // D3D12 ERROR: [0xB7C447A4] ID3D12GraphicsCommandList(Graphics)::RSSetShadingRateImage: All viewports are currently invalid, as they have not been set via RSSetViewports since command list recording started.
        m_scopedErrorFilters.push_back(new ScopedErrorFilter(device, 0xB7C447A4));
        // D3D12 ERROR: [0x67B99DD5] ID3D12GraphicsCommandList(Graphics)::RSSetShadingRateImage: All scissor rects are currently invalid, as they have not been set via RSSetScissorRects since command list recording started.
        m_scopedErrorFilters.push_back(new ScopedErrorFilter(device, 0x67B99DD5));

        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(VrsTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(VrsTest::m_rootSignature);

        CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        // These options are fixed for Xbox
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
        device->CheckFeatureSupport(D3D12_FEATURE::D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6));
        assert(D3D12_VARIABLE_SHADING_RATE_TIER_2 == options6.VariableShadingRateTier);
        assert(8U == options6.ShadingRateImageTileSize);

        auto descTexShadingRate = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8_UINT,
            (VrsTest::m_standardWidth + 7U) / 8U,  
            (VrsTest::m_standardHeight + 7U) / 8U, 
            1U,
            1U,
            1U,
            0U,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12XBOX_RESOURCE_FLAG_DENY_COMPRESSION_DATA);
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
            D3D12_HEAP_FLAG_NONE, 
            &descTexShadingRate, 
            D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(VrsTest::m_texShadingRate.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(VrsTest::m_texShadingRate);

        // Uav
        D3D12_DESCRIPTOR_HEAP_DESC descHeap = 
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,          // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeap, IID_GRAPHICS_PPV_ARGS(VrsTest::m_descriptorHeap.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(VrsTest::m_descriptorHeap);

        auto descriptorIndex = 0;
        VrsTest::m_descriptorUavCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(VrsTest::m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
            descriptorIndex, 
            device->GetDescriptorHandleIncrementSize(descHeap.Type));
        VrsTest::m_descriptorUavGpu = CD3DX12_GPU_DESCRIPTOR_HANDLE(VrsTest::m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
            descriptorIndex, 
            device->GetDescriptorHandleIncrementSize(descHeap.Type));
        ++descriptorIndex;

        assert(UINT(descriptorIndex) <= descHeap.NumDescriptors);

        device->CreateUnorderedAccessView(VrsTest::m_texShadingRate.Get(), nullptr, nullptr, VrsTest::m_descriptorUavCpu);

        // We require a render target in order to set viewport dimensions.
        auto descTexOut = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM,
            VrsTest::m_standardWidth,  
            VrsTest::m_standardHeight, 
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
            IID_GRAPHICS_PPV_ARGS(VrsTest::m_texOut.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(VrsTest::m_texOut);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapRtv = 
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapRtv, IID_GRAPHICS_PPV_ARGS(VrsTest::m_descriptorHeapRtv.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(VrsTest::m_descriptorHeapRtv);

        VrsTest::m_descriptorRtvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(VrsTest::m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(), 
            0U, 
            device->GetDescriptorHandleIncrementSize(descHeapRtv.Type));

        device->CreateRenderTargetView(VrsTest::m_texOut.Get(), nullptr, VrsTest::m_descriptorRtvCpu);

        auto descTexDepth = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D16_UNORM,
            VrsTest::m_standardWidth,  
            VrsTest::m_standardHeight, 
            1U,
            1U,
            1U,
            DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties, 
            D3D12_HEAP_FLAG_NONE, 
            &descTexDepth, 
            D3D12_RESOURCE_STATE_DEPTH_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(VrsTest::m_texDepth.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(VrsTest::m_texDepth);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapDsv = 
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapDsv, IID_GRAPHICS_PPV_ARGS(VrsTest::m_descriptorHeapDsv.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(VrsTest::m_descriptorHeapDsv);

        VrsTest::m_descriptorDsvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(VrsTest::m_descriptorHeapDsv->GetCPUDescriptorHandleForHeapStart(), 
            0U, 
            device->GetDescriptorHandleIncrementSize(descHeapDsv.Type));

        D3D12_TEX2D_DSV tex2DDsv =
        {
            // UINT MipSlice;
        };
        D3D12_DEPTH_STENCIL_VIEW_DESC descDsv =
        {
            DXGI_FORMAT_D16_UNORM,                              // DXGI_FORMAT Format;
            D3D12_DSV_DIMENSION_TEXTURE2D,                      // D3D12_DSV_DIMENSION ViewDimension;
            D3D12_DSV_FLAG_NONE,                                // D3D12_DSV_FLAGS Flags;
        };
        descDsv.Texture2D = tex2DDsv;
        device->CreateDepthStencilView(VrsTest::m_texDepth.Get(), &descDsv, VrsTest::m_descriptorDsvCpu);

        D3D12_VIEWPORT viewport = 
        {
            0,                                                          // FLOAT TopLeftX;
            0,                                                          // FLOAT TopLeftY;
            FLOAT(VrsTest::m_standardWidth),                     // FLOAT Width;
            FLOAT(VrsTest::m_standardHeight),                    // FLOAT Height;
            0.0f,                                                       // FLOAT MinDepth;
            1.0f,                                                       // FLOAT MaxDepth;
        };
        VrsTest::m_viewport = viewport;

        D3D12_RECT scissorRect =
        {
            0,                                                          // LONG    left;
            0,                                                          // LONG    top;
            LONG(VrsTest::m_standardWidth),                      // LONG    right;
            LONG(VrsTest::m_standardHeight),                     // LONG    bottom;
        };
        VrsTest::m_scissorRect = scissorRect;

        VrsTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        VrsTest::m_rootSignature.Reset();

        VrsTest::m_descriptorHeap.Reset();
        VrsTest::m_texShadingRate.Reset();

        VrsTest::m_texOut.Reset();
        VrsTest::m_descriptorHeapRtv.Reset();

        VrsTest::m_texDepth.Reset();
        VrsTest::m_descriptorHeapDsv.Reset();

        for (auto filter : m_scopedErrorFilters)
        {
            delete filter;
        }
        m_scopedErrorFilters.clear();
    }

private:
    std::vector<ScopedErrorFilter*>                     m_scopedErrorFilters;

    class VrsTest final : public Test
    {
    public:
        static constexpr uint32_t m_standardWidth = 256;
        static constexpr uint32_t m_standardHeight = 256;
        static constexpr auto m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
        static constexpr uint32_t m_instances = 32;
        static constexpr uint32_t m_threadPerWave = 64; // Change this to per-test if we ever go to wave32 on Scarlett

        struct VrsParams
        {
            const wchar_t*                              m_globalRateName;
            D3D12_SHADING_RATE                          m_globalRate;
            const wchar_t*                              m_primRateName;
            D3D12_SHADING_RATE                          m_primRate;
            const wchar_t*                              m_imageRateName;
            D3D12_SHADING_RATE                          m_imageRate;
        };
        static const std::vector<VrsParams> m_allVrsParams;

        VrsTest(const VrsParams& params) : m_params(params)
        {
        }

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto pixelShaderBlob = DX::ReadData(L"VrsPs.cso");

            std::wostringstream geometryShaderName;
            geometryShaderName << L"Vrs" << m_params.m_primRateName << L"Gs.cso";
            auto geometryShaderBlob = DX::ReadData(geometryShaderName.str().c_str());

            auto depthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            depthStencilState.DepthEnable = FALSE;

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
        }

        void Uninitialize() override
        {
            m_pipelineState.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Global rate = ";
            name << m_params.m_globalRateName;
            name << L", ";

            name << L"Prim rate = ";
            name << m_params.m_primRateName;
            name << L", ";

            name << L"Image rate = ";
            name << m_params.m_imageRateName;

            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Global", L"", 4);

            report->AddColumn(L"Prim", L"", 4);

            report->AddColumn(L"Image", L"", 4);

            report->AddColumn(L"Time", L" ms", 5, 2);

            report->AddColumn(L"PS threads", L"", 8);

            report->AddColumn(L"pixels", L"", 8);

            report->AddColumn(L"DB 1x1", L"", 8);
            report->AddColumn(L"DB 2x1", L"", 8);
            report->AddColumn(L"DB 1x2", L"", 8);
            report->AddColumn(L"DB 2x2", L"", 8);

            report->AddColumn(L"SC 1x1", L"", 8);
            report->AddColumn(L"SC 2x1", L"", 8);
            report->AddColumn(L"SC 1x2", L"", 8);
            report->AddColumn(L"SC 2x2", L"", 8);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            auto timeMs = m_elapsedTime;

            auto psThreads = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_ITEMS, GpuCounter::SHADER_MASK_PS);

            auto pixels = GetCounterValue(GPUPerfCounters::CB_PERF_SEL_DRAWN_PIXEL);

            auto dbQuads1x1 = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_DB_CB_lquad_quads_vrs_rate_1x1);
            auto dbQuads2x1 = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_DB_CB_lquad_quads_vrs_rate_2x1);
            auto dbQuads1x2 = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_DB_CB_lquad_quads_vrs_rate_1x2);
            auto dbQuads = GetCounterValue(GPUPerfCounters::DB_PERF_SEL_DB_CB_lquad_quads);
            auto dbQuads2x2 = dbQuads - dbQuads1x1 - dbQuads2x1 - dbQuads1x2;
            
            auto scQuads1x1 = GetCounterValue(GPUPerfCounters::SC_PK_PM_VRS_RATE_X_00_Y_00_QUAD);
            auto scQuads2x1 = GetCounterValue(GPUPerfCounters::SC_PK_PM_VRS_RATE_X_01_Y_00_QUAD);
            auto scQuads1x2 = GetCounterValue(GPUPerfCounters::SC_PK_PM_VRS_RATE_X_00_Y_01_QUAD);
            auto scQuads2x2 = GetCounterValue(GPUPerfCounters::SC_PK_PM_VRS_RATE_X_01_Y_01_QUAD);

            report->AddRowData(m_params.m_globalRateName);

            report->AddRowData(m_params.m_primRateName);

            report->AddRowData(m_params.m_imageRateName);

            report->AddRowData(timeMs);

            report->AddRowData(psThreads);

            report->AddRowData(pixels);

            report->AddRowData(dbQuads1x1);
            report->AddRowData(dbQuads2x1);
            report->AddRowData(dbQuads1x2);
            report->AddRowData(dbQuads2x2);

            report->AddRowData(scQuads1x1);
            report->AddRowData(scQuads2x1);
            report->AddRowData(scQuads1x2);
            report->AddRowData(scQuads2x2);

            report->EndRow();
        }

        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            // Initialize and bind shading rate texture
            // Needs to happen in the Start/Stop functions, since it may take non-trivial time
            auto TransitionBarrier = [commandList] (ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)->void
            {
                auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
                commandList->ResourceBarrier(1U, &barrier);
            };

            TransitionBarrier(m_texShadingRate.Get(), 
                D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE, 
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            ID3D12DescriptorHeap* descriptorHeaps[] =
            {
                m_descriptorHeap.Get(), 
            };
            commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

            UINT clearValue[4] = { reinterpret_cast<const UINT&>(m_params.m_imageRate), };
            commandList->ClearUnorderedAccessViewUint(m_descriptorUavGpu, m_descriptorUavCpu, m_texShadingRate.Get(), clearValue, 0U, nullptr);

            TransitionBarrier(m_texShadingRate.Get(), 
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 
                D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

            // More efficient to set a DSV, even if we don't need one here.
            // Otherwise, there will be a copy of the shading rate image into a dummy htile.
            commandList->OMSetRenderTargets(1, &m_descriptorRtvCpu, FALSE, &m_descriptorDsvCpu);

            commandList->RSSetViewports(1, &m_viewport);
            commandList->RSSetScissorRects(1, &m_scissorRect);

            ID3D12GraphicsCommandList5* commandList5 = nullptr;
            commandList->QueryInterface(IID_GRAPHICS_PPV_ARGS(&commandList5));
            D3D12_SHADING_RATE_COMBINER combiners[] = { D3D12_SHADING_RATE_COMBINER_SUM, D3D12_SHADING_RATE_COMBINER_SUM, };
            static_assert(D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT == _countof(combiners), "Wrong number of combiners.");
            commandList5->RSSetShadingRate(m_params.m_globalRate, combiners);
            commandList5->RSSetShadingRateImage(m_texShadingRate.Get());

            commandList->DrawInstanced(1, m_instances, 0, 0);

            // Must restore shading rate, because we use this command list to render the UI
            commandList5->RSSetShadingRate(D3D12_SHADING_RATE_1X1, combiners);
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>      m_rootSignature;

        static ComPtr<ID3D12DescriptorHeap>     m_descriptorHeap;
        static ComPtr<ID3D12Resource>           m_texShadingRate;
        static D3D12_CPU_DESCRIPTOR_HANDLE      m_descriptorUavCpu;
        static D3D12_GPU_DESCRIPTOR_HANDLE      m_descriptorUavGpu;

        static ComPtr<ID3D12Resource>           m_texOut;
        static ComPtr<ID3D12DescriptorHeap>     m_descriptorHeapRtv;
        static D3D12_CPU_DESCRIPTOR_HANDLE      m_descriptorRtvCpu;

        static ComPtr<ID3D12Resource>           m_texDepth;
        static ComPtr<ID3D12DescriptorHeap>     m_descriptorHeapDsv;
        static D3D12_CPU_DESCRIPTOR_HANDLE      m_descriptorDsvCpu;

        static D3D12_VIEWPORT                   m_viewport;
        static D3D12_RECT                       m_scissorRect;

    private:
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>             m_pipelineState;

        VrsParams                               m_params;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature>     VrsBenchmark::VrsTest::m_rootSignature;

ComPtr<ID3D12DescriptorHeap>    VrsBenchmark::VrsTest::m_descriptorHeap;
ComPtr<ID3D12Resource>          VrsBenchmark::VrsTest::m_texShadingRate;
D3D12_CPU_DESCRIPTOR_HANDLE     VrsBenchmark::VrsTest::m_descriptorUavCpu;
D3D12_GPU_DESCRIPTOR_HANDLE     VrsBenchmark::VrsTest::m_descriptorUavGpu;

ComPtr<ID3D12Resource>          VrsBenchmark::VrsTest::m_texOut;
ComPtr<ID3D12DescriptorHeap>    VrsBenchmark::VrsTest::m_descriptorHeapRtv;
D3D12_CPU_DESCRIPTOR_HANDLE     VrsBenchmark::VrsTest::m_descriptorRtvCpu;

ComPtr<ID3D12Resource>          VrsBenchmark::VrsTest::m_texDepth;
ComPtr<ID3D12DescriptorHeap>    VrsBenchmark::VrsTest::m_descriptorHeapDsv;
D3D12_CPU_DESCRIPTOR_HANDLE     VrsBenchmark::VrsTest::m_descriptorDsvCpu;

D3D12_VIEWPORT                  VrsBenchmark::VrsTest::m_viewport;
D3D12_RECT                      VrsBenchmark::VrsTest::m_scissorRect;

const std::vector<VrsBenchmark::VrsTest::VrsParams> VrsBenchmark::VrsTest::m_allVrsParams =
{
    { L"1X1",    D3D12_SHADING_RATE_1X1, L"1X1",    D3D12_SHADING_RATE_1X1,     L"1X1",    D3D12_SHADING_RATE_1X1,     }, 
    { L"2X1",    D3D12_SHADING_RATE_2X1, L"1X1",    D3D12_SHADING_RATE_1X1,     L"1X1",    D3D12_SHADING_RATE_1X1,     }, 
    { L"1X2",    D3D12_SHADING_RATE_1X2, L"1X1",    D3D12_SHADING_RATE_1X1,     L"1X1",    D3D12_SHADING_RATE_1X1,     }, 
    { L"2X2",    D3D12_SHADING_RATE_2X2, L"1X1",    D3D12_SHADING_RATE_1X1,     L"1X1",    D3D12_SHADING_RATE_1X1,     }, 
    { L"1X1",    D3D12_SHADING_RATE_1X1, L"1X1",    D3D12_SHADING_RATE_1X1,     L"1X1",    D3D12_SHADING_RATE_1X1,     }, 
    { L"1X1",    D3D12_SHADING_RATE_1X1, L"2X1",    D3D12_SHADING_RATE_2X1,     L"1X1",    D3D12_SHADING_RATE_1X1,     }, 
    { L"1X1",    D3D12_SHADING_RATE_1X1, L"1X2",    D3D12_SHADING_RATE_1X2,     L"1X1",    D3D12_SHADING_RATE_1X1,     }, 
    { L"1X1",    D3D12_SHADING_RATE_1X1, L"2X2",    D3D12_SHADING_RATE_2X2,     L"1X1",    D3D12_SHADING_RATE_1X1,     }, 
    { L"1X1",    D3D12_SHADING_RATE_1X1, L"1X1",    D3D12_SHADING_RATE_1X1,     L"1X1",    D3D12_SHADING_RATE_1X1,     }, 
    { L"1X1",    D3D12_SHADING_RATE_1X1, L"1X1",    D3D12_SHADING_RATE_1X1,     L"2X1",    D3D12_SHADING_RATE_2X1,     }, 
    { L"1X1",    D3D12_SHADING_RATE_1X1, L"1X1",    D3D12_SHADING_RATE_1X1,     L"1X2",    D3D12_SHADING_RATE_1X2,     }, 
    { L"1X1",    D3D12_SHADING_RATE_1X1, L"1X1",    D3D12_SHADING_RATE_1X1,     L"2X2",    D3D12_SHADING_RATE_2X2,     }, 
};

VrsBenchmark benchmark;
#endif

