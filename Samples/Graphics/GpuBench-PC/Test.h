//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef _GAMING_XBOX
#include "PerformanceTimersXbox.h"
#else
#include "PerformanceTimers.h"
#endif
#include "Util.h"

class Test
{
public:
    Test() :
        m_elapsedTime(0.0)
    {
    }
    virtual ~Test() = default;

    void InitializeCommon(ID3D12Device* device)
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandAllocator);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
        SET_NAME_TO_SELF(m_commandList);
    }

    void UninitializeCommon()
    {
        m_commandAllocator = nullptr;
        m_commandList = nullptr;
    }

    void PrintRunningMessage() const
    {
        std::wostringstream message;
        message << L"\tRunning test \"";
        message << GetName();
        message << L"\"";
        message << std::endl;
        OutputDebugString(message.str().c_str());
    }

    void MeasureTime(ID3D12Device* device, ID3D12CommandQueue* commandQueue) 
    {
        PrintRunningMessage();

        // Measure GPU time (probably no need to do this in a separate pass)
        if (!m_gpuTime)
        {
            m_gpuTime = new DX::GPUTimer(device, commandQueue);
        }

        Start(m_commandList.Get());

        Execute(commandQueue);
        FlushAndHalt(device, commandQueue);
        Reset();

        m_gpuTime->BeginFrame(m_commandList.Get());
        m_gpuTime->Start(m_commandList.Get());

        Run(m_commandList.Get());

        m_gpuTime->Stop(m_commandList.Get());
        m_gpuTime->EndFrame(m_commandList.Get());

        Execute(commandQueue);
        FlushAndHalt(device, commandQueue);
        Reset();

        Stop(m_commandList.Get());

        // Obtain the most recent timings, not those from several frames ago
        m_gpuTime->Flush(m_commandList.Get());
        m_elapsedTime = m_gpuTime->GetElapsedMS();
    }

    void MeasureCounters(ID3D12Device* device, ID3D12CommandQueue* commandQueue, GpuCounterSet* gpuCounterSet) 
    {
        PrintRunningMessage();

        // Measure GPU counters
        gpuCounterSet->StartCollect(device, commandQueue);

        while (!gpuCounterSet->Done())
        {
            Start(m_commandList.Get());

            Execute(commandQueue);
            FlushAndHalt(device, commandQueue);
            Reset();

            gpuCounterSet->StartPass(m_commandList.Get());

            Run(m_commandList.Get());

            gpuCounterSet->EndPass(m_commandList.Get());

            gpuCounterSet->StartExecutePass(commandQueue);
            Execute(commandQueue);
            gpuCounterSet->EndExecutePass(commandQueue);
            FlushAndHalt(device, commandQueue);
            Reset();

            Stop(m_commandList.Get());
        }

        gpuCounterSet->RetrieveCounters(m_gpuCounterTable, commandQueue);

        if (m_dumpCounters)
        {
            gpuCounterSet->DumpCounters();
        }

        gpuCounterSet->EndCollect(device, commandQueue);
    }

    void Capture(ID3D12Device* device, ID3D12CommandQueue* commandQueue) 
    {
        PrintRunningMessage();

        Start(m_commandList.Get());

        Execute(commandQueue);
        FlushAndHalt(device, commandQueue);
        Reset();

        // Event should omit the first Execute in order to exclude the cost of Start
        PIXBeginEvent(commandQueue, PIX_COLOR_DEFAULT, GetName().c_str());

        Run(m_commandList.Get());

        Execute(commandQueue);

        // Event should include the second Execute in order to contain the Test commandList
        PIXEndEvent(commandQueue);

        FlushAndHalt(device, commandQueue);
        Reset();

        Stop(m_commandList.Get());
    }

    void Execute(ID3D12CommandQueue* commandQueue) const
    {
        // Submit everything recorded up to now
        DX::ThrowIfFailed(m_commandList->Close());
        commandQueue->ExecuteCommandLists(1U, CommandListCast(m_commandList.GetAddressOf()));
    }

    void Reset() const
    {
        DX::ThrowIfFailed(m_commandAllocator->Reset());
        DX::ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
    }

    void FlushAndHalt(ID3D12Device* device, ID3D12CommandQueue* commandQueue) const
    {
#ifdef _GAMING_XBOX
        // Flush caches, and block on the CPU until completion
        GpuCacheFlushAndFullBarrier(device, commandQueue);
#endif
        GpuFullStop(device, commandQueue);
    }

    virtual void PutReport(Report* report) const = 0;

    virtual std::wstring GetName() const = 0;
    virtual void Initialize(ID3D12Device* device) = 0;
    virtual void Start(ID3D12GraphicsCommandList*) const {};        // override of this function is optional
    virtual void Run(ID3D12GraphicsCommandList* commandList) const = 0;
    virtual void Stop(ID3D12GraphicsCommandList*) const {};         // override of this function is optional
    virtual void Uninitialize() = 0;

