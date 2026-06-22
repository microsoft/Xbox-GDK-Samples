//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Benchmark.h"

using Microsoft::WRL::ComPtr;

class ComputeBenchmark final : public Benchmark
{
public:
    ComputeBenchmark() = default;

    ~ComputeBenchmark() override = default;

    const wchar_t* GetName() const override
    {
        return L"Compute";
    }

    void Initialize(ID3D12Device* /*device*/) override
    {
        for (const auto& params : ComputeValuTest::m_allParams)
        {
            AddTest(new ComputeValuTest(params));
        }

        for (const auto& params : ComputeSaluTest::m_allParams)
        {
            AddTest(new ComputeSaluTest(params));
        }

        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_VALU);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INST_CYCLES_VALU);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_VALU_TRANS);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_INSTS_SALU);

        // These have to be in the same pass, and consecutive in this order
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_LEVEL_WAVES);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_ACCUM_PREV);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_BUSY_CYCLES);
        AddCounter(GPUPerfCounters::SQ_PERF_SEL_WAVE_CYCLES);
        
        ComputeTest::PutHeader(&m_report);
    }

    void Uninitialize() override
    {
    }

private:
    class ComputeTest: public Test
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
            report->AddColumn(L"Instruction type", L"", 24);
            report->AddColumn(L"Time", L" ms", 5, 2);
            report->AddColumn(L"Occupancy", L"", 10, 2);
            report->AddColumn(L"Count(API)", L"", 15, 0);
            report->AddColumn(L"Count(GPU)", L"", 15, 0);
            report->AddColumn(L"Trans(GPU)", L"", 13, 0);
            report->AddColumn(L"TFLOPS", L"", 5, 2);
            report->AddColumn(L"ALU/clock", L"", 5, 2);
            report->AddColumn(L"% of max", L"%", 6, 2);

            report->AddHeader();
        }
    };

    class ComputeValuTest final : public ComputeTest
    {
    public:
        static constexpr uint32_t m_threadGroupX = 8;
        static constexpr uint32_t m_threadGroupY = 8;
        static constexpr uint32_t m_threadGroupZ = 1;

        struct Params
        {
            const wchar_t*                              m_instructionTypeName;
            const wchar_t*                              m_shaderName;
            uint32_t                                    m_simdClockIdeal;
            uint32_t                                    m_opGpuPerApi;          // Number of actual math ops, as opposed to VALU instructions
            uint32_t                                    m_valuExpected;
            uint32_t                                    m_valuExtra;
            uint32_t                                    m_threadPerWave;
        };

        static const std::vector<Params> m_allParams;

        ComputeValuTest(const Params& params) :
            m_params(params)
        {
        #ifndef NDEBUG
            const auto& gpuProperties = GpuProperties::Get();
        #endif

            m_sizeX = 16 * 1024;
            m_sizeY = 256;
            m_sizeZ = 1;

            m_sizeX /= m_params.m_simdClockIdeal;
            assert(0ULL == gpuProperties.m_numThreadsPerSimd % m_params.m_simdClockIdeal);
        }

        void Initialize(ID3D12Device* device) override
        {
            auto computeShaderBlob = DX::ReadData(m_params.m_shaderName);
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
            DX::ThrowIfFailed(device->CreateComputePipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
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

            name << L"Instruction type = ";
            name << m_params.m_instructionTypeName;

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            const auto& gpuProperties = GpuProperties::Get();

            auto timeMs = m_elapsedTime;
            auto valuGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_INSTS_VALU) * m_params.m_threadPerWave;
            auto valuTransGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_INSTS_VALU_TRANS) * m_params.m_threadPerWave;
            auto valuAPI = (m_params.m_valuExpected + m_params.m_valuExtra) * m_sizeX * m_sizeY;
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;

            // We use valuAPI here, mainly to support IntDiv, for which valuGPU is expected to be much larger
            // The "% of max" line will show how much slower software emulation is than a hypothetical hw operation would be
            auto valuPerClock = (1000.0f * valuAPI) / (timeMs * clockSpeed);
            auto numThreadsPerSimd = gpuProperties.m_numThreadsPerSimd;
            auto numSe = gpuProperties.m_numSe;
            auto numCuPerSe = gpuProperties.m_numCuPerSe;
            auto numSimdPerCu = gpuProperties.m_numSimdPerCu;
            auto valuPerClockIdeal = (numSimdPerCu * numCuPerSe * numSe * numThreadsPerSimd) / m_params.m_simdClockIdeal;

            // Watch for uint64_t overflow here
            auto tflops = (float(valuAPI) * m_params.m_opGpuPerApi) / (timeMs * 1000000000);

            auto simdsPerSe = gpuProperties.m_numCuPerSe * gpuProperties.m_numSimdPerCu;

            // Two similar means of calculating occupancy
#ifdef OCCUPANCY_BY_LEVEL_WAVES
            auto occupancy = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_ACCUM_PREV)
                / (float)GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_BUSY_CYCLES)
                / simdsPerSe;
