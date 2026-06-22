//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class LaunchRatePsBenchmark final : public Benchmark
{
public:
    LaunchRatePsBenchmark() = default;

    ~LaunchRatePsBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"LaunchRate(PS)";
    }

    void Initialize(ID3D12Device* device) override
    {
#ifdef _GAMING_XBOX_SCARLETT
        for (auto rbPlusEnabled : { true, false, })
            for (auto waveSize : { 64U, 32U, })
#else
        for (auto rbPlusEnabled : { false, })
            for (auto waveSize : { 64U, })
#endif
            {
#ifdef _GAMING_XBOX_SCARLETT
                for (auto populatedVgprs : {2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, })
#else
                for (auto populatedVgprs : {2U, 3U, 4U, 5U, 6U, })
#endif
                {
                    AddTest(new LaunchRatePsTest(rbPlusEnabled, waveSize, populatedVgprs));
                }
            }

        AddCounter(GPUPerfCounters::SQ_PERF_SEL_WAVES, GpuCounter::SHADER_MASK_PS);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_BUSY_CYCLES, GpuCounter::SHADER_MASK_PS);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_WAVE_CYCLES, GpuCounter::SHADER_MASK_PS);

        auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0,
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(LaunchRatePsTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(LaunchRatePsTest::m_rootSignature);

        // We require a render target in order to set viewport dimensions.
        auto descTexOut = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM,
            LaunchRatePsTest::m_standardWidth,
            LaunchRatePsTest::m_standardHeight,
            1U,
            1U,
            1U,
            DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
        );

        auto memoryType = IsDurangoClass() ? MEMORY_TYPE_ESRAM : MEMORY_TYPE_GARLIC;
        auto pageFlag = uint32_t((IsScarlettClass() || IsScorpioClass()) ? MEM_2MB_PAGES : MEM_64K_PAGES);
        auto address = LaunchRatePsTest::m_resourceAllocator.AllocateResourceMemory(device, &descTexOut, memoryType, pageFlag);

        DX::ThrowIfFailed(device->CreatePlacedResourceX(address,
            &descTexOut,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(LaunchRatePsTest::m_texOut.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(LaunchRatePsTest::m_texOut);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapRtv =
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapRtv, IID_GRAPHICS_PPV_ARGS(LaunchRatePsTest::m_descriptorHeapRtv.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(LaunchRatePsTest::m_descriptorHeapRtv);

        LaunchRatePsTest::m_descriptorRtvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(LaunchRatePsTest::m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(),
            0U,
            device->GetDescriptorHandleIncrementSize(descHeapRtv.Type));

        device->CreateRenderTargetView(LaunchRatePsTest::m_texOut.Get(), nullptr, LaunchRatePsTest::m_descriptorRtvCpu);

        D3D12_VIEWPORT viewport =
        {
            0,                                                          // FLOAT TopLeftX;
            0,                                                          // FLOAT TopLeftY;
            FLOAT(LaunchRatePsTest::m_standardWidth),                   // FLOAT Width;
            FLOAT(LaunchRatePsTest::m_standardHeight),                  // FLOAT Height;
            0.0f,                                                       // FLOAT MinDepth;
            1.0f,                                                       // FLOAT MaxDepth;
        };
        LaunchRatePsTest::m_viewport = viewport;

        D3D12_RECT scissorRect =
        {
            0,                                                          // LONG    left;
            0,                                                          // LONG    top;
            LONG(LaunchRatePsTest::m_standardWidth),                    // LONG    right;
            LONG(LaunchRatePsTest::m_standardHeight),                   // LONG    bottom;
        };
        LaunchRatePsTest::m_scissorRect = scissorRect;

        LaunchRatePsTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        LaunchRatePsTest::m_rootSignature.Reset();

        auto memoryType = IsDurangoClass() ? MEMORY_TYPE_ESRAM : MEMORY_TYPE_GARLIC;
        LaunchRatePsTest::m_resourceAllocator.FreeResourceMemory(LaunchRatePsTest::m_texOut->GetGPUVirtualAddress(), memoryType);
        LaunchRatePsTest::m_texOut.Reset();
        LaunchRatePsTest::m_descriptorHeapRtv.Reset();
    }

private:
    class LaunchRatePsTest final : public Test
    {
    public:
        static constexpr uint32_t m_standardWidth = 1024;
        static constexpr uint32_t m_standardHeight = 1024;
        static constexpr uint32_t m_waveAPIperSE = 1024U * 256U;

#ifdef _GAMING_XBOX_SCARLETT
        LaunchRatePsTest(bool rbPlusEnabled, uint32_t waveSize, uint32_t populatedVgprs) :
            m_rbPlusEnabled(rbPlusEnabled),
#else
        LaunchRatePsTest(bool /*rbPlusEnabled*/, uint32_t waveSize, uint32_t populatedVgprs) :
#endif
            m_waveSize(waveSize),
            m_populatedVgprs(populatedVgprs)
        {
            const auto& gpuProperties = GpuProperties::Get();

            m_instances = m_waveAPIperSE * (uint32_t)gpuProperties.m_numSe * m_waveSize / (m_standardWidth * m_standardHeight);
        }

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"NullVs.cso");
            auto geometryShaderBlob = DX::ReadData(L"FullScreenGs.cso");

            std::wostringstream shaderName;
            shaderName << L"LaunchRatePsWave" << m_waveSize << L"Vgpr" << m_populatedVgprs << L"Ps.cso";
            auto pixelShaderBlob = DX::ReadData(shaderName.str().c_str());

#ifdef _GAMING_XBOX_SCARLETT
            D3D12_PIPELINE_STATE_FLAGS flags = m_rbPlusEnabled ? D3D12_PIPELINE_STATE_FLAG_NONE : D3D12XBOX_PIPELINE_STATE_FLAG_DISABLE_RB_PLUS;
#else
            D3D12_PIPELINE_STATE_FLAGS flags = D3D12_PIPELINE_STATE_FLAG_NONE;
#endif

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
                flags                                           // D3D12_PIPELINE_STATE_FLAGS Flags;                
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

            name << L"Wave size = " << m_waveSize << L", ";
            name << L"#VGPR = " << m_populatedVgprs << L", ";
#ifdef _GAMING_XBOX_SCARLETT
            name << L"RB+ = " << (m_rbPlusEnabled ? L"Enabled" : L"Disabled");
#endif
            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Wave size", L"", 10, 0);
            report->AddColumn(L"#VGPR", L"", 10, 0);
#ifdef _GAMING_XBOX_SCARLETT
            report->AddColumn(L"RB+", L"", 10, 0);
#endif
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Wave(API)", L"", 15);
            report->AddColumn(L"Wave(GPU)", L"", 15);
            report->AddColumn(L"Wave/clock", L"", 5, 5);
            report->AddColumn(L"Occupancy", L"", 10, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            const auto& gpuProperties = GpuProperties::Get();
            auto timeMs = m_elapsedTime;

            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;
            auto waveAPI = m_waveAPIperSE * (uint32_t)gpuProperties.m_numSe;
            auto waveGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_WAVES, GpuCounter::SHADER_MASK_PS);
            auto wavesPerClock = (1000.0f * waveAPI) / (timeMs * clockSpeed);
            auto simdsPerSe = gpuProperties.m_numCuPerSe * gpuProperties.m_numSimdPerCu;
            auto occupancy = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_WAVE_CYCLES, GpuCounter::SHADER_MASK_PS)
                / (float)GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_BUSY_CYCLES, GpuCounter::SHADER_MASK_PS)
                / simdsPerSe;

            report->AddRowData(m_waveSize);
            report->AddRowData(m_populatedVgprs);
#ifdef _GAMING_XBOX_SCARLETT
            report->AddRowData(m_rbPlusEnabled ? L"Enabled" : L"Disabled");
#endif
            report->AddRowData(timeMs);
            report->AddRowData(waveAPI);
            report->AddRowData(waveGPU);
            report->AddRowData(wavesPerClock);
            report->AddRowData(occupancy);

            report->EndRow();
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

            commandList->OMSetRenderTargets(1, &m_descriptorRtvCpu, FALSE, nullptr);

            commandList->RSSetViewports(1, &m_viewport);
            commandList->RSSetScissorRects(1, &m_scissorRect);

            commandList->DrawInstanced(1, m_instances, 0, 0);
        }

        // Resources which are shared by all tests
        static GpuBenchAllocator                m_resourceAllocator;

        static ComPtr<ID3D12RootSignature>      m_rootSignature;

        static ComPtr<ID3D12Resource>           m_texOut;
        static ComPtr<ID3D12DescriptorHeap>     m_descriptorHeapRtv;
        static D3D12_CPU_DESCRIPTOR_HANDLE      m_descriptorRtvCpu;

        static D3D12_VIEWPORT                   m_viewport;
        static D3D12_RECT                       m_scissorRect;

    private:
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>             m_pipelineState;

#ifdef _GAMING_XBOX_SCARLETT
        bool                                    m_rbPlusEnabled;
#endif
        uint32_t                                m_waveSize;
        uint32_t                                m_populatedVgprs;
        uint32_t                                m_instances;
    };
};

// Resources which are shared by all tests
GpuBenchAllocator               LaunchRatePsBenchmark::LaunchRatePsTest::m_resourceAllocator;

ComPtr<ID3D12RootSignature>     LaunchRatePsBenchmark::LaunchRatePsTest::m_rootSignature;

ComPtr<ID3D12Resource>          LaunchRatePsBenchmark::LaunchRatePsTest::m_texOut;
ComPtr<ID3D12DescriptorHeap>    LaunchRatePsBenchmark::LaunchRatePsTest::m_descriptorHeapRtv;
D3D12_CPU_DESCRIPTOR_HANDLE     LaunchRatePsBenchmark::LaunchRatePsTest::m_descriptorRtvCpu;

D3D12_VIEWPORT                  LaunchRatePsBenchmark::LaunchRatePsTest::m_viewport;
D3D12_RECT                      LaunchRatePsBenchmark::LaunchRatePsTest::m_scissorRect;

LaunchRatePsBenchmark benchmark;