#if defined(_GAMING_XBOX)
    template<typename t_counterType>
    CounterValue GetCounterValue(t_counterType id, GpuCounter::ShaderMask shaderMask = GpuCounter::SHADER_MASK_ALL, CounterAggregator summarizer = CounterValueArrayAdd) const
#else // defined(_GAMING_DESKTOP)
    CounterValue GetCounterValue(const char *counter) const
#endif
    {
#if defined(_GAMING_XBOX)
        auto counter = GpuCounter(id, shaderMask);
#endif
        try
        {
#if defined(_GAMING_XBOX)
            return summarizer(m_gpuCounterTable.at(counter));
#else // defined(_GAMING_DESKTOP)
            auto result = m_gpuCounterTable.find(counter);
            if (result != m_gpuCounterTable.end())
            {
                return result->second;
            }
            return 0.0;
#endif
        }
        catch (std::out_of_range)
        {
            std::wostringstream message;
            message << L"Tried to retrieve ";
#if defined(_GAMING_XBOX)
            message << counter.GetTypeName();
            message << L" counter ";
            message << id;
            message << L" shaderMask ";
            message << L"0x" << std::setw(2) << std::setfill(L'0') << std::hex << shaderMask;
#else // defined(_GAMING_DESKTOP)
            message << counter;
            message << L" counter ";
#endif
            message << ", which wasn't added!";
            message << L"\"";
            message << std::endl;
            OutputDebugString(message.str().c_str());
            assert(false);
            return 0U;
        }
    }

    double GetElapsedTimeMs() const
    {
        auto elapsedTimeMs = 0.0;
#ifdef _GAMING_DESKTOP
        const auto& gpuProps = GpuProperties::Get();
        if (gpuProps.IsSupportedNvidiaGpu())
        {
            auto timeNs = GetCounterValue("gpu__time_active.sum");
            elapsedTimeMs = timeNs / 1000000.0;
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            auto timeNs = GetCounterValue("ExecutionDuration");
            elapsedTimeMs = timeNs / 1000000.0;
        }
        else
#endif // _GAMING_DESKTOP
        {
            elapsedTimeMs = m_elapsedTime;
        }
        return elapsedTimeMs;
    }

