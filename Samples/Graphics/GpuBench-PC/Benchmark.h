//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Capture.h"
#include "GpuCounterSet.h"
#include "Report.h"
#include "Test.h"

// The benchmark interface
class IBenchmark
{
public:
    enum State : uint32_t
    {
        STATE_NOT_STARTED,
        STATE_STARTED,
        STATE_MEASURE_TIME,
        STATE_MEASURE_COUNTERS,
        STATE_REPORT,
        STATE_PIX_CAPTURE,
        STATE_COMPLETE,
        STATE_STOPPED,

        STATE_COUNT
    };
    virtual const wchar_t* GetName() const = 0;
    virtual void Start(ID3D12Device* device) = 0;
    virtual State GetState() const = 0;
    virtual void RunOneFrame(ID3D12Device* device, ID3D12CommandQueue* commandQueue) = 0;
    virtual void Stop() = 0;
    virtual std::wstring GetReportHeader() const = 0;
    virtual size_t GetReportRowCount() const = 0;
    virtual std::wstring GetReportRow(uint32_t i) const = 0;
    virtual DirectX::XMVECTOR GetReportRowColor(uint32_t i) const = 0;
};

// Class to allow transparent add/remove of benchmarks by adding/removing a new .cpp file
class Benchmark : public IBenchmark
{
public:
    Benchmark() :
        m_state(STATE_NOT_STARTED)
    {
        BenchmarkList().push_back(this);
    }

    // We don't expect this to ever be called
    virtual ~Benchmark()
    {
        for (auto test : m_tests)
        {
            delete test;
        }
        BenchmarkList().erase(std::remove(BenchmarkList().begin(), BenchmarkList().end(), this), BenchmarkList().end());
    }

    // Use "Construct On First Use Idiom" to avoid order-of-static-initialization dependencies
    static std::vector<IBenchmark*>& BenchmarkList()
    {
        static std::vector<IBenchmark*> benchmarkList;

        return benchmarkList;
    }

    State GetState() const override final
    {
        return m_state;
    }
    std::wstring GetReportHeader() const override final
    {
        return m_report.GetHeader();
    }
    size_t GetReportRowCount() const override final
    {
        return m_report.GetRowCount();
    }
    std::wstring GetReportRow(uint32_t i) const override final
    {
        return m_report.GetRow(i);
    }
    DirectX::XMVECTOR GetReportRowColor(uint32_t i) const override final
    {
        return m_report.GetRowColor(i);
    }

    virtual void Initialize(ID3D12Device* device) = 0;
    void Start(ID3D12Device* device) override final
    {
        m_state = STATE_STARTED;
        m_report.Reset();

        m_gpuCounterSet.Initialize();

        m_gpuCounterSet.BeginAddingCounters();
        Initialize(device);
        m_gpuCounterSet.EndAddingCounters();

        for (auto test : m_tests)
        {
            test->InitializeCommon(device);
            test->Initialize(device);
        }
    }

    void RunOneFrame(ID3D12Device* device, ID3D12CommandQueue* commandQueue) override final
    {
        StartFrame(commandQueue);

        RunTests(device, commandQueue);

        EndFrame(commandQueue);
    }

    virtual void Uninitialize() = 0;
    void Stop() override final
    {
        for (auto test : m_tests)
        {
            test->UninitializeCommon();
            test->Uninitialize();
        }

        Uninitialize();

        m_tests.clear();

        m_gpuCounterSet.Uninitialize();

        m_state = STATE_STOPPED;
    }

    void StartFrame(ID3D12CommandQueue* commandQueue)
    {
        if (STATE_PIX_CAPTURE == m_state)
        {
            m_capture.Start(commandQueue, GetName());
        }
    }

    void EndFrame(ID3D12CommandQueue* commandQueue)
    {
        if (STATE_PIX_CAPTURE == m_state)
        {
            m_capture.End(commandQueue);
        }

        m_state = State(m_state + 1);
    }

protected:
    void AddTest(Test* test)
    {
        m_tests.push_back(test);
    }

