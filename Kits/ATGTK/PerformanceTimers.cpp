//--------------------------------------------------------------------------------------
// File: PerformanceTimers.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "PerformanceTimers.h"

#include "DirectXHelpers.h"

#include <exception>
#include <stdexcept>

using namespace DirectX;
using namespace DX;

using Microsoft::WRL::ComPtr;

namespace
{
    inline float lerp(float a, float b, float f)
    {
        return (1.f - f) * a + f * b;
    }

    inline float UpdateRunningAverage(float avg, float value)
    {
        return lerp(value, avg, 0.95f);
    }

    inline void DebugWarnings(uint32_t timerid, uint64_t start, uint64_t end)
    {
#if defined(_DEBUG)
        if (!start && end > 0)
        {
            char buff[128] = {};
            sprintf_s(buff, "ERROR: Timer %u stopped but not started\n", timerid);
            OutputDebugStringA(buff);
        }
        else if (start > 0 && !end)
        {
            char buff[128] = {};
            sprintf_s(buff, "ERROR: Timer %u started but not stopped\n", timerid);
            OutputDebugStringA(buff);
        }
#else
        UNREFERENCED_PARAMETER(timerid);
        UNREFERENCED_PARAMETER(start);
        UNREFERENCED_PARAMETER(end);
#endif
    }
};

//======================================================================================
// CPUTimer
//======================================================================================

CPUTimer::CPUTimer() :
    m_qpfFreqInv(1.f),
    m_start{},
    m_end{},
    m_avg{}
{
    LARGE_INTEGER qpfFreq;
    if (!QueryPerformanceFrequency(&qpfFreq))
    {
        throw std::exception("QueryPerformanceFrequency");
    }

    m_qpfFreqInv = 1000.0 / double(qpfFreq.QuadPart);
}

void CPUTimer::Start(uint32_t timerid)
{
    if (timerid >= c_maxTimers)
        throw std::out_of_range("Timer ID out of range");

    if (!QueryPerformanceCounter(&m_start[timerid]))
    {
        throw std::exception("QueryPerformanceCounter");
    }
}

void CPUTimer::Stop(uint32_t timerid)
{
    if (timerid >= c_maxTimers)
        throw std::out_of_range("Timer ID out of range");

    if (!QueryPerformanceCounter(&m_end[timerid]))
    {
        throw std::exception("QueryPerformanceCounter");
    }
}

void CPUTimer::Update()
{
    for (uint32_t j = 0; j < c_maxTimers; ++j)
    {
        auto start = static_cast<uint64_t>(m_start[j].QuadPart);
        auto end = static_cast<uint64_t>(m_end[j].QuadPart);

        DebugWarnings(j, start, end);

        float value = float(double(end - start) * m_qpfFreqInv);
        m_avg[j] = UpdateRunningAverage(m_avg[j], value);
    }
}

void CPUTimer::Reset()
{
    memset(m_avg, 0, sizeof(m_avg));
}

double CPUTimer::GetElapsedMS(uint32_t timerid) const
{
    if (timerid >= c_maxTimers)
        return 0.0;

    auto start = static_cast<uint64_t>(m_start[timerid].QuadPart);
    auto end = static_cast<uint64_t>(m_end[timerid].QuadPart);

    return double(end - start) * m_qpfFreqInv;
}


//======================================================================================
// GPUTimer (DirectX 12)
//======================================================================================
#if defined(__d3d12_h__)

void GPUTimer::BeginFrame(_In_ ID3D12GraphicsCommandList* commandList)
{
    UNREFERENCED_PARAMETER(commandList);
}

void GPUTimer::EndFrame(_In_ ID3D12GraphicsCommandList* commandList)
{
    // Resolve the query data in ranges to minimize API call count
    struct
    {
        uint32_t start;
        uint32_t size;
    } ranges[c_maxTimers / 2]{};
    uint32_t count = 0;

    for (uint32_t j = 0; j < c_maxTimers; ++j)
    {
        if (m_used[j])
        {
            // Determine if we're starting a new range
            if (ranges[count].size == 0)
            {
                ranges[count].start = j * 2;
            }

            ranges[count].size += 2; // Two slots per timer (begin, end timestamps)

            // Special case to end the range straddling the timer list end
            if (j == (c_maxTimers - 1))
            {
                ++count;
            }
        }
        else if (ranges[count].size > 0)
        {
            // End this range
            ++count;
        }
    }

    for (uint32_t j = 0; j < count; ++j)
    {
        commandList->ResolveQueryData(m_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, ranges[j].start, ranges[j].size, m_buffer.Get(), ranges[j].start * sizeof(UINT64));
    }

    memset(m_used, 0, sizeof(m_used));


    // Grab read-back data for the queries
    D3D12_RANGE dataRange =
    {
        0,
        c_timerSlots * sizeof(UINT64),
    };

    UINT64* timingData;
    ThrowIfFailed(m_buffer->Map(0, &dataRange, reinterpret_cast<void**>(&timingData)));
    memcpy(m_timing, timingData, sizeof(UINT64) * c_timerSlots);
    m_buffer->Unmap(0, nullptr);

    for (uint32_t j = 0; j < c_maxTimers; ++j)
    {
        UINT64 start = m_timing[j * 2];
        UINT64 end = m_timing[j * 2 + 1];

        DebugWarnings(j, start, end);

        float value = float(double(end - start) * m_gpuFreqInv);
        m_avg[j] = UpdateRunningAverage(m_avg[j], value);
    }
}