#if defined(_GAMING_DESKTOP)

    // helper function to get all necessary counters on Nvidia / AMD that would count VMEM instructions, IPC, Throughput
    void GetVMemInstructions(uint32_t threadPerWave, uint64_t *outVmemInstructions, double *outVmemIPC, double *outVmemThroughput) const
    {
        double vmemGPU = 0.0;
        const auto& gpuProps = GpuProperties::Get();

        if (gpuProps.IsSupportedNvidiaGpu())
        {
            // TEX:
            //      Texture Unit.The SM texture pipeline forwards texture and surface instructions to the L1TEX unit's TEXIN stage.
            //      On GPUs where FP64 or Tensor pipelines are decoupled, the texture pipeline forwards those types of instructions, too.
            //
            // The below counters can be used to count "TEX" instructions
            // (which also include all Texture Sample\Load\Store instructions, Buffer Load/Store), their IPC and Throughput
            vmemGPU = GetCounterValue("sm__inst_executed_pipe_tex.sum");
            vmemGPU *= threadPerWave;

            if (nullptr != outVmemIPC)
                *outVmemIPC = GetCounterValue("sm__inst_executed_pipe_tex.sum.per_cycle_elapsed") * threadPerWave;

            if (nullptr != outVmemThroughput)
                *outVmemThroughput = GetCounterValue("sm__inst_executed_pipe_tex.sum.pct_of_peak_sustained_elapsed");
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            /**
             *  pass regex for AMD counters to match either of below mutually-exclusive possibilities:
             *      - SQWGP#_SQ_PERF_SEL_{name} for GPUs where there's SQ counter per WGP
             *      - SQ#_PERF_SEL_{name} - for GPUs where there's SQ counter per SA
             */
            vmemGPU = GetCounterValue("(SQWGP\\d+_SQ|SQ\\d+)_PERF_SEL_INSTS_TEX_LOAD")
                    + GetCounterValue("(SQWGP\\d+_SQ|SQ\\d+)_PERF_SEL_INSTS_TEX_STORE");

            vmemGPU *= threadPerWave;

            if (nullptr != outVmemIPC)
                *outVmemIPC = vmemGPU / GetCounterValue("GPUBusyCycles");

            // No good way to compute on AMD without knowing topology
            if (nullptr != outVmemThroughput)
                *outVmemThroughput = 0.0;
        }
        if (nullptr != outVmemInstructions)
            *outVmemInstructions = static_cast<uint64_t>(vmemGPU);
    }

    // helper functions to dynamically estimate wave size for most frequently used wave types
    // TODO: avoid "typed" duplication by generating function through macro?
    uint32_t GetCsWaveSize() const
    {
        auto csThreadCount = 0.0;
        auto csWaveCount = 0.0;
        const auto & gpuProps = GpuProperties::Get();

        if (gpuProps.IsSupportedNvidiaGpu())
        {
            csThreadCount = GetCounterValue("sm__threads_launched_shader_cs.sum");
            csWaveCount = GetCounterValue("sm__warps_launched_shader_cs.sum");

            assert(csThreadCount > 0.0);
            assert(csWaveCount > 0.0);
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            csThreadCount = GetCounterValue("(SQWGP_CS\\d+_SQ|SQ_CS\\d+)_PERF_SEL_ITEMS");

            // on AMD waves can run either as Wave32 or as Wave64, so we sum both
            // because the Test we run only one workload, so we can assume
            // Wave32/Wave64 are mutually exclusive
            csWaveCount = GetCounterValue("(SQWGP_CS\\d+_SQ|SQ_CS\\d+)_PERF_SEL_WAVES")
                        + GetCounterValue("(SQWGP_CS\\d+_SQ|SQ_CS\\d+)_PERF_SEL_WAVES_32");

            assert(csThreadCount > 0.0);
            assert(csWaveCount > 0.0);
        }

        auto csWaveSize = csThreadCount > 0.0 && csWaveCount > 0.0
                        ? static_cast<uint32_t>(csThreadCount / csWaveCount)
                        : 32U;

        // check the wave size is a power of 2
        assert(0 == (csWaveSize & (csWaveSize - 1)));
        return csWaveSize;
    }

    uint32_t GetPsWaveSize() const
    {
        auto psThreadCount = 0.0;
        auto psWaveCount = 0.0;
        const auto & gpuProps = GpuProperties::Get();

        if (gpuProps.IsSupportedNvidiaGpu())
        {
            psThreadCount = GetCounterValue("sm__threads_launched_shader_ps.sum");
            psWaveCount = GetCounterValue("sm__warps_launched_shader_ps.sum");

            assert(psThreadCount > 0.0);
            assert(psWaveCount > 0.0);
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            psThreadCount = GetCounterValue("(SQWGP_PS\\d+_SQ|SQ_PS\\d+)_PERF_SEL_ITEMS");

            // on AMD waves can run either as Wave32 or as Wave64, so we sum both
            // because the Test we run only one workload, so we can assume
            // Wave32/Wave64 are mutually exclusive
            psWaveCount = GetCounterValue("(SQWGP_PS\\d+_SQ|SQ_PS\\d+)_PERF_SEL_WAVES")
                        + GetCounterValue("(SQWGP_PS\\d+_SQ|SQ_PS\\d+)_PERF_SEL_WAVES_32");

            assert(psThreadCount > 0.0);
            assert(psWaveCount > 0.0);
        }

        auto psWaveSize = psThreadCount > 0.0 && psWaveCount > 0.0
                        ? static_cast<uint32_t>(psThreadCount / psWaveCount)
                        : 32U;

        // check the wave size is a power of 2
        assert(0 == (psWaveSize & (psWaveSize - 1)));
        return psWaveSize;
    }
#endif

public:
    static bool                                         m_dumpCounters;

protected:
    double                                              m_elapsedTime;
    GpuCounterTable                                     m_gpuCounterTable;

private:
    static DX::GPUTimer*                                m_gpuTime;

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_commandList;
};

__declspec(selectany) bool Test::m_dumpCounters = false;
__declspec(selectany) DX::GPUTimer* Test::m_gpuTime = nullptr;
