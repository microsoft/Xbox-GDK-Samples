//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class LaunchRateCsBenchmark final : public Benchmark
{
public:
    LaunchRateCsBenchmark() = default;

    ~LaunchRateCsBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"LaunchRate(CS)";
    }

    void Initialize(ID3D12Device* /*device*/) override
    {
#ifdef _GAMING_XBOX_SCARLETT
        for (auto waveSize : { 64U, 32U, })
#else
        for (auto waveSize : { 64U, })
#endif
        {
            for (auto wavePerTG : { 1U, 2U, 4U, })
            {
                if (4U == wavePerTG && 64U == waveSize)
                {
                    continue;
                }

                // for (auto numThreadgroupsWalkedPerCu : { 1U, 2U, 3U, 4U, 5U, 6U, 7U, })
                for (auto numThreadgroupsWalkedPerCu : { 1U, 2U, 4U, })
                {
                    for (auto simdWalkAlgorithm : { D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM_DEFAULT,
#ifdef TEST_SIMD_WALK_ALGORITHM
                    D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM_GLOBAL_BALANCED,
                    D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM_GLOBAL_STRICT,
                    D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM_PER_CU,
#endif
                        })
                    {
                        for (auto populatedVgprs : { 1U, 2U, 3U, })
                        {
                            AddTest(new LaunchRateCsTest(waveSize, wavePerTG, numThreadgroupsWalkedPerCu, simdWalkAlgorithm, populatedVgprs));
                        }
                    }
                }
            }
        }

        // These have to be in the same pass, and consecutive in this order
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_WAVES);
#ifdef _GAMING_XBOX_SCARLETT
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_WAVES_32);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_WAVES_64);
#endif
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_BUSY_CYCLES);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_WAVE_CYCLES);

        ComputeTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
    }

