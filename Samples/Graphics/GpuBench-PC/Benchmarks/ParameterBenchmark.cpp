//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

#if _GXDK_VER < 0x633610AF /* GDK Edition 240600 */
    #define PARAM_STRIPPING_SUPPORTED 0
#else
    #ifdef _GAMING_XBOX_SCARLETT
        #define PARAM_STRIPPING_SUPPORTED 1
    #else
        #define PARAM_STRIPPING_SUPPORTED 0
    #endif
#endif
// ENABLE this define to test PC path on Xbox (full PSO ensuring pixel shader runs and uses vertex-to-pixel interpolants to avoid hitting fast path when the driver eliminates unused interpolants)
//#define ENABLE_DESKTOP_FULL_PSO_ON_XBOX 1

using Microsoft::WRL::ComPtr;

class ParameterBenchmark final : public Benchmark
{
public:
    ParameterBenchmark() = default;

    ~ParameterBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Parameter";
    }

    void Initialize(ID3D12Device* device) override
    {
#if PARAM_STRIPPING_SUPPORTED
        for (bool strip : { false, true })
        {
#endif
            for (auto numParams = 0U; numParams < 32; ++numParams)
            {
                AddTest(new ParameterTest(numParams
#if PARAM_STRIPPING_SUPPORTED
                    , strip
#endif
                ));
            }
#if PARAM_STRIPPING_SUPPORTED
        }
#endif

#if defined(_GAMING_XBOX_SCARLETT)
        AddCounter(GPUPerfCounters::GE_PERFCOUNT_SEL_SPI_ESVERT_VALID);
        AddCounter(GPUPerfCounters::GE_PERFCOUNT_SEL_SPI_VSVERT_SEND);
        AddCounter(GPUPerfCounters::GE_PERFCOUNT_SEL_VS_PC_STALL);

        AddCounter(GPUPerfCounters::SQ_PERF_SEL_WAVES);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_EXP);
#elif defined(_GAMING_XBOX_XBOXONE)
        AddCounter(GPUPerfCounters::VGT_PERF_VGT_SPI_VSVERT_SEND);
        AddCounter(GPUPerfCounters::SPI_PERF_VS_PC_STALL);

        AddCounter(GPUPerfCounters::SQ_PERF_SEL_WAVES);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_EXP);
#else // defined(_GAMING_DESKTOP)
        const auto & gpuProps = GpuProperties::Get();

        AddCounter_ElapsedTime();

        if (gpuProps.IsSupportedNvidiaGpu())
        {
            AddCounter("pda__input_verts.sum");
            AddCounter("pda__input_verts.sum.per_cycle_elapsed");
            AddCounter("pda__input_verts.sum.pct_of_peak_sustained_elapsed");

            AddCounter("vpc__output_attrs.sum");
            AddCounter("vpc__output_attrs.sum.per_cycle_elapsed");
            AddCounter("vpc__output_attrs.sum.pct_of_peak_sustained_elapsed");
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            AddCounter("GPUBusyCycles");
            AddCounter("VsGsVerticesIn");
            #define AmdGpu_SqGsPrefix "(SQWGP_GS\\d+_SQ|SQ_GS\\d+)_PERF_SEL_"
            #define AmdGpu_SqVsPrefix "(SQWGP_VS\\d+_SQ|SQ_VS\\d+)_PERF_SEL_"
            AddCounter(AmdGpu_SqGsPrefix "INSTS_TEX_STORE");
            AddCounter(AmdGpu_SqVsPrefix "INSTS_TEX_STORE");
            AddCounter(AmdGpu_SqGsPrefix "INSTS_EXP");
            AddCounter(AmdGpu_SqVsPrefix "INSTS_EXP");

            AddCounter(AmdGpu_SqGsPrefix "WAVES");
            AddCounter(AmdGpu_SqVsPrefix "WAVES");

            AddCounter(AmdGpu_SqGsPrefix "WAVES_32");
            AddCounter(AmdGpu_SqVsPrefix "WAVES_32");

            AddCounter(AmdGpu_SqGsPrefix "ITEMS");
            AddCounter(AmdGpu_SqVsPrefix "ITEMS");
        }