    void RunTests(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
    {
        for (auto test : m_tests)
        {
            switch (m_state)
            {
            case STATE_MEASURE_TIME:
                test->MeasureTime(device, commandQueue);
                break;

            case STATE_MEASURE_COUNTERS:
                test->MeasureCounters(device, commandQueue, &m_gpuCounterSet);
                break;

            case STATE_REPORT:
                test->PutReport(&m_report);
                break;

            case STATE_PIX_CAPTURE:
                test->Capture(device, commandQueue);
                break;
            }
        }
    }

#ifdef _GAMING_XBOX
    template<typename t_counterType>
    void AddCounter(t_counterType id, GpuCounter::ShaderMask shaderMask = GpuCounter::SHADER_MASK_ALL)
    {
        m_gpuCounterSet.AddCounter(id, shaderMask);
    }
#else
    void AddCounter(const char *id)
    {
        m_gpuCounterSet.AddCounter(id);
    }

    void AddCounter_ElapsedTime()
    {
        const auto& gpuProps = GpuProperties::Get();
        if (gpuProps.IsSupportedNvidiaGpu())
        {
            AddCounter("gpu__time_active.sum");
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            AddCounter("ExecutionDuration");
        }
    }

    // helper function to add all necessary counters on Nvidia / AMD that would count VMEM instructions
    void AddCounter_VMemInstructions()
    {
        const auto& gpuProps = GpuProperties::Get();

        if (gpuProps.IsSupportedNvidiaGpu())
        {
            // TEX:
            //      Texture Unit.The SM texture pipeline forwards texture and surface instructions to the L1TEX unit's TEXIN stage.
            //      On GPUs where FP64 or Tensor pipelines are decoupled, the texture pipeline forwards those types of instructions, too.
            //
            // The below counters can be used to count "TEX" instructions
            // (which also include all Texture Sample\Load\Store instructions, Buffer Load/Store), their IPC and Throughput
            AddCounter("sm__inst_executed_pipe_tex.sum");
            AddCounter("sm__inst_executed_pipe_tex.sum.per_cycle_elapsed");
            AddCounter("sm__inst_executed_pipe_tex.sum.pct_of_peak_sustained_elapsed");
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            /**
             *  pass regex for AMD counters to match either of below mutually-exclusive possibilities:
             *      - SQWGP#_SQ_PERF_SEL_{name} for GPUs where there's SQ counter per WGP
             *      - SQ#_PERF_SEL_{name} - for GPUs where there's SQ counter per SA
             */
            AddCounter("(SQWGP\\d+_SQ|SQ\\d+)_PERF_SEL_INSTS_TEX_LOAD");
            AddCounter("(SQWGP\\d+_SQ|SQ\\d+)_PERF_SEL_INSTS_TEX_STORE");

            // Add GPU Busy Cycles to be able to get instructions per clock
            AddCounter("GPUBusyCycles");
        }
    }

    // below are helpers to add counters used frequently to estimate wave size at runtime
    // TODO: avoid "typed" duplication by generating function through macro?
    void AddCounter_CsWaveSize()
    {
        const auto & gpuProps = GpuProperties::Get();

        if (gpuProps.IsSupportedNvidiaGpu())
        {
            AddCounter("sm__threads_launched_shader_cs.sum");
            AddCounter("sm__warps_launched_shader_cs.sum");
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            //
            //  pass regex for AMD counters to match either of below mutually-exclusive possibilities:
            //      - SQWGP#_SQ_PERF_SEL_{name} for GPUs where there's SQ counter per WGP
            //      - SQ#_PERF_SEL_{name} - for GPUs where there's SQ counter per SA
            //
            AddCounter("(SQWGP_CS\\d+_SQ|SQ_CS\\d+)_PERF_SEL_WAVES");
            AddCounter("(SQWGP_CS\\d+_SQ|SQ_CS\\d+)_PERF_SEL_WAVES_32");
            AddCounter("(SQWGP_CS\\d+_SQ|SQ_CS\\d+)_PERF_SEL_ITEMS");
        }
    }
    void AddCounter_PsWaveSize()
    {
        const auto & gpuProps = GpuProperties::Get();

        if (gpuProps.IsSupportedNvidiaGpu())
        {
            AddCounter("sm__threads_launched_shader_ps.sum");
            AddCounter("sm__warps_launched_shader_ps.sum");
        }
        else if (gpuProps.IsSupportedAmdGpu())
        {
            //
            //  pass regex for AMD counters to match either of below mutually-exclusive possibilities:
            //      - SQWGP#_SQ_PERF_SEL_{name} for GPUs where there's SQ counter per WGP
            //      - SQ#_PERF_SEL_{name} - for GPUs where there's SQ counter per SA
            //
            AddCounter("(SQWGP_PS\\d+_SQ|SQ_PS\\d+)_PERF_SEL_WAVES");
            AddCounter("(SQWGP_PS\\d+_SQ|SQ_PS\\d+)_PERF_SEL_WAVES_32");
            AddCounter("(SQWGP_PS\\d+_SQ|SQ_PS\\d+)_PERF_SEL_ITEMS");
        }
    }
#endif

    std::vector<Test*>                          m_tests;
    State                                       m_state;
    Report                                      m_report;
    GpuCounterSet                               m_gpuCounterSet;
    Capture                                     m_capture;
};
