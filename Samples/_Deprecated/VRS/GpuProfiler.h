//
// GpuProfiler.h
//

#pragma once

#include "pch.h"

namespace DirectX
{
    class DX12Timer
    {
    private:
        ID3D12Resource*     m_readBackBuffer;
        ID3D12QueryHeap*    m_queryHeap;
        UINT                m_numCounters;
        UINT                m_frameIndex;
        double              m_frequencyToMs;
        bool                m_firstFrame;

        UINT GetTimerIndex(UINT index, bool start)
        {
            assert(index < m_numCounters);
            return ((m_frameIndex * m_numCounters) + index) * 2 + (start ? 0 : 1);
        }

    public:
        DX12Timer();
        ~DX12Timer();

        void Initialize(ID3D12Device *device, ID3D12CommandQueue *queue, UINT numCounters);

        void BeginFrame() { }

        void StartTimer(ID3D12GraphicsCommandList *cmdList, UINT index)
        {
            cmdList->EndQuery(m_queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, GetTimerIndex(index, true));
        }

        void EndTimer(ID3D12GraphicsCommandList *cmdList, UINT index)
        {
            cmdList->EndQuery(m_queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, GetTimerIndex(index, false));
        }

        void EndFrame(ID3D12GraphicsCommandList *cmdList, float *resultsMs);
    };


    class TimeScope
    {
        DX12Timer &m_timer;
        ID3D12GraphicsCommandList *m_cmdList;
        UINT m_index;

    public:
        TimeScope(DX12Timer &timer, ID3D12GraphicsCommandList *cmdList, UINT index)
        : m_timer(timer)
        , m_cmdList(cmdList)
        , m_index(index)
        {
            m_timer.StartTimer(m_cmdList, m_index);
        }

        ~TimeScope()
        {
            m_timer.EndTimer(m_cmdList, m_index);
        }
    };
}
