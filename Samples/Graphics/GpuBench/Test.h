//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "PerformanceTimersXbox.h"
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
        gpuCounterSet->StartCollect(device);

        while (!gpuCounterSet->Done())
        {
            Start(m_commandList.Get());

            Execute(commandQueue);
            FlushAndHalt(device, commandQueue);
            Reset();

            gpuCounterSet->StartPass(m_commandList.Get());

            Run(m_commandList.Get());

            gpuCounterSet->EndPass(m_commandList.Get());

            Execute(commandQueue);
            FlushAndHalt(device, commandQueue);
            Reset();

            Stop(m_commandList.Get());
        }

        gpuCounterSet->RetrieveCounters(m_gpuCounterTable);

        if (m_dumpCounters)
        {
            gpuCounterSet->DumpCounters();
        }

        gpuCounterSet->EndCollect(device);
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
        // Flush caches, and block on the CPU until completion
        GpuCacheFlushAndFullBarrier(device, commandQueue);
        GpuFullStop(device, commandQueue);
    }

    virtual void PutReport(Report* report) const = 0;

    virtual std::wstring GetName() const = 0;
    virtual void Initialize(ID3D12Device* device) = 0;
    virtual void Start(ID3D12GraphicsCommandList*) const {};        // override of this function is optional
    virtual void Run(ID3D12GraphicsCommandList* commandList) const = 0;
    virtual void Stop(ID3D12GraphicsCommandList*) const {};         // override of this function is optional
    virtual void Uninitialize() = 0;

    template<typename t_counterType>
    CounterValue GetCounterValue(t_counterType id, GpuCounter::ShaderMask shaderMask = GpuCounter::SHADER_MASK_ALL, CounterAggregator summarizer = CounterValueArrayAdd) const
    {
        auto counter = GpuCounter(id, shaderMask);
        try
        {
            return summarizer(m_gpuCounterTable.at(counter));
        }
        catch (std::out_of_range)
        {
            std::wostringstream message;
            message << L"Tried to retrieve ";
            message << counter.GetTypeName();
            message << L" counter ";
            message << id;
            message << L" shaderMask ";
            message << L"0x" << std::setw(2) << std::setfill(L'0') << std::hex << shaderMask;
            message << ", which wasn't added!";
            message << L"\"";
            message << std::endl;
            OutputDebugString(message.str().c_str());
            assert(false);
            return 0U;
        }
    }

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