void GPUTimer::Flush(_In_ ID3D12GraphicsCommandList* commandList)
{
    UNREFERENCED_PARAMETER(commandList);
}

void GPUTimer::Start(_In_ ID3D12GraphicsCommandList* commandList, uint32_t timerid)
{
    if (timerid >= c_maxTimers)
        throw std::out_of_range("Timer ID out of range");

    commandList->EndQuery(m_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, timerid * 2);

    m_used[timerid] = true;
}

void GPUTimer::Stop(_In_ ID3D12GraphicsCommandList* commandList, uint32_t timerid)
{
    if (timerid >= c_maxTimers)
        throw std::out_of_range("Timer ID out of range");

    commandList->EndQuery(m_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, timerid * 2 + 1);
}

void GPUTimer::Reset()
{
    memset(m_avg, 0, sizeof(m_avg));
}

double GPUTimer::GetElapsedMS(uint32_t timerid) const
{
    if (timerid >= c_maxTimers)
        return 0.0;

    UINT64 start = m_timing[timerid * 2];
    UINT64 end = m_timing[timerid * 2 + 1];

    if (end < start)
        return 0.0;

    return double(end - start) * m_gpuFreqInv;
}

void GPUTimer::ReleaseDevice()
{
    m_heap.Reset();
    m_buffer.Reset();
}

void GPUTimer::RestoreDevice(_In_ ID3D12Device* device, _In_ ID3D12CommandQueue* commandQueue)
{
    assert(device != 0 && commandQueue != 0);

#if defined(_DEBUG) || defined(PROFILE)
    if (FAILED(device->SetStablePowerState(TRUE)))
    {
        OutputDebugStringA("WARNING: Unable to set a stable power state.\n        profiling data may be inaccurate due to power management!\n");
    }
#endif

    UINT64 gpuFreq;
    ThrowIfFailed(commandQueue->GetTimestampFrequency(&gpuFreq));
    m_gpuFreqInv = 1000.0 / double(gpuFreq);

    D3D12_QUERY_HEAP_DESC desc = {};
    desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    desc.Count = c_timerSlots;
    ThrowIfFailed(device->CreateQueryHeap(&desc, IID_GRAPHICS_PPV_ARGS(m_heap.ReleaseAndGetAddressOf())));
    m_heap->SetName(L"GPUTimer");

    auto readBack = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(c_timerSlots * sizeof(UINT64));
    ThrowIfFailed(device->CreateCommittedResource(
        &readBack,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_GRAPHICS_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf()))
    );
    m_buffer->SetName(L"GPUTimer");
}


//======================================================================================
// GPUTimer (DirectX 11)
//======================================================================================
#elif defined(__d3d11_h__)

void GPUTimer::BeginFrame(_In_ ID3D11DeviceContext* context)
{
    m_frame[m_currentFrame].BeginFrame(context);
}

void GPUTimer::EndFrame(_In_ ID3D11DeviceContext* context)
{
    m_frame[m_currentFrame].EndFrame(context);
    m_frame[m_currentFrame].m_pending = true;
    m_currentFrame = (m_currentFrame + 1) % c_bufferCount;

    if (m_frame[m_currentFrame].m_pending)
    {
        m_frame[m_currentFrame].ComputeFrame(context);
        m_frame[m_currentFrame].m_pending = false;
    }
}

void GPUTimer::Flush(_In_ ID3D11DeviceContext* context)
{
    for (size_t frame = 0; frame < 2 * (c_bufferCount - 1); ++frame)
    {
        BeginFrame(context);
        EndFrame(context);
    }
}

void GPUTimer::Start(_In_ ID3D11DeviceContext* context, uint32_t timerid)
{
    m_frame[m_currentFrame].Start(context, timerid);
}

void GPUTimer::Stop(_In_ ID3D11DeviceContext* context, uint32_t timerid)
{
    m_frame[m_currentFrame].Stop(context, timerid);
}

void GPUTimer::Reset()
{
    for (size_t i = 0; i < c_bufferCount; ++i)
    {
        memset(m_frame[i].m_avg, 0, sizeof(Frame::m_avg));
    }
}

double GPUTimer::GetElapsedMS(uint32_t timerid) const
{
    if (timerid >= c_maxTimers)
        return 0.0;

    uint32_t oldestBuffer = (m_currentFrame + 1) % c_bufferCount;
    return m_frame[oldestBuffer].m_timing[timerid];
}