#else
            auto occupancy = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_WAVE_CYCLES)
                / (float)GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_BUSY_CYCLES)
                / simdsPerSe;
#endif

#ifdef _GAMING_XBOX_XBOXONE
            occupancy *= 4.0f;      // counters are in units of 4 
#endif

            report->AddRowData(m_params.m_instructionTypeName);
            report->AddRowData(timeMs);
            report->AddRowData(occupancy);
            report->AddRowData(valuAPI);
            report->AddRowData(valuGPU);
            // These counters don't exist on Durango
            if (IsDurangoClass())
            {
                report->AddRowData(L"unavailable");
            }
            else
            {
                report->AddRowData(valuTransGPU);
            }
            report->AddRowData(tflops);
            report->AddRowData(valuPerClock);
            report->AddRowData(100.0f * valuPerClock / valuPerClockIdeal);


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

            commandList->Dispatch(m_sizeX / m_threadGroupX, m_sizeY / m_threadGroupY, m_sizeZ / m_threadGroupZ);
        }

    protected:
        // Resources which are unique per-test
        Params                              m_params;

        ComPtr<ID3D12RootSignature>         m_rootSignature;
        ComPtr<ID3D12PipelineState>         m_pipelineState;

        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeap;

        uint32_t                            m_sizeX;
        uint32_t                            m_sizeY;
        uint32_t                            m_sizeZ;
    };

    class ComputeSaluTest final : public ComputeTest
    {
    public:
        static constexpr uint32_t m_threadGroupX = 8;
        static constexpr uint32_t m_threadGroupY = 8;
        static constexpr uint32_t m_threadGroupZ = 1;

        struct Params
        {
            const wchar_t*                              m_instructionTypeName;
            const wchar_t*                              m_shaderName;
            uint32_t                                    m_simdClockIdeal;
            uint32_t                                    m_saluGpuPerApi;
            uint32_t                                    m_saluExpected;
            uint32_t                                    m_saluExtra;
        };

        static const std::vector<Params> m_allParams;

        ComputeSaluTest(const Params& params) :
            m_params(params)
        {
            m_sizeX = 4096;
            m_sizeY = 4096;
            m_sizeZ = 1;

            m_sizeX /= m_params.m_simdClockIdeal;
        }

        void Initialize(ID3D12Device* device) override
        {
            auto computeShaderBlob = DX::ReadData(m_params.m_shaderName);
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
            DX::ThrowIfFailed(device->CreateComputePipelineState(&descPipelineState, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
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

            name << L"Instruction type = ";
            name << m_params.m_instructionTypeName;

            return name.str();
        }

        void PutReport(Report* report) const override
        {
            const auto& gpuProperties = GpuProperties::Get();

            auto timeMs = m_elapsedTime;
            auto saluGPU = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_INSTS_SALU);

            // Watch for uint32_t overflow here
            auto saluAPI = (m_params.m_saluExpected + m_params.m_saluExtra) * (m_sizeX / m_threadGroupX) * (m_sizeY / m_threadGroupY);
            auto clockSpeed = g_gpuHardwareConfiguration.GpuFrequency;
            auto saluPerClock = (1000.0f * saluAPI) / (timeMs * clockSpeed);   
            auto numSe = gpuProperties.m_numSe;
            auto numCuPerSe = gpuProperties.m_numCuPerSe;
            auto numSimdPerCu = gpuProperties.m_numSimdPerCu;
            auto saluPerClockIdeal = (numSimdPerCu * numCuPerSe * numSe) / m_params.m_simdClockIdeal;

            // Watch for uint64_t overflow here
            auto tflops = (float(saluAPI) * m_params.m_saluGpuPerApi) / (timeMs * 1000000000);   

            auto simdsPerSe = gpuProperties.m_numCuPerSe * gpuProperties.m_numSimdPerCu;

            // Two similar means of calculating occupancy
#ifdef OCCUPANCY_BY_LEVEL_WAVES
            auto occupancy = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_ACCUM_PREV)
                / (float)GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_BUSY_CYCLES)
                / simdsPerSe;
#else
            auto occupancy = GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_WAVE_CYCLES)
                / (float)GetCounterValue(GPUPerfCounters::SQ_PERF_SEL_BUSY_CYCLES)
                / simdsPerSe;
#endif

#ifdef _GAMING_XBOX_XBOXONE
            occupancy *= 4.0f;      // counters are in units of 4 
