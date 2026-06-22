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

#ifdef _GAMING_XBOX_SCARLETT
        AddCounter(GPUPerfCounters::GE_PERFCOUNT_SEL_SPI_ESVERT_VALID);
        AddCounter(GPUPerfCounters::GE_PERFCOUNT_SEL_SPI_VSVERT_SEND);
        AddCounter(GPUPerfCounters::GE_PERFCOUNT_SEL_VS_PC_STALL);
#else
        AddCounter(GPUPerfCounters::VGT_PERF_VGT_SPI_VSVERT_SEND);
        AddCounter(GPUPerfCounters::SPI_PERF_VS_PC_STALL);
#endif
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_WAVES);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_EXP);
        
        auto vertexShaderBlob = DX::ReadData(L"Parameter00Vs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0,
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(ParameterTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(ParameterTest::m_rootSignature);

        ParameterTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        ParameterTest::m_rootSignature.Reset();
    }

private:
    class ParameterTest final : public Test
    {
    public:
        static constexpr uint32_t m_numPrims = 1024 * 1024;
        static constexpr uint32_t m_numVertsPerPrim = 1;
        static constexpr uint32_t m_threadsPerWave = 64;
        
        ParameterTest(uint32_t numParams
#if PARAM_STRIPPING_SUPPORTED
            , bool stripUnusedExports
#endif
        )
            : m_numParams(numParams)
#if PARAM_STRIPPING_SUPPORTED
            , m_stripUnusedExports(stripUnusedExports)
#endif
        {
        }

        void Initialize(ID3D12Device* device) override
        {
            std::wostringstream vertexShaderName;
            vertexShaderName << L"Parameter" << std::setw(2) << std::setfill(L'0') << m_numParams
#if PARAM_STRIPPING_SUPPORTED
                << (m_stripUnusedExports ? L"Strip" : L"")
#endif
                << L"Vs.cso";
            auto vertexShaderBlob = DX::ReadData(vertexShaderName.str().c_str());

            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignature.Get(),                          // ID3D12RootSignature* pRootSignature;
                {
                    vertexShaderBlob.data(),
                    vertexShaderBlob.size(),
                },                                              // D3D12_SHADER_BYTECODE VS;
                {},                                             // D3D12_SHADER_BYTECODE PS;
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
                0U,                                             // UINT NumRenderTargets;
                {},                                             // DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
                DXGI_FORMAT_UNKNOWN,                            // DXGI_FORMAT DSVFormat;
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
            const auto& gpuProperties = GpuProperties::Get();

            auto timeMs = m_elapsedTime;
            auto vertsAPI = m_numPrims * m_numVertsPerPrim;
#ifdef _GAMING_XBOX_SCARLETT
            auto vertsGPU = GetCounterValue(GPUPerfCounters::GE_PERFCOUNT_SEL_SPI_VSVERT_SEND);
            auto pcStallClocks = GetCounterValue(GPUPerfCounters::GE_PERFCOUNT_SEL_VS_PC_STALL);
#else
            auto vertsGPU = GetCounterValue(GPUPerfCounters::VGT_PERF_VGT_SPI_VSVERT_SEND);
            auto pcStallClocks = GetCounterValue(GPUPerfCounters::SPI_PERF_VS_PC_STALL, GpuCounter::SHADER_MASK_ALL, CounterValueArrayAvg);
#endif
            auto expsAPI = m_numPrims * m_numVertsPerPrim * m_numParams;
            auto expsGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_INSTS_EXP) * m_threadsPerWave;
            expsGPU -= GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_WAVES) * m_threadsPerWave;    // exclude the Position export
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;
            auto vertsPerClock = (1000.0f * vertsGPU) / (timeMs * clockSpeed);
            auto vertsTheoreticalMax = 1.0f * gpuProperties.m_numIa * gpuProperties.m_numVgtPerIa;
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
            auto stallsPerClock = (1000.0f * pcStallClocks) / (timeMs * clockSpeed);

            report->AddRowData(m_numParams);
#if PARAM_STRIPPING_SUPPORTED
            report->AddRowData(m_stripUnusedExports);
#endif
            report->AddRowData(timeMs);
            report->AddRowData(vertsAPI);
            report->AddRowData(vertsGPU);
            report->AddRowData(vertsPerClock);
            report->AddRowData(100.0f * vertsPerClock / vertsTheoreticalMax);
            report->AddRowData(expsAPI);
            report->AddRowData(expsGPU);
            report->AddRowData(expsPerClock);
            report->AddRowData(100.0f * expsPerClock / expsTheoreticalMax);
            report->AddRowData(100.0f * stallsPerClock);

            report->EndRow(DirectX::Colors::Tan);
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            // Set up for VS-only rendering 
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

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
#else            
            shaderLimits.MaxWavesWithLateAllocParameterCache = D3D12XBOX_SHADER_MAX_WAVES_WITH_LATE_ALLOC_PARAMETER_CACHE_DISABLE;
            commandList->SetGraphicsShaderLimitsX(FALSE, FALSE, FALSE, FALSE, &shaderLimits);
#endif

            commandList->DrawInstanced(m_numPrims * m_numVertsPerPrim, 1, 0, 0);
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>      m_rootSignature;

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

ComPtr<ID3D12RootSignature> ParameterBenchmark::ParameterTest::m_rootSignature;

ParameterBenchmark benchmark;
