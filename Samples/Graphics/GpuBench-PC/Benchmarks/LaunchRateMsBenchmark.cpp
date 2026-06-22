//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

#ifdef _GAMING_XBOX_SCARLETT
class LaunchRateMsBenchmark final : public Benchmark
{
public:
    LaunchRateMsBenchmark() = default;

    ~LaunchRateMsBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"LaunchRate(MS)";
    }

    void Initialize(ID3D12Device* /*device*/) override
    {
        for (auto waveSize : { 32U, 64U,})
        {
            for (auto wavePerSG : { 1U, 2U, 4U, 8U, })
            {
                if (8U == wavePerSG && 64U == waveSize)
                {
                    continue;
                }

                AddTest(new LaunchRateMsTest(waveSize, wavePerSG));
            }
        }
     
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_WAVES, GpuCounter::SHADER_MASK_GS);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_BUSY_CYCLES, GpuCounter::SHADER_MASK_GS);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_WAVE_CYCLES, GpuCounter::SHADER_MASK_GS);
     
        LaunchRateMsTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
        LaunchRateMsTest::m_rootSignature.Reset();
    }

private:
    class LaunchRateMsTest final : public Test
    {
    public:
        static constexpr uint32_t m_waveAPIperSE = 1024U * 256U;
       
        LaunchRateMsTest(uint32_t waveSize, uint32_t wavePerSG) :
            m_waveSize(waveSize),
            m_wavePerSG(wavePerSG)
        {
            const auto& gpuProperties = GpuProperties::Get();

            m_counts = m_waveAPIperSE * (uint32_t)gpuProperties.m_numSe / m_wavePerSG;            
        }

        void Initialize(ID3D12Device* device) override
        {
            std::wostringstream shaderName;
            shaderName << L"LaunchRateMsWave" << m_waveSize << L"WavePerSG" << m_wavePerSG << L"Ms.cso";            
            auto meshShaderBlob = DX::ReadData(shaderName.str().c_str());

            DX::ThrowIfFailed(device->CreateRootSignature(0,
                meshShaderBlob.data(),
                meshShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_rootSignature);

            D3DX12_MESH_SHADER_PIPELINE_STATE_DESC descPipelineState =            
            {
                m_rootSignature.Get(),                          // ID3D12RootSignature* pRootSignature;
                {},                                             // D3D12_SHADER_BYTECODE AS;
                {
                    meshShaderBlob.data(),
                    meshShaderBlob.size(),
                },                                              // D3D12_SHADER_BYTECODE MS;
                {},                                             // D3D12_SHADER_BYTECODE PS;
                CD3DX12_BLEND_DESC(D3D12_DEFAULT),              // D3D12_BLEND_DESC BlendState;
                UINT_MAX,                                       // UINT SampleMask;
                CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),         // D3D12_RASTERIZER_DESC RasterizerState;
                CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),      // D3D12_DEPTH_STENCIL_DESC DepthStencilState;                
                {},                                             // D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
                0U,                                             // UINT NumRenderTargets;
                {},                                             // DXGI_FORMAT RTVFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
                DXGI_FORMAT_UNKNOWN,                            // DXGI_FORMAT DSVFormat;
                {
                    1U,
                    DXGI_STANDARD_MULTISAMPLE_QUALITY_PATTERN,
                },
                0U,                                             // UINT NodeMask;
                {},                                             // D3D12_CACHED_PIPELINE_STATE CachedPSO;
                D3D12_PIPELINE_STATE_FLAG_NONE,                 // D3D12_PIPELINE_STATE_FLAGS Flags;
            };

            auto meshStateStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(descPipelineState);

            D3D12_PIPELINE_STATE_STREAM_DESC descPipelineStateStream =
            {
                sizeof(meshStateStream),
                & meshStateStream
            };

            ID3D12Device8* device8 = nullptr;
            device->QueryInterface(IID_GRAPHICS_PPV_ARGS(&device8));            
            DX::ThrowIfFailed(device8->CreatePipelineState(&descPipelineStateStream, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
            
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
            name << L"Wave per Subgroup = " << m_wavePerSG << L"";            

            return name.str();
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Wave size", L"", 10, 0);
            report->AddColumn(L"Wave per Subgroup", L"", 10, 0);
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
            auto waveGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_WAVES, GpuCounter::SHADER_MASK_GS);
            auto wavesPerClock = (1000.0f * waveAPI) / (timeMs * clockSpeed);
            auto simdsPerSe = gpuProperties.m_numCuPerSe * gpuProperties.m_numSimdPerCu;
            auto occupancy = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_WAVE_CYCLES, GpuCounter::SHADER_MASK_GS)
                / (float)GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_BUSY_CYCLES, GpuCounter::SHADER_MASK_GS)
                / simdsPerSe;

            report->AddRowData(m_waveSize);
            report->AddRowData(m_wavePerSG);
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
            commandList->SetGraphicsRoot32BitConstant(0, 0, 0); // Set numPrims to 0 so that launch rate will not be PA-bound

            commandList->SetPipelineState(m_pipelineState.Get());
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // Doesn't matter what these are, as all verts will be NaN, and no rtv or dsv is bound
            D3D12_VIEWPORT viewport = {};
            D3D12_RECT scissorRect = {};
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            ID3D12GraphicsCommandList6* commandList6 = nullptr;
            commandList->QueryInterface(IID_GRAPHICS_PPV_ARGS(&commandList6));
            commandList6->DispatchMesh(m_counts, 1, 1);
        }

        // Resources which are shared by all tests
        static ComPtr<ID3D12RootSignature>      m_rootSignature;

    private:
        // Resources which are unique per-test
        ComPtr<ID3D12PipelineState>             m_pipelineState;
        
        uint32_t                                m_waveSize;        
        uint32_t                                m_wavePerSG;
        uint32_t                                m_counts;
    };
};

// Resources which are shared by all tests
ComPtr<ID3D12RootSignature> LaunchRateMsBenchmark::LaunchRateMsTest::m_rootSignature;

LaunchRateMsBenchmark benchmark;
#endif