#endif

            report->AddRowData(m_params.m_instructionTypeName);
            report->AddRowData(timeMs);
            report->AddRowData(occupancy);
            report->AddRowData(saluAPI);
            report->AddRowData(saluGPU);
            report->AddRowData(0);
            report->AddRowData(tflops);
            report->AddRowData(saluPerClock);
            report->AddRowData(100.0f * saluPerClock / saluPerClockIdeal);

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
            commandList->SetComputeRootDescriptorTable(ROOT_PARAMETER_INDEX_SRV, m_descriptorHeap->GetGPUDescriptorHandleForHeapStart());
            commandList->SetComputeRootDescriptorTable(ROOT_PARAMETER_INDEX_UAV, m_descriptorHeap->GetGPUDescriptorHandleForHeapStart());

            commandList->Dispatch(m_sizeX / m_threadGroupX, m_sizeY / m_threadGroupY, m_sizeZ / m_threadGroupZ);
        }

    protected:
        // Resources which are unique per-test
        Params                              m_params;

        ComPtr<ID3D12RootSignature>         m_rootSignature;
        ComPtr<ID3D12PipelineState>         m_pipelineState;

        ComPtr<ID3D12DescriptorHeap>        m_descriptorHeap;

        uint32_t                            m_sizeX;
        uint32_t                            m_sizeY;
        uint32_t                            m_sizeZ;
    };
};

const std::vector<ComputeBenchmark::ComputeValuTest::Params> ComputeBenchmark::ComputeValuTest::m_allParams = 
{
    { L"FloatArithmetic",       L"ComputeFloatArithmeticCs.cso",        1,  2,  512,   0,  64,  },  // 'mad' counts as two ops
    { L"FloatTranscendental",   L"ComputeFloatTranscendentalCs.cso",    4,  1,  512,   0,  64,  },
#ifdef _GAMING_XBOX_SCARLETT
    { L"FloatArithTrans",       L"ComputeFloatArithTransCs.cso",        1,  1,  512,   0,  32,  },  // the "ideal value" of 1 is if transcendentals issue in parallel with arithmetic
#endif
    { L"FloatSin",              L"ComputeFloatSinCs.cso",               4,  1,  512,   0,  64,  },  // the "ideal value" of 4 is if HLSL sin mapped to the single hardware instruction
    { L"DoubleArithmetic",      L"ComputeDoubleArithmeticCs.cso",       16, 1,  512,   3,  64,  },  // shader has three convert ops
#ifdef _GAMING_XBOX_SCARLETT
    { L"DoubleAdd",             L"ComputeDoubleAddCs.cso",              16,  1,  512,  0,  64,  },  // double add is 8 clocks on XboxOne, 16 clocks on Scarlett
#else
    { L"DoubleAdd",             L"ComputeDoubleAddCs.cso",              8,  1,  512,   0,  64,  },
#endif
#if defined(_GAMING_XBOX_SCARLETT) && (_GXDK_VER >= 0x55F007B1)  // October 2021 GDK
    { L"HalfArithmetic",        L"ComputeHalfArithmeticCs.cso",         1,  4,  512,   0,  64,  },  // the 'mad' counts as two ops, and each packed instruction is two ops on float16_t
 #endif
    { L"IntArithmetic",         L"ComputeIntArithmeticCs.cso",          1,  1,  512,   0,  64,  },
    { L"IntMul",                L"ComputeIntMulCs.cso",                 4,  1,  512,   0,  64,  },
    { L"IntDiv",                L"ComputeIntDivCs.cso",                 4,  1,  512,   0,  64,  },  // the "ideal value" of 4 is if there was a hardware div instruction
#if defined(_GAMING_XBOX_SCARLETT) && (_GXDK_VER >= 0x55F007B1)  // October 2021 GDK
    { L"ShortMul",              L"ComputeShortMulCs.cso",               1,  2,  512,   0,  64,  },  // each packed instruction is two ops on uint16_t
 #endif
};

const std::vector<ComputeBenchmark::ComputeSaluTest::Params> ComputeBenchmark::ComputeSaluTest::m_allParams = 
{
    // Scarlett SALU is 2x faster than Xbox One per clock, to compensate for 1/2 wave size
#ifdef _GAMING_XBOX_SCARLETT
    { L"Scalar",                L"ComputeScalarCs.cso",                 2,  1,  512,   3 },   // root signature indirection of 32-bit pointer involves s_mov_b32 of 0  
#else
    { L"Scalar",                L"ComputeScalarCs.cso",                 4,  1,  512,   2 },   // root signature indirection of 32-bit pointer involves s_mov_b32 of 0  
#endif
};

ComputeBenchmark benchmark;