private:
    class ComputeTest : public Test
    {
    public:
        static constexpr uint32_t m_threadGroupX = 8;
        static constexpr uint32_t m_threadGroupY = 8;
        static constexpr uint32_t m_threadGroupZ = 1;

        ComputeTest()
        {
        }

        static void PutHeader(Report* report)
        {
            report->AddColumn(L"Wave size", L"", 10, 0);
            report->AddColumn(L"Wave per TG", L"", 12, 0);
            report->AddColumn(L"TG per CU", L"", 10, 0);
#ifdef TEST_SIMD_WALK_ALGORITHM
            report->AddColumn(L"SIMD walk", L"", 10, 0);
#endif
            report->AddColumn(L"#VGPR", L"", 10, 0);
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Wave(API)", L"", 15, 0);
            report->AddColumn(L"Wave(GPU)", L"", 15, 0);
            report->AddColumn(L"Wave/clock", L"", 10, 2);
            report->AddColumn(L"Occupancy", L"", 10, 2);

            report->AddHeader();
        }
    };

    class LaunchRateCsTest final : public ComputeTest
    {
    public:
        LaunchRateCsTest(uint32_t waveSize,
            uint32_t wavePerTG,
            uint32_t numThreadgroupsWalkedPerCu,
            D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM simdWalkAlgorithm,
            uint32_t populatedVgprs
            ) :
            m_waveSize(waveSize),
            m_wavePerTG(wavePerTG),
            m_numThreadgroupsWalkedPerCu(numThreadgroupsWalkedPerCu),
            m_simdWalkAlgorithm(simdWalkAlgorithm),
            m_populatedVgprs(populatedVgprs)
        {
            const auto& gpuProperties = GpuProperties::Get();

            m_dispatchSizeX = 1024U / m_wavePerTG;
            m_dispatchSizeY = 256U * (uint32_t)gpuProperties.m_numSe;
            m_dispatchSizeZ = 1U;
        }

        void Initialize(ID3D12Device* device) override
        {
            std::wostringstream shaderName;
            shaderName << L"LaunchRateCsWave" << m_waveSize << L"Tg" << m_wavePerTG << L"Vgpr" <<m_populatedVgprs << L"Cs.cso";
            auto computeShaderBlob = DX::ReadData(shaderName.str().c_str());
            DX::ThrowIfFailed(device->CreateRootSignature(0,
                computeShaderBlob.data(),
                computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_rootSignature);

            D3D12_COMPUTE_PIPELINE_STATE_DESC descPipelineState =
            {
                m_rootSignature.Get(),                          // ID3D12RootSignature* pRootSignature;
                {
                    computeShaderBlob.data(),
                    computeShaderBlob.size(),
                },                                              // D3D12_SHADER_BYTECODE CS;
                0,                                              // UINT NodeMask;
                { nullptr, 0, },                                // D3D12_CACHED_PIPELINE_STATE CachedPSO;
            };
            D3D12XBOX_COMPUTE_SHADER_LIMITS_DESC descComputeShaderLimits = {};
#ifdef _GAMING_XBOX_SCARLETT
            D3D12XboxInitializeDefaultComputeShaderLimits(&descComputeShaderLimits);
#else
            D3D12XboxInitializeDefaultComputeShaderLimits(FALSE, &descComputeShaderLimits);
#endif
            descComputeShaderLimits.NumThreadgroupsWalkedPerCu = m_numThreadgroupsWalkedPerCu;
            descComputeShaderLimits.SimdWalkAlgorithm = m_simdWalkAlgorithm;
            D3D12XBOX_EXTENDED_COMPUTE_PIPELINE_STATE_DESC descExtendedPipelineState[] =
            {
                {
                    D3D12XBOX_EXTENDED_COMPUTE_PIPELINE_STATE_SHADER_LIMITS,
                    descComputeShaderLimits,
                },
            };
            DX::ThrowIfFailed(device->CreateComputePipelineStateX(&descPipelineState,
                _countof(descExtendedPipelineState),
                descExtendedPipelineState,
                IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_pipelineState);

            D3D12_DESCRIPTOR_HEAP_DESC descHeap =
            {
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,             // D3D12_DESCRIPTOR_HEAP_TYPE Type;
                1U,                                                 // UINT NumDescriptors;
                D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,          // D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                                                    // UINT NodeMask;
            };
            DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeap, IID_GRAPHICS_PPV_ARGS(m_descriptorHeap.ReleaseAndGetAddressOf())));
            SET_NAME_TO_SELF(m_descriptorHeap);

            NullDescriptor(m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), device->GetDescriptorHandleIncrementSize(descHeap.Type));
        }

        void Uninitialize() override
        {
            m_rootSignature.Reset();
            m_pipelineState.Reset();

            m_descriptorHeap.Reset();
        }

        std::wstring GetName() const override
        {
            std::wostringstream name;

            name << L"Wave size = ";
            name << m_waveSize;
            name << L", ";
            name << L"Wave per TG = ";
            name << m_wavePerTG;
            name << L", ";
            name << L"TG per CU = ";
            name << m_numThreadgroupsWalkedPerCu;
#ifdef TEST_SIMD_WALK_ALGORITHM
            name << L", ";
            name << L"SIMD walk = ";
            name << m_simdWalkAlgorithm;
#endif
            name << L", ";
            name << L"#VGPR = ";
            name << m_populatedVgprs;

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            const auto& gpuProperties = GpuProperties::Get();

            auto timeMs = m_elapsedTime;
            auto waveAPI = m_wavePerTG * m_dispatchSizeX * m_dispatchSizeY * m_dispatchSizeZ;
            auto waveGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_WAVES);
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;

            auto wavePerClock = (1000.0f * waveGPU) / (timeMs * clockSpeed);

            auto simdsPerSe = gpuProperties.m_numCuPerSe * gpuProperties.m_numSimdPerCu;
            auto occupancy = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_WAVE_CYCLES)
                / (float)GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_BUSY_CYCLES)
                / simdsPerSe;

            report->AddRowData(m_waveSize);
            report->AddRowData(m_wavePerTG);
            report->AddRowData(m_numThreadgroupsWalkedPerCu);
#ifdef TEST_SIMD_WALK_ALGORITHM
            report->AddRowData(m_simdWalkAlgorithm);
#endif
            report->AddRowData(m_populatedVgprs);
            report->AddRowData(timeMs);
            report->AddRowData(waveAPI);
            report->AddRowData(waveGPU);
            report->AddRowData(wavePerClock);
            report->AddRowData(occupancy);

            report->EndRow();
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

            // Unbind all views, so our CS outputs are dropped.
            commandList->SetComputeRootDescriptorTable(ROOT_PARAMETER_INDEX_UAV, m_descriptorHeap->GetGPUDescriptorHandleForHeapStart());

            commandList->Dispatch(m_dispatchSizeX, m_dispatchSizeY, m_dispatchSizeZ);
        }

    protected:
        // Resources which are unique per-test
        uint32_t                                            m_waveSize;
        uint32_t                                            m_wavePerTG;
        uint32_t                                            m_numThreadgroupsWalkedPerCu;
        D3D12XBOX_SHADER_CS_SIMD_WALK_ALGORITHM             m_simdWalkAlgorithm;
        uint32_t                                            m_populatedVgprs;

        ComPtr<ID3D12RootSignature>                         m_rootSignature;
        ComPtr<ID3D12PipelineState>                         m_pipelineState;

        ComPtr<ID3D12DescriptorHeap>                        m_descriptorHeap;

        uint32_t                                            m_dispatchSizeX;
        uint32_t                                            m_dispatchSizeY;
        uint32_t                                            m_dispatchSizeZ;
    };
};

LaunchRateCsBenchmark benchmark;

