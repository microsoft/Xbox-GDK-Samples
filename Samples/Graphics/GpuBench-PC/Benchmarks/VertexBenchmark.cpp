//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class VertexBenchmark final : public Benchmark
{
public:
    VertexBenchmark() = default;

    ~VertexBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Vertex";
    }

    void Initialize(ID3D12Device* device) override
    {
        for (const auto & topologyParams : VertexTest::m_allTopologyParams)
        {
            for (auto splitWorkIntoDraws : {false, true,})
            {
                for (auto splitInstanceIntoPrims : {false, true,})
                {
#if defined(_GAMING_XBOX_SCARLETT)
                    for (auto drawNgg : {D3D12_PIPELINE_STATE_FLAG_NONE,
    #if _GXDK_VER >= 0x55F0110A /** Supported from June 2022 GDK */
                        D3D12XBOX_PIPELINE_STATE_FLAG_VS_NGG
    #endif
                        })
                    {
                        // NGG VS is only supported for TRILIST topology as of June 2022 GDK
                        if (drawNgg != D3D12_PIPELINE_STATE_FLAG_NONE && topologyParams.m_topology != D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
                        {
                            continue;
                        }
                        AddTest(new VertexTest(topologyParams, splitWorkIntoDraws, splitInstanceIntoPrims, drawNgg));
                    }
#elif defined(_GAMING_XBOX_XBOXONE)
                    for (auto drawBalancing : {D3D12XBOX_PIPELINE_STATE_FLAG_DISABLE_DRAW_BALANCING, D3D12XBOX_PIPELINE_STATE_FLAG_BASIC_DRAW_BALANCING, D3D12_PIPELINE_STATE_FLAG_NONE, })
                    {
                        // Draw balancing is only supported on Scorpio
                        if (IsDurangoClass() && drawBalancing != D3D12_PIPELINE_STATE_FLAG_NONE)
                        {
                            continue;
                        }
                        AddTest(new VertexTest(topologyParams, splitWorkIntoDraws, splitInstanceIntoPrims, drawBalancing));
                    }
#else // defined(_GAMING_DESKTOP)
                AddTest(new VertexTest(topologyParams, splitWorkIntoDraws, splitInstanceIntoPrims, D3D12_PIPELINE_STATE_FLAG_NONE));
#endif
                }
            }
        }

#if defined(_GAMING_XBOX_SCARLETT)
        AddCounter(GPUPerfCounters::GE_PERFCOUNT_SEL_SPI_ESVERT_VALID);
        AddCounter(GPUPerfCounters::GE_PERFCOUNT_SEL_SPI_VSVERT_SEND);
#elif defined(_GAMING_XBOX_XBOXONE)
        AddCounter(GPUPerfCounters::VGT_PERF_VGT_SPI_VSVERT_SEND);
#else // defined(_GAMING_DESKTOP)
        const auto & gpuProps = GpuProperties::Get();
        if (gpuProps.IsSupportedNvidiaGpu())
        {
            AddCounter("pda__input_verts.sum");
            AddCounter("pda__input_verts.sum.per_cycle_elapsed");
            AddCounter("pda__input_verts.sum.pct_of_peak_sustained_elapsed");
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            AddCounter("VsGsVerticesIn");
            AddCounter("GPUBusyCycles");
        }
        AddCounter_ElapsedTime();
#endif

        auto vertexShaderBlob = DX::ReadData(L"VertexVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(VertexTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(VertexTest::m_rootSignature);

        VertexTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        VertexTest::m_rootSignature.Reset();
    }

private:
    class VertexTest final : public Test
    {
    public:
        static constexpr uint32_t numDraws = 2048;
        static constexpr uint32_t numInstances = 1;
        static constexpr uint32_t numPrimsPerInstance = 2048;

        struct TopologyParams
        {
            const wchar_t*                              m_topologyName;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE               m_topologyType;
            D3D12_PRIMITIVE_TOPOLOGY                    m_topology;
            uint32_t                                    m_vertsPerPrim;
        };
        static const std::vector<TopologyParams> m_allTopologyParams;

        VertexTest(TopologyParams topologyParams, bool splitWorkIntoDraws, bool splitInstanceIntoPrims, D3D12_PIPELINE_STATE_FLAGS drawBalancingOrNgg) :
            m_topologyParams(topologyParams), 
            m_splitWorkIntoDraws(splitWorkIntoDraws),
            m_splitInstanceIntoPrims(splitInstanceIntoPrims),
            m_drawBalancingOrNgg(drawBalancingOrNgg)
        {
        }

        std::wstring GetDrawInstanceVertString() const
        {
            std::wostringstream output;

            if (m_splitWorkIntoDraws)
            {
                if (m_splitInstanceIntoPrims)
                {
                    output << numDraws << L'/' << (numInstances * numPrimsPerInstance) << L'/' << (m_topologyParams.m_vertsPerPrim);
                }
                else
                {
                    output << numDraws << L'/' << (numInstances) << L'/' << (numPrimsPerInstance * m_topologyParams.m_vertsPerPrim);
                }
            }
            else
            {
                if (m_splitInstanceIntoPrims)
                {
                    output << 1 << L'/' << (numDraws * numInstances * numPrimsPerInstance) << L'/' << (m_topologyParams.m_vertsPerPrim);
                }
                else
                {
                    output << 1 << L'/' << (numDraws * numInstances) << L'/' << (numPrimsPerInstance * m_topologyParams.m_vertsPerPrim);
                }
            }

            return output.str();
        }

#ifdef _GAMING_XBOX
        const wchar_t* GetDrawBalancingName() const
        {
    #ifdef _GAMING_XBOX_SCARLETT
            return (m_drawBalancingOrNgg == D3D12_PIPELINE_STATE_FLAG_NONE ? L"off" : L"on");
    #else //defined(_GAMING_XBOX_XBOXONE)
            if (IsDurangoClass())
            {
                return (L"n/a");
            }
            else
            {
                return ((m_drawBalancingOrNgg & D3D12XBOX_PIPELINE_STATE_FLAG_DISABLE_DRAW_BALANCING) ? L"off" : ((m_drawBalancingOrNgg & D3D12XBOX_PIPELINE_STATE_FLAG_BASIC_DRAW_BALANCING) ? L"bas" : L"adv"));
            }
    #endif
        }
#endif // #ifdef _GAMING_XBOX

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"VertexVs.cso");

            auto flags = m_drawBalancingOrNgg;

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
                m_topologyParams.m_topologyType,                // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
                0U,                                             // UINT NumRenderTargets;
                {},                                             // DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
                DXGI_FORMAT_UNKNOWN,                            // DXGI_FORMAT DSVFormat;
                { 
                    1U,
#ifdef _GAMING_XBOX
                    DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
#else
                    0
#endif
                },                                              // DXGI_SAMPLE_DESC SampleDesc;
                0U,                                             // UINT NodeMask;
                {},                                             // D3D12_CACHED_PIPELINE_STATE CachedPSO;
                flags,                                          // D3D12_PIPELINE_STATE_FLAGS Flags;
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
        }

        void Uninitialize() override
        {
            m_pipelineState.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Topology = ";
            name << m_topologyParams.m_topologyName;
            name << L", ";
            name << L"Draw/Instance/Vert = ";
            name << GetDrawInstanceVertString();
#if defined(_GAMING_XBOX_SCARLETT)
            name << L", ";
            name << L"NGG = ";
            name << GetDrawBalancingName();
#elif defined (_GAMING_XBOX_XBOXONE)
            name << L", ";
            name << L"Balancing = ";
            name << GetDrawBalancingName();
#endif
            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Topology", L"", 10);
            report->AddColumn(L"Draw/Instance/Vert", L"", 10);
#if defined(_GAMING_XBOX_SCARLETT)
            report->AddColumn(L"NGG", L"", 4);
#elif defined(_GAMING_XBOX_XBOXONE)
            report->AddColumn(L"Balancing", L"", 4);
#endif
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Verts(API)", L"", 10, 0);
            report->AddColumn(L"Verts(GPU)", L"", 10, 0);
#if defined(_GAMING_XBOX)
            report->AddColumn(L"Stdev", L"", 10, 0);
#endif
            report->AddColumn(L"Verts/clock", L"", 5, 2);
            report->AddColumn(L"% of max", L"%", 6, 2);
            report->AddColumn(L"Max Verts/clock", L"", 10, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            const auto& gpuProperties = GpuProperties::Get();

            auto timeMs = GetElapsedTimeMs();
            auto vertsAPI = numDraws * numInstances * numPrimsPerInstance * m_topologyParams.m_vertsPerPrim;
#ifdef _GAMING_XBOX
    #ifdef _GAMING_XBOX_SCARLETT
            auto counter = m_drawBalancingOrNgg != D3D12_PIPELINE_STATE_FLAG_NONE
                         ? GPUPerfCounters::GE_PERFCOUNT_SEL_SPI_ESVERT_VALID
                         : GPUPerfCounters::GE_PERFCOUNT_SEL_SPI_VSVERT_SEND;
            auto vertsGPU = GetCounterValue(counter);
            auto vertsStd = GetCounterValue(counter, GpuCounter::SHADER_MASK_ALL, CounterValueArrayStd);
    #else // defined(_GAMING_XBOX_XBOXONE)
            auto vertsGPU = GetCounterValue(GPUPerfCounters::VGT_PERF_VGT_SPI_VSVERT_SEND);
            auto vertsStd = GetCounterValue(GPUPerfCounters::VGT_PERF_VGT_SPI_VSVERT_SEND, GpuCounter::SHADER_MASK_ALL, CounterValueArrayStd);
    #endif
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;
            auto vertsPerClock = (1000.0f * vertsGPU) / (timeMs * clockSpeed);
            auto vertsTheoreticalMax = 1.0f * gpuProperties.m_numIa * gpuProperties.m_numVgtPerIa;
            auto vertsThroughputPct = 100.0f * vertsPerClock / vertsTheoreticalMax;
#else // defined(_GAMING_DESKTOP)
            auto vertsGPU = 0U;
            auto vertsPerClock = 0.0;
            auto vertsThroughputPct = 0.0;
            auto vertsTheoreticalMax = 0U;
            if (gpuProperties.IsSupportedNvidiaGpu())
            {
                vertsGPU = static_cast<uint32_t>(GetCounterValue("pda__input_verts.sum"));
                vertsPerClock = GetCounterValue("pda__input_verts.sum.per_cycle_elapsed");
                vertsThroughputPct = GetCounterValue("pda__input_verts.sum.pct_of_peak_sustained_elapsed");
                vertsTheoreticalMax = static_cast<uint32_t>(vertsPerClock * 100.0f / vertsThroughputPct);
            }
            else if (gpuProperties.IsSupportedAmdGpu())
            {
                auto vertsF64 = GetCounterValue("VsGsVerticesIn");
                auto cyclesF64 = GetCounterValue("GPUBusyCycles");
                vertsGPU = static_cast<uint32_t>(vertsF64);
                vertsPerClock = vertsF64 / cyclesF64;
            }
#endif

            report->AddRowData(m_topologyParams.m_topologyName);
            report->AddRowData(GetDrawInstanceVertString());
#ifdef _GAMING_XBOX
            report->AddRowData(GetDrawBalancingName());
#endif
            report->AddRowData(timeMs);
            report->AddRowData(vertsAPI);
            report->AddRowData(vertsGPU);
#ifdef _GAMING_XBOX
            report->AddRowData(vertsStd);
#endif
            report->AddRowData(vertsPerClock);
            report->AddRowData(vertsThroughputPct);
            report->AddRowData(vertsTheoreticalMax);

            DirectX::XMVECTOR color;
            if (m_splitWorkIntoDraws)
            {
                if (m_splitInstanceIntoPrims)
                {
                    color = DirectX::Colors::Tan;
                }
                else
                {
                    color = DirectX::Colors::White;
                }
            }
            else
            {
                if (m_splitInstanceIntoPrims)
                {
                    color = DirectX::Colors::Wheat;
                }
                else
                {
                    color = DirectX::Colors::Silver;
                }
            }
            report->EndRow(color);
        }

        void Run(ID3D12GraphicsCommandList* commandList) const override
        {
            // Set up for VS-only rendering 
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());
            commandList->SetPipelineState(m_pipelineState.Get());

            commandList->IASetPrimitiveTopology(m_topologyParams.m_topology);

            // Doesn't matter what these are, as all verts will be NaN, and no rtv or dsv is bound
            D3D12_VIEWPORT viewport = {};
            D3D12_RECT scissorRect = {};
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            if (m_splitWorkIntoDraws)
            {
                for (auto draw = 0U; draw < numDraws; ++draw)
                {
                    if (m_splitInstanceIntoPrims)
                    {
                        commandList->DrawInstanced(m_topologyParams.m_vertsPerPrim, numInstances * numPrimsPerInstance, 0, 0);
                    }
                    else
                    {
                        commandList->DrawInstanced(numPrimsPerInstance * m_topologyParams.m_vertsPerPrim, numInstances, 0, 0);
                    }
                }
            }
            else
            {
                if (m_splitInstanceIntoPrims)
                {
                    commandList->DrawInstanced(m_topologyParams.m_vertsPerPrim, numDraws * numInstances * numPrimsPerInstance, 0, 0);
                }
                else
                {
                    commandList->DrawInstanced(numPrimsPerInstance * m_topologyParams.m_vertsPerPrim, numDraws * numInstances, 0, 0);
                }
            }
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>      m_rootSignature;

    private:
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>             m_pipelineState;

        TopologyParams                          m_topologyParams;
        bool                                    m_splitWorkIntoDraws;
        bool                                    m_splitInstanceIntoPrims;
        D3D12_PIPELINE_STATE_FLAGS              m_drawBalancingOrNgg;
    };
};

ComPtr<ID3D12RootSignature> VertexBenchmark::VertexTest::m_rootSignature;

const std::vector<VertexBenchmark::VertexTest::TopologyParams> VertexBenchmark::VertexTest::m_allTopologyParams = 
{
    { L"Pointlist", D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,    D3D_PRIMITIVE_TOPOLOGY_POINTLIST,       1,  },
    { L"Linelist",  D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,     D3D_PRIMITIVE_TOPOLOGY_LINELIST,        2,  },
    { L"Trilist",   D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,    3,  },
#ifdef _GAMING_XBOX
    { L"Rectlist",  D3D12XBOX_PRIMITIVE_TOPOLOGY_TYPE_RECT, D3D_PRIMITIVE_TOPOLOGY_RECTLIST,        3,  },
    { L"Quadlist",  D3D12XBOX_PRIMITIVE_TOPOLOGY_TYPE_QUAD, D3D_PRIMITIVE_TOPOLOGY_QUADLIST,        4,  },
#endif
};

VertexBenchmark benchmark;
