//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class PrimitiveBenchmark final : public Benchmark
{
public:
    PrimitiveBenchmark() = default;

    ~PrimitiveBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Primitive";
    }

    void Initialize(ID3D12Device* device) override
    {
        for (const auto& topologyParams : PrimitiveTest::m_allTopologyParams)
        {
            for (auto splitWorkIntoDraws : {false, true,})
            {
                for (auto splitInstanceIntoPrims : {false, true,})
                {
#ifdef _GAMING_XBOX_SCARLETT
                    for (auto drawNgg : { D3D12_PIPELINE_STATE_FLAG_NONE,
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
                        AddTest(new PrimitiveTest(topologyParams, splitWorkIntoDraws, splitInstanceIntoPrims, drawNgg));
                    }
#else
                    for (auto drawBalancing : {D3D12XBOX_PIPELINE_STATE_FLAG_DISABLE_DRAW_BALANCING, D3D12XBOX_PIPELINE_STATE_FLAG_BASIC_DRAW_BALANCING, D3D12_PIPELINE_STATE_FLAG_NONE, })
                    {
                        // Draw balancing is only supported on Scorpio
                        if (IsDurangoClass() && drawBalancing != D3D12_PIPELINE_STATE_FLAG_NONE)
                        {
                            continue;
                        }

                        AddTest(new PrimitiveTest(topologyParams, splitWorkIntoDraws, splitInstanceIntoPrims, drawBalancing));
                    }
#endif
                }
            }
        }

#ifdef _GAMING_XBOX_SCARLETT
        AddCounter(GPUPerfCounters::PERF_PAPC_PA_INPUT_PRIM);
        AddCounter(GPUPerfCounters::GE_PERFCOUNT_SEL_SPI_GSPRIM_VALID);
#else
        AddCounter(GPUPerfCounters::PAPC_PERF_PA_INPUT_PRIM);
#endif

        auto vertexShaderBlob = DX::ReadData(L"PrimitiveVs.cso");
        DX::ThrowIfFailed(device->CreateRootSignature(0, 
            vertexShaderBlob.data(),
            vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(PrimitiveTest::m_rootSignature.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(PrimitiveTest::m_rootSignature);

        CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        typedef UINT16 Index;   // must match DXGI_FORMAT_R16_UINT 
        typedef Index IndexBuffer[PrimitiveTest::maxPrimsPerInstance * PrimitiveTest::maxIndicesPerPrim];
        auto descBuf = CD3DX12_RESOURCE_DESC::Buffer(
            sizeof(IndexBuffer)
        );
        DX::ThrowIfFailed(device->CreateCommittedResource(&defaultHeapProperties,             
            D3D12_HEAP_FLAG_NONE, 
            &descBuf, 
            D3D12_RESOURCE_STATE_INDEX_BUFFER,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(PrimitiveTest::m_indexBuffer.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(PrimitiveTest::m_indexBuffer);

        // Zero-initialize the index buffer
        // This code is only valid for Xbox, due to the buffer not being in an UPLOAD heap
        void* mapData = nullptr;
        PrimitiveTest::m_indexBuffer->Map(0U, nullptr, &mapData);
        IndexBuffer* indexData = reinterpret_cast<IndexBuffer*>(mapData);
        ZeroMemory(indexData, sizeof(*indexData));
        D3D12_RANGE writtenRange = { 0U, sizeof(*indexData), };
        PrimitiveTest::m_indexBuffer->Unmap(0U, &writtenRange);

        D3D12_INDEX_BUFFER_VIEW indexBufferView =
        {
            PrimitiveTest::m_indexBuffer->GetGPUVirtualAddress(),   // D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
            sizeof(IndexBuffer),                                    // UINT SizeInBytes;
            DXGI_FORMAT_R16_UINT /* must match type "Index"*/,      // DXGI_FORMAT Format;
        };
        PrimitiveTest::m_indexBufferView = indexBufferView;

        PrimitiveTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        PrimitiveTest::m_rootSignature.Reset();
    }

private:
    class PrimitiveTest final : public Test
    {
    public:
        static constexpr uint32_t numDraws = 2048;
        static constexpr uint32_t numInstances = 1;
        static constexpr uint32_t numPrimsPerInstance = 2048;

        static constexpr uint32_t maxIndicesPerPrim = 4;
        static constexpr uint32_t maxPrimsPerInstance = 4096;

        struct TopologyParams
        {
            const wchar_t*                              m_topologyName;
            D3D12_PRIMITIVE_TOPOLOGY_TYPE               m_topologyType;
            D3D_PRIMITIVE_TOPOLOGY                      m_topology;
            uint32_t                                    m_indicesPerFirstPrim; // For a strip, indices for the first prim
            uint32_t                                    m_indicesPerNextPrim;  // For a strip, incremental indices for an additional prim
        };
        static const std::vector<TopologyParams> m_allTopologyParams;

        PrimitiveTest(TopologyParams topologyParams, bool splitWorkIntoDraws, bool splitInstanceIntoPrims, D3D12_PIPELINE_STATE_FLAGS drawBalancingOrNgg) :
            m_topologyParams(topologyParams), 
            m_splitWorkIntoDraws(splitWorkIntoDraws),
            m_splitInstanceIntoPrims(splitInstanceIntoPrims),
            m_drawBalancingOrNgg(drawBalancingOrNgg)
        {
            if (m_topologyParams.m_indicesPerFirstPrim > maxIndicesPerPrim || numPrimsPerInstance > maxPrimsPerInstance)
            {
                throw std::exception("Parameters exceed size of index buffer\n");
            }
 
            if (m_splitInstanceIntoPrims)
            {
                // We are drawing one "primitive" per instance, where topology might be a list or a strip

                // For lists default to one prim per "strip"
                m_numPrimsPerStrip = 1;
                m_numIndicesPerStrip = m_topologyParams.m_indicesPerFirstPrim;
                m_numStripsPerInstance = numPrimsPerInstance;

                // For strips, we make an arbitrary choice to build strips which occupy about one wave
                // This might slightly alter the number of primitives, so as to fill out a strip
                bool connectedTopology = m_topologyParams.m_indicesPerNextPrim < m_topologyParams.m_indicesPerFirstPrim;
                if (connectedTopology)
                {
                    constexpr uint32_t threadsPerStrip = 64;

                    m_numPrimsPerStrip = (threadsPerStrip - m_topologyParams.m_indicesPerFirstPrim) / m_topologyParams.m_indicesPerNextPrim + 1;
                    m_numIndicesPerStrip = m_topologyParams.m_indicesPerFirstPrim + (m_numPrimsPerStrip - 1) * m_topologyParams.m_indicesPerNextPrim;
                    m_numStripsPerInstance = (numPrimsPerInstance + m_numPrimsPerStrip - 1) / m_numPrimsPerStrip;
                }
            }
            else
            {
                // We are drawing multiple primitives per instance

                // One list or strip per instance
                m_numPrimsPerStrip = numPrimsPerInstance;
                m_numIndicesPerStrip = m_topologyParams.m_indicesPerFirstPrim + (numPrimsPerInstance - 1) * m_topologyParams.m_indicesPerNextPrim;
                m_numStripsPerInstance = 1;
            }
        }

        std::wstring GetDrawInstancePrimString() const
        {
            std::wostringstream output;

            if (m_splitWorkIntoDraws)
            {
                if (m_splitInstanceIntoPrims)
                {
                    output << numDraws << L'/' << (numInstances * m_numStripsPerInstance) << L'/' << (m_numPrimsPerStrip);
                }
                else
                {
                    output << numDraws << L'/' << (numInstances) << L'/' << (m_numStripsPerInstance * m_numPrimsPerStrip);
                }
            }
            else
            {
                if (m_splitInstanceIntoPrims)
                {
                    output << 1 << L'/' << (numDraws * numInstances * m_numStripsPerInstance) << L'/' << (m_numPrimsPerStrip);
                }
                else
                {
                    output << 1 << L'/' << (numDraws * numInstances) << L'/' << (m_numStripsPerInstance * m_numPrimsPerStrip);
                }
            }

            return output.str();
        }

        const wchar_t* GetDrawBalancingName() const
        {
#ifdef _GAMING_XBOX_SCARLETT
            return (m_drawBalancingOrNgg == D3D12_PIPELINE_STATE_FLAG_NONE ? L"off" : L"on");
#else
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

        void Initialize(ID3D12Device* device) override
        {
            auto vertexShaderBlob = DX::ReadData(L"PrimitiveVs.cso");

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
                    DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN, 
                },                                              // DXGI_SAMPLE_DESC SampleDesc;
                0U,                                             // UINT NodeMask;
                {},                                             // D3D12_CACHED_PIPELINE_STATE CachedPSO;
                flags,                                          // D3D12_PIPELINE_STATE_FLAGS Flags;
            };
            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineState);
        }

        void Uninitialize() override
        {
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Topology = ";
            name << m_topologyParams.m_topologyName;
            name << L", ";
            name << L"Draw/Instance/Prim = ";
            name << GetDrawInstancePrimString();
            name << L", ";
#ifdef _GAMING_XBOX_SCARLETT
            name << L"NGG = ";
#else
            name << L"Balancing = ";
#endif
            name << GetDrawBalancingName();

            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Topology", L"", 10);
            report->AddColumn(L"Draw/Instance/Prim", L"", 10);
#ifdef _GAMING_XBOX_SCARLETT
            report->AddColumn(L"NGG", L"", 4);
#else
            report->AddColumn(L"Balancing", L"", 4);
#endif
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Prims(API)", L"", 10, 0);
            report->AddColumn(L"Prims(GPU)", L"", 10, 0);
            report->AddColumn(L"Stdev", L"", 10, 0);
            report->AddColumn(L"Prims/clock", L"", 5, 2);
            report->AddColumn(L"% of max", L"%", 6, 2);

            report->AddHeader();
        }

        void PutReport(Report* report) const override
        {
            const auto& gpuProperties = GpuProperties::Get();

            auto timeMs = m_elapsedTime;
            auto primsAPI = numDraws * numInstances * m_numStripsPerInstance * m_numPrimsPerStrip;
#ifdef _GAMING_XBOX_SCARLETT
            auto primsPostVsGsGPU = GetCounterValue(GPUPerfCounters::PERF_PAPC_PA_INPUT_PRIM);
            auto primsPostVsGsStd = GetCounterValue(GPUPerfCounters::PERF_PAPC_PA_INPUT_PRIM, GpuCounter::SHADER_MASK_ALL, CounterValueArrayStd);

            // In NGG case these are primitives submitted by Geometry Engine
            auto primsPreVsGsGPU = GetCounterValue(GPUPerfCounters::GE_PERFCOUNT_SEL_SPI_GSPRIM_VALID);
            auto primsPreVsGsStd = GetCounterValue(GPUPerfCounters::GE_PERFCOUNT_SEL_SPI_GSPRIM_VALID, GpuCounter::SHADER_MASK_ALL, CounterValueArrayStd);

            // In NGG case, primitives could be culled within NGG VS, so select the max of Pre-VS/Post-VS primitive to measure maximal throughput
            auto primsGPU = std::max(primsPostVsGsGPU, primsPreVsGsGPU);
            auto primsStd = (primsGPU == primsPostVsGsGPU) ? primsPostVsGsStd : primsPreVsGsStd;
#else
            auto primsGPU = GetCounterValue(GPUPerfCounters::PAPC_PERF_PA_INPUT_PRIM);
            auto primsStd = GetCounterValue(GPUPerfCounters::PAPC_PERF_PA_INPUT_PRIM, GpuCounter::SHADER_MASK_ALL, CounterValueArrayStd);
#endif
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;
            auto primsPerClock = (1000.0f * primsGPU) / (timeMs * clockSpeed);
            auto primsTheoreticalMax = 1.0f * gpuProperties.m_numIa * gpuProperties.m_numVgtPerIa;

            report->AddRowData(m_topologyParams.m_topologyName);
            report->AddRowData(GetDrawInstancePrimString());
            report->AddRowData(GetDrawBalancingName());
            report->AddRowData(timeMs);
            report->AddRowData(primsAPI);
            report->AddRowData(primsGPU);
            report->AddRowData(primsStd);
            report->AddRowData(primsPerClock);
            report->AddRowData(100.0f * primsPerClock / primsTheoreticalMax);

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

            commandList->IASetIndexBuffer(&m_indexBufferView);

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
                        commandList->DrawIndexedInstanced(m_numIndicesPerStrip, numInstances * m_numStripsPerInstance, 0, 0, 0);
                    }
                    else
                    {
                        commandList->DrawIndexedInstanced(m_numStripsPerInstance * m_numIndicesPerStrip, numInstances, 0, 0, 0);
                    }
                }
            }
            else
            {
                if (m_splitInstanceIntoPrims)
                {
                    commandList->DrawIndexedInstanced(m_numIndicesPerStrip, numDraws * numInstances * m_numStripsPerInstance, 0, 0, 0);
                }
                else
                {
                    commandList->DrawIndexedInstanced(m_numStripsPerInstance * m_numIndicesPerStrip, numDraws * numInstances, 0, 0, 0);
                }
            }
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>      m_rootSignature;

        static ComPtr<ID3D12Resource>           m_indexBuffer;
        static D3D12_INDEX_BUFFER_VIEW          m_indexBufferView;

    private:
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>             m_pipelineState;

        TopologyParams                          m_topologyParams;
        bool                                    m_splitWorkIntoDraws;
        bool                                    m_splitInstanceIntoPrims;
        D3D12_PIPELINE_STATE_FLAGS              m_drawBalancingOrNgg;

        // Derived counts, based on connectivity
        uint32_t                                m_numPrimsPerStrip;
        uint32_t                                m_numIndicesPerStrip;
        uint32_t                                m_numStripsPerInstance;
    };
};

ComPtr<ID3D12RootSignature>     PrimitiveBenchmark::PrimitiveTest::m_rootSignature;

ComPtr<ID3D12Resource>          PrimitiveBenchmark::PrimitiveTest::m_indexBuffer;
D3D12_INDEX_BUFFER_VIEW         PrimitiveBenchmark::PrimitiveTest::m_indexBufferView;

const std::vector<PrimitiveBenchmark::PrimitiveTest::TopologyParams> PrimitiveBenchmark::PrimitiveTest::m_allTopologyParams = 
{
    { L"Pointlist", D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,    D3D_PRIMITIVE_TOPOLOGY_POINTLIST,       1,  1,  },
    { L"Linelist",  D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,     D3D_PRIMITIVE_TOPOLOGY_LINELIST,        2,  2,  },
    { L"Linestrip", D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,     D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,       2,  1,  },
    { L"Trilist",   D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,    3,  3,  },
    { L"Tristrip",  D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,   3,  1,  },
    { L"Rectlist",  D3D12XBOX_PRIMITIVE_TOPOLOGY_TYPE_RECT, D3D_PRIMITIVE_TOPOLOGY_RECTLIST,        3,  3,  },
    { L"Quadlist",  D3D12XBOX_PRIMITIVE_TOPOLOGY_TYPE_QUAD, D3D_PRIMITIVE_TOPOLOGY_QUADLIST,        4,  4,  },
    { L"Quadstrip", D3D12XBOX_PRIMITIVE_TOPOLOGY_TYPE_QUAD, D3D_PRIMITIVE_TOPOLOGY_QUADSTRIP,       4,  2,  },
};

PrimitiveBenchmark benchmark;