#endif

        auto vertexShaderBlob = DX::ReadData(L"Parameter00Vs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0,
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(ParameterTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(ParameterTest::m_rootSignature);

#if defined(_GAMING_DESKTOP) || defined(ENABLE_DESKTOP_FULL_PSO_ON_XBOX)
        CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        auto descTexColor = CD3DX12_RESOURCE_DESC::Tex2D(ParameterTest::m_colorFormat,
                                                         ParameterTest::m_standardWidth,
                                                         ParameterTest::m_standardHeight,
                                                         1U,
                                                         1U,
                                                         1U,
                                                         DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
                                                         D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

        D3D12_CLEAR_VALUE colorClearValue;
        colorClearValue.Format = ParameterTest::m_colorFormat;
        colorClearValue.Color[0] = ParameterTest::m_colorClearValue[0];
        colorClearValue.Color[1] = ParameterTest::m_colorClearValue[1];
        colorClearValue.Color[2] = ParameterTest::m_colorClearValue[2];
        colorClearValue.Color[3] = ParameterTest::m_colorClearValue[3];
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,
                                                          D3D12_HEAP_FLAG_NONE,
                                                          &descTexColor,
                                                          D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                          &colorClearValue,
                                                          IID_GRAPHICS_PPV_ARGS(ParameterTest::m_texColor.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(ParameterTest::m_texColor);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapRtv =
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
            // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapRtv, IID_GRAPHICS_PPV_ARGS(ParameterTest::m_descriptorHeapRtv.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(ParameterTest::m_descriptorHeapRtv);

        ParameterTest::m_descriptorRtvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(ParameterTest::m_descriptorHeapRtv->GetCPUDescriptorHandleForHeapStart(),
                                                                          0U,
                                                                          device->GetDescriptorHandleIncrementSize(descHeapRtv.Type));

        device->CreateRenderTargetView(ParameterTest::m_texColor.Get(), nullptr, ParameterTest::m_descriptorRtvCpu);


        auto descTexDepth = CD3DX12_RESOURCE_DESC::Tex2D(ParameterTest::m_depthFormat,
                                                         ParameterTest::m_standardWidth,
                                                         ParameterTest::m_standardHeight,
                                                         1U,
                                                         1U,
                                                         1U,
                                                         DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
                                                         D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        D3D12_CLEAR_VALUE depthClearValue;
        depthClearValue.Format = ParameterTest::m_depthFormat;
        depthClearValue.DepthStencil.Depth = ParameterTest::m_depthClearValue;
        depthClearValue.DepthStencil.Stencil = 0;
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,
                                                          D3D12_HEAP_FLAG_NONE,
                                                          &descTexDepth,
                                                          D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                          &depthClearValue,
                                                          IID_GRAPHICS_PPV_ARGS(ParameterTest::m_texDepth.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(ParameterTest::m_texDepth);

        D3D12_DESCRIPTOR_HEAP_DESC descHeapDsv =
        {
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV,                     // D3D12_DESCRIPTOR_HEAP_TYPE Type;
            1U,                                                 // UINT NumDescriptors;
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,                    // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
            // UINT NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapDsv, IID_GRAPHICS_PPV_ARGS(ParameterTest::m_descriptorHeapDsv.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(ParameterTest::m_descriptorHeapDsv);

        ParameterTest::m_descriptorDsvCpu = CD3DX12_CPU_DESCRIPTOR_HANDLE(ParameterTest::m_descriptorHeapDsv->GetCPUDescriptorHandleForHeapStart(),
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
        device->CreateDepthStencilView(ParameterTest::m_texDepth.Get(), &descDsv, ParameterTest::m_descriptorDsvCpu);

        D3D12_VIEWPORT viewport =
        {
            0,                                                  // FLOAT TopLeftX;
            0,                                                  // FLOAT TopLeftY;
            FLOAT(ParameterTest::m_standardWidth),              // FLOAT Width;
            FLOAT(ParameterTest::m_standardHeight),             // FLOAT Height;
            0.0f,                                               // FLOAT MinDepth;
            1.0f,                                               // FLOAT MaxDepth;
        };
        ParameterTest::m_viewport = viewport;

        D3D12_RECT scissorRect =
        {
            0,                                                  // LONG    left;
            0,                                                  // LONG    top;
            LONG(ParameterTest::m_standardWidth),               // LONG    right;
            LONG(ParameterTest::m_standardHeight),              // LONG    bottom;
        };
        ParameterTest::m_scissorRect = scissorRect;
#endif

        ParameterTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        ParameterTest::m_rootSignature.Reset();
#if defined(_GAMING_DESKTOP) || defined(ENABLE_DESKTOP_FULL_PSO_ON_XBOX)
        ParameterTest::m_descriptorHeapDsv.Reset();
        ParameterTest::m_descriptorHeapRtv.Reset();
        ParameterTest::m_texDepth.Reset();
        ParameterTest::m_texColor.Reset();
#endif
    }

private:
    class ParameterTest final : public Test
    {
    public:
        static constexpr uint32_t m_numPrims = 1024 * 1024;
        static constexpr uint32_t m_numVertsPerPrim = 1;
        static constexpr uint32_t m_threadsPerWave = 64;
        
#if defined(_GAMING_DESKTOP) || defined(ENABLE_DESKTOP_FULL_PSO_ON_XBOX)
        static constexpr uint32_t m_standardWidth = 256;
        static constexpr uint32_t m_standardHeight = 256;
        static constexpr DXGI_FORMAT m_depthFormat = DXGI_FORMAT_D16_UNORM;
        static constexpr DXGI_FORMAT m_colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

        static constexpr float m_depthClearValue    = 1.0f;
        static constexpr float m_colorClearValue[4] = { 1.0f, 1.0, 1.0f, 1.0f };
#endif

#if PARAM_STRIPPING_SUPPORTED
        ParameterTest(uint32_t numParams, bool stripUnusedExports) : m_numParams(numParams), m_stripUnusedExports(stripUnusedExports) {}
#else
        ParameterTest(uint32_t numParams) : m_numParams(numParams) {}
#endif

        void Initialize(ID3D12Device* device) override
        {
            std::wostringstream vertexShaderName;
            vertexShaderName << L"Parameter" << std::setw(2) << std::setfill(L'0') << m_numParams
#if PARAM_STRIPPING_SUPPORTED
                << (m_stripUnusedExports ? L"Strip" : L"")
#endif
                << L"Vs.cso";
            auto vertexShaderBlob = DX::ReadData(vertexShaderName.str().c_str());
#if defined(_GAMING_DESKTOP) || defined(ENABLE_DESKTOP_FULL_PSO_ON_XBOX)
            std::wostringstream pixelShaderName;
            pixelShaderName << L"Parameter" << std::setw(2) << std::setfill(L'0') << m_numParams << L"Ps.cso";
            auto pixelShaderBlob = DX::ReadData(pixelShaderName.str().c_str());
#endif

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignature.Get(),                          // ID3D12RootSignature* pRootSignature;
                {
                    vertexShaderBlob.data(),
                    vertexShaderBlob.size(),
                },                                              // D3D12_SHADER_BYTECODE VS;
                {
#if defined(_GAMING_DESKTOP) || defined(ENABLE_DESKTOP_FULL_PSO_ON_XBOX)
                    pixelShaderBlob.data(),
                    pixelShaderBlob.size(),
#endif
                },                                              // D3D12_SHADER_BYTECODE PS;
                {},                                             // D3D12_SHADER_BYTECODE DS;
                {},                                             // D3D12_SHADER_BYTECODE HS;
                {},                                             // D3D12_SHADER_BYTECODE GS;
                {},                                             // D3D12_STREAM_OUTPUT_DESC StreamOutput;
                CD3DX12_BLEND_DESC(D3D12_DEFAULT),              // D3D12_BLEND_DESC BlendState;
                UINT_MAX,                                       // UINT SampleMask;
                CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),         // D3D12_RASTERIZER_DESC RasterizerState;
                CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),      // D3D12_DEPTH_STENCIL_DESC DepthStencilState;
                {},                                             // D3D12_INPUT_LAYOUT_DESC InputLayout;
                D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,    // D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue;
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,            // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
#if defined(_GAMING_DESKTOP) || defined(ENABLE_DESKTOP_FULL_PSO_ON_XBOX)
                1U,                                             // UINT NumRenderTargets;
                {},                                             // DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
                m_depthFormat,                                  // DXGI_FORMAT DSVFormat;
#else
                0U,                                             // UINT NumRenderTargets;
                {},                                             // DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
                DXGI_FORMAT_UNKNOWN,                            // DXGI_FORMAT DSVFormat;
#endif
                {
                    1U,
                    DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
                },                                              // DXGI_SAMPLE_DESC SampleDesc;
                0U,                                             // UINT NodeMask;
                {},                                             // D3D12_CACHED_PIPELINE_STATE CachedPSO;
#if PARAM_STRIPPING_SUPPORTED
                m_stripUnusedExports ? D3D12XBOX_PIPELINE_STATE_FLAG_DELETE_UNUSED_PARAMS : D3D12_PIPELINE_STATE_FLAG_NONE,
#else
                D3D12_PIPELINE_STATE_FLAG_NONE,                 // D3D12_PIPELINE_STATE_FLAGS Flags;
#endif
            };
#if defined(_GAMING_DESKTOP) || defined(ENABLE_DESKTOP_FULL_PSO_ON_XBOX)
            descPipelineState.RTVFormats[0] = ParameterTest::m_colorFormat;
#endif
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

            name << L"Params = ";
            name << m_numParams;
#if PARAM_STRIPPING_SUPPORTED
            name << L", Strip Unused Exports = ";
            name << (m_stripUnusedExports ? L"true" : L"false");
#endif

            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Params", L"", 2);
#if PARAM_STRIPPING_SUPPORTED
            report->AddColumn(L"Export Strip", L"", 2);
#endif
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Verts(API)", L"", 10, 0);
            report->AddColumn(L"Verts(GPU)", L"", 10, 0);
            report->AddColumn(L"Verts/clock", L"", 5, 2);
            report->AddColumn(L"% of max", L"%", 6, 2);
            report->AddColumn(L"Exps(API)", L"", 10, 0);
            report->AddColumn(L"Exps(GPU)", L"", 10, 0);
            report->AddColumn(L"Exps/clock", L"", 5, 2);
            report->AddColumn(L"% of max", L"%", 6, 2);
            report->AddColumn(L"PC stall", L"%", 6, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            const auto & gpuProperties = GpuProperties::Get();

            auto timeMs = GetElapsedTimeMs();
            auto vertsAPI = m_numPrims * m_numVertsPerPrim;
#if defined(_GAMING_XBOX_SCARLETT)
            auto vertsGPU = GetCounterValue(GPUPerfCounters::GE_PERFCOUNT_SEL_SPI_VSVERT_SEND);
            auto pcStallClocks = GetCounterValue(GPUPerfCounters::GE_PERFCOUNT_SEL_VS_PC_STALL);
#elif defined(_GAMING_XBOX_XBOXONE)
            auto vertsGPU = GetCounterValue(GPUPerfCounters::VGT_PERF_VGT_SPI_VSVERT_SEND);
            auto pcStallClocks = GetCounterValue(GPUPerfCounters::SPI_PERF_VS_PC_STALL, GpuCounter::SHADER_MASK_ALL, CounterValueArrayAvg);
#else // #ifdef _GAMING_DESKTOP
            auto vertsGPUF64 = 0.0;
            if (gpuProperties.IsSupportedNvidiaGpu())
            {
                vertsGPUF64 = GetCounterValue("pda__input_verts.sum");
            }
            else if (gpuProperties.IsSupportedAmdGpu())
            {
                vertsGPUF64 = GetCounterValue("VsGsVerticesIn");
            }
            auto vertsGPU = static_cast<uint32_t>(vertsGPUF64);
#endif
            auto expsAPI = m_numPrims * m_numVertsPerPrim * m_numParams;
#ifdef _GAMING_XBOX
            auto expsGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_INSTS_EXP) * m_threadsPerWave;
            expsGPU -= GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_WAVES) * m_threadsPerWave;    // exclude the Position export
#else // #ifdef _GAMING_DESKTOP
            auto expsGPUF64 = 0.0;
            if (gpuProperties.IsSupportedNvidiaGpu())
            {
                // It appears that vpc__output_attr counts in dwords, not dword4 and position export adds 5 dwords
                expsGPUF64 = GetCounterValue("vpc__output_attrs.sum") - 5.0 * vertsGPUF64;
                // Account for counting in dwords instead of dword4.
                expsGPUF64 /= 4.0;
            }
            else if (gpuProperties.IsSupportedAmdGpu())
            {

                // some generation use "exp" to export to SX, other store to memory
                auto gsExps = GetCounterValue(AmdGpu_SqGsPrefix "INSTS_TEX_STORE")
                            + GetCounterValue(AmdGpu_SqGsPrefix "INSTS_EXP");

                auto gsThreadCount = GetCounterValue(AmdGpu_SqGsPrefix"ITEMS");
                auto gsWaveCount = GetCounterValue(AmdGpu_SqGsPrefix "WAVES")
                                 + GetCounterValue(AmdGpu_SqGsPrefix "WAVES_32");
                gsExps *= gsThreadCount / (gsWaveCount > 0.0 ? gsWaveCount : 1.0);

                auto vsExps = GetCounterValue(AmdGpu_SqVsPrefix "INSTS_TEX_STORE")
                            + GetCounterValue(AmdGpu_SqVsPrefix "INSTS_EXP");

                auto vsThreadCount = GetCounterValue(AmdGpu_SqVsPrefix"ITEMS");
                auto vsWaveCount = GetCounterValue(AmdGpu_SqVsPrefix "WAVES")
                                 + GetCounterValue(AmdGpu_SqVsPrefix "WAVES_32");
                vsExps *= vsThreadCount / (vsWaveCount > 0.0 ? vsWaveCount : 1.0);

                expsGPUF64 = vsExps + gsExps;
            }
            auto expsGPU = static_cast<uint32_t>(expsGPUF64);
#endif

#ifdef _GAMING_XBOX
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;
            auto vertsPerClock = (1000.0f * vertsGPU) / (timeMs * clockSpeed);
            auto vertsTheoreticalMax = 1.0f * gpuProperties.m_numIa * gpuProperties.m_numVgtPerIa;
            auto vertsThroughput = 100.0f * vertsPerClock / vertsTheoreticalMax;
#else // #ifdef _GAMING_DESKTOP
            auto vertsPerClock = 0.0;
            auto vertsThroughput = 0.0;
            auto cyclesF64 = 0.0;
            if (gpuProperties.IsSupportedNvidiaGpu())
            {
                vertsPerClock = GetCounterValue("pda__input_verts.sum.per_cycle_elapsed");
                vertsThroughput = GetCounterValue("pda__input_verts.sum.pct_of_peak_sustained_elapsed");
            }
            else if (gpuProperties.IsSupportedAmdGpu())
            {
                cyclesF64 = GetCounterValue("GPUBusyCycles");
                vertsPerClock = vertsGPUF64 / cyclesF64;
            }
#endif

#ifdef _GAMING_XBOX
            auto expsPerClock = (1000.0f * expsGPU) / (timeMs * clockSpeed);
            auto expsTheoreticalMax = 0.0f;
            if (IsScarlettClass())
            {
                expsTheoreticalMax = 8.0f * gpuProperties.m_numSe; // 32 dwords per SX per 2 clocks
            }
            else if (IsScorpioClass())
            {
                expsTheoreticalMax = 4.0f * gpuProperties.m_numSe;  // 16 dwords per SE per clock seems solid (?)
            }
            else
            {
                expsTheoreticalMax = 8.0f * gpuProperties.m_numSe;  // Not seeing a clear outcome (?)
            }
            auto expsThroughput = 100.0f * expsPerClock / expsTheoreticalMax;
            auto stallsPerClock = (1000.0f * pcStallClocks) / (timeMs * clockSpeed);
#else // defined(_GAMING_DESKTOP)
            auto expsPerClock = 0.0;
            auto expsThroughput = 0.0;
            auto stallsPerClock = 0;
            if (gpuProperties.IsSupportedNvidiaGpu())
            {
                expsPerClock = GetCounterValue("vpc__output_attrs.sum.per_cycle_elapsed");
                expsThroughput = GetCounterValue("vpc__output_attrs.sum.pct_of_peak_sustained_elapsed");
            }
            else if (gpuProperties.IsSupportedAmdGpu())
            {
                expsPerClock = expsGPUF64 / cyclesF64;
            }
#endif

            report->AddRowData(m_numParams);
#if PARAM_STRIPPING_SUPPORTED
            report->AddRowData(m_stripUnusedExports);
#endif
            report->AddRowData(timeMs);
            report->AddRowData(vertsAPI);
            report->AddRowData(vertsGPU);
            report->AddRowData(vertsPerClock);
            report->AddRowData(vertsThroughput);
            report->AddRowData(expsAPI);
            report->AddRowData(expsGPU);
            report->AddRowData(expsPerClock);
            report->AddRowData(expsThroughput);
            report->AddRowData(100.0f * stallsPerClock);

            report->EndRow(DirectX::Colors::Tan);
        }

#if defined(_GAMING_DESKTOP) || defined(ENABLE_DESKTOP_FULL_PSO_ON_XBOX)
        void Start(ID3D12GraphicsCommandList* commandList) const override
        {
            commandList->ClearRenderTargetView(m_descriptorRtvCpu, m_colorClearValue, 0, nullptr);
            commandList->ClearDepthStencilView(m_descriptorDsvCpu, D3D12_CLEAR_FLAG_DEPTH, m_depthClearValue, 0, 0, nullptr);
        }
#endif

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            // Set up for VS-only rendering 
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

#if defined(_GAMING_XBOX) && !defined(ENABLE_DESKTOP_FULL_PSO_ON_XBOX)
            // Doesn't matter what these are, as all verts will be NaN, and no rtv or dsv is bound
            D3D12_VIEWPORT viewport = {};
            D3D12_RECT scissorRect = {};
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            // Disabling late alloc seems to help a bit for large param counts on Xbox One, 
            // and hurt a bit for medium param counts on Scarlett.
            D3D12XBOX_GRAPHICS_SHADER_LIMITS_DESC shaderLimits = {};
            D3D12XboxInitializeDefaultGraphicsShaderLimits(&shaderLimits);

#ifdef _GAMING_XBOX_SCARLETT
            shaderLimits.MaxLateAllocParameterCacheLines = D3D12XBOX_SHADER_MAX_LATE_ALLOC_PARAMETER_CACHE_LINES_DISABLE;
            //commandList->SetGraphicsShaderLimitsX(FALSE, FALSE, FALSE, FALSE, FALSE, &shaderLimits);
#else // defined(_GAMING_XBOX_XBOXONE)
            shaderLimits.MaxWavesWithLateAllocParameterCache = D3D12XBOX_SHADER_MAX_WAVES_WITH_LATE_ALLOC_PARAMETER_CACHE_DISABLE;
            commandList->SetGraphicsShaderLimitsX(FALSE, FALSE, FALSE, FALSE, &shaderLimits);
#endif

#else
            commandList->RSSetViewports(1, &m_viewport);
            commandList->RSSetScissorRects(1, &m_scissorRect);
            commandList->OMSetRenderTargets(1, &m_descriptorRtvCpu, FALSE, &m_descriptorDsvCpu);
#endif

            commandList->DrawInstanced(m_numPrims * m_numVertsPerPrim, 1, 0, 0);
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>      m_rootSignature;

#if defined(_GAMING_DESKTOP) || defined(ENABLE_DESKTOP_FULL_PSO_ON_XBOX)
        static ComPtr<ID3D12DescriptorHeap>     m_descriptorHeapDsv;
        static ComPtr<ID3D12DescriptorHeap>     m_descriptorHeapRtv;

        static ComPtr<ID3D12Resource>           m_texDepth;
        static ComPtr<ID3D12Resource>           m_texColor;

        static D3D12_CPU_DESCRIPTOR_HANDLE      m_descriptorDsvCpu;
        static D3D12_CPU_DESCRIPTOR_HANDLE      m_descriptorRtvCpu;

        static D3D12_VIEWPORT                   m_viewport;
        static D3D12_RECT                       m_scissorRect;
#endif

    private:
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>             m_pipelineState;

        // Test params
        uint32_t                                m_numParams;
#if PARAM_STRIPPING_SUPPORTED
        bool                                    m_stripUnusedExports;
#endif
    };
};

ComPtr<ID3D12RootSignature>     ParameterBenchmark::ParameterTest::m_rootSignature;
#if defined(_GAMING_DESKTOP) || defined(ENABLE_DESKTOP_FULL_PSO_ON_XBOX)
// On Desktop we use full PSO with Pixel shader and color/depth exports
ComPtr<ID3D12DescriptorHeap>    ParameterBenchmark::ParameterTest::m_descriptorHeapDsv;
ComPtr<ID3D12DescriptorHeap>    ParameterBenchmark::ParameterTest::m_descriptorHeapRtv;
ComPtr<ID3D12Resource>          ParameterBenchmark::ParameterTest::m_texDepth;
ComPtr<ID3D12Resource>          ParameterBenchmark::ParameterTest::m_texColor;
D3D12_CPU_DESCRIPTOR_HANDLE     ParameterBenchmark::ParameterTest::m_descriptorDsvCpu;
D3D12_CPU_DESCRIPTOR_HANDLE     ParameterBenchmark::ParameterTest::m_descriptorRtvCpu;
D3D12_VIEWPORT                  ParameterBenchmark::ParameterTest::m_viewport;
D3D12_RECT                      ParameterBenchmark::ParameterTest::m_scissorRect;
#endif

ParameterBenchmark benchmark;
