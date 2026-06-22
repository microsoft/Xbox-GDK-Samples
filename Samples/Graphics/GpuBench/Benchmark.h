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

        Initialize(device);

        m_gpuCounterSet.Initialize();

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

    template<typename t_counterType>
    void AddCounter(t_counterType id, GpuCounter::ShaderMask shaderMask = GpuCounter::SHADER_MASK_ALL)
    {
        m_gpuCounterSet.AddCounter(id, shaderMask);
    }

    std::vector<Test*>                          m_tests;
    State                                       m_state;
    Report                                      m_report;
    GpuCounterSet                               m_gpuCounterSet;
    Capture                                     m_capture;
};