float GPUTimer::GetAverageMS(uint32_t timerid) const
{
    if (timerid >= c_maxTimers)
        return 0.f;

    uint32_t oldestBuffer = (m_currentFrame + 1) % c_bufferCount;
    return m_frame[oldestBuffer].m_avg[timerid];
}

void GPUTimer::ReleaseDevice()
{
    for (size_t i = 0; i < c_bufferCount; ++i)
    {
        m_frame[i].m_disjoint.Reset();

        for (size_t j = 0; j < c_maxTimers; ++j)
        {
            m_frame[i].m_start[j].Reset();
            m_frame[i].m_end[j].Reset();
        }
    }
}

void GPUTimer::RestoreDevice(_In_ ID3D11Device* device)
{
    assert(device != 0);

    for (size_t i = 0; i < c_bufferCount; ++i)
    {
        D3D11_QUERY_DESC desc = {};
        desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        ThrowIfFailed(device->CreateQuery(&desc, m_frame[i].m_disjoint.ReleaseAndGetAddressOf()));
        SetDebugObjectName(m_frame[i].m_disjoint.Get(), "GPUTimer");

        desc.Query = D3D11_QUERY_TIMESTAMP;
        for (size_t j = 0; j < c_maxTimers; ++j)
        {
            ThrowIfFailed(device->CreateQuery(&desc, m_frame[i].m_start[j].ReleaseAndGetAddressOf()));
            SetDebugObjectName(m_frame[i].m_start[j].Get(), "GPUTimer");

            ThrowIfFailed(device->CreateQuery(&desc, m_frame[i].m_end[j].ReleaseAndGetAddressOf()));
            SetDebugObjectName(m_frame[i].m_end[j].Get(), "GPUTimer");

        }
    }
}

void GPUTimer::Frame::BeginFrame(_In_ ID3D11DeviceContext* context)
{
    memset(&m_used, 0, sizeof(m_used));

    context->Begin(m_disjoint.Get());
}

void GPUTimer::Frame::EndFrame(_In_ ID3D11DeviceContext* context)
{
    context->End(m_disjoint.Get());
}

void GPUTimer::Frame::ComputeFrame(_In_ ID3D11DeviceContext* context)
{
#if defined(_DEBUG) || defined(PROFILE)
    bool blocked = false;
#endif

    while (S_FALSE == context->GetData(m_disjoint.Get(), nullptr, 0, 0))
    {
#if defined(_DEBUG) || defined(PROFILE)
        if (!blocked)
        {
            OutputDebugStringA("WARNING: Blocked performance queries\n");
            blocked = true;
        }
#endif
        Sleep(1);
    }

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint = { 1, TRUE };
    if (SUCCEEDED(context->GetData(m_disjoint.Get(), &disjoint, sizeof(disjoint), 0)))
    {
        double gpuFreqInv = 1000.0 / double(disjoint.Frequency);

        for (uint32_t j = 0; j < c_maxTimers; ++j)
        {
            if (!m_used[j])
            {
                m_timing[j] = 0.f;
                continue;
            }

            while (S_FALSE == context->GetData(m_start[j].Get(), nullptr, 0, 0))
            {
#if defined(_DEBUG) || defined(PROFILE)
                if (!blocked)
                {
                    OutputDebugStringA("WARNING: Blocked performance queries\n");
                    blocked = true;
                }
#endif
                Sleep(1);
            }

            while (S_FALSE == context->GetData(m_end[j].Get(), nullptr, 0, 0))
            {
#if defined(_DEBUG) || defined(PROFILE)
                if (!blocked)
                {
                    OutputDebugStringA("WARNING: Blocked performance queries\n");
                    blocked = true;
                }
#endif
                Sleep(1);
            }

            UINT64 start;
            if (FAILED(context->GetData(m_start[j].Get(), &start, sizeof(start), 0)))
                start = 0;

            UINT64 end;
            if (FAILED(context->GetData(m_end[j].Get(), &end, sizeof(end), 0)))
                end = 0;

            if (!disjoint.Disjoint)
            {
                DebugWarnings(j, start, end);

                float value = float(double(end - start) * gpuFreqInv);
                m_timing[j] = value;
                m_avg[j] = UpdateRunningAverage(m_avg[j], value);
            }
        }
    }
}

void GPUTimer::Frame::Start(_In_ ID3D11DeviceContext* context, uint32_t timerid)
{
    if (timerid >= c_maxTimers)
        throw std::out_of_range("Timer ID out of range");

    m_used[timerid] = true;

    context->End(m_start[timerid].Get());
}

void GPUTimer::Frame::Stop(_In_ ID3D11DeviceContext* context, uint32_t timerid)
{
    if (timerid >= c_maxTimers)
        throw std::out_of_range("Timer ID out of range");

    context->End(m_end[timerid].Get());
}


#else
#error Must include d3d11*.h or d3d12*.h in pch.h
#endif
