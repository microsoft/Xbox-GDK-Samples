//
// GpuProfiler.cpp
//

#include "pch.h"
#include "GpuProfiler.h"

using namespace DirectX;
using namespace DX;

using Microsoft::WRL::ComPtr;


DX12Timer::DX12Timer()
: m_readBackBuffer(nullptr)
, m_queryHeap(nullptr)
, m_numCounters(0)
, m_frameIndex(0)
, m_frequencyToMs(0.0)
, m_firstFrame(true)
{

}


void DX12Timer::Initialize(ID3D12Device *device, ID3D12CommandQueue *queue, UINT numCounters)
{
    assert(m_numCounters == 0);
    assert(numCounters > 0);

    m_numCounters = numCounters;
    m_frameIndex = 0;
    {
        D3D12_QUERY_HEAP_DESC heapDesc = { };
        heapDesc.Count = m_numCounters * 4;     // double buffer, 2x entries per timer, start and stop
        heapDesc.NodeMask = 0;
        heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        device->CreateQueryHeap(&heapDesc, IID_GRAPHICS_PPV_ARGS(&m_queryHeap));
    }
    {
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = m_numCounters * 2 * sizeof(UINT64);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Alignment = 0;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_READBACK;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 0;
        heapProps.VisibleNodeMask = 0;

        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_GRAPHICS_PPV_ARGS(&m_readBackBuffer));
    }
    UINT64 frequencyHz;
    queue->GetTimestampFrequency(&frequencyHz);
    m_frequencyToMs = 1000.0 / double(frequencyHz);
    m_firstFrame = true;
}


DX12Timer::~DX12Timer()
{
    if (m_readBackBuffer)
    {
        m_readBackBuffer->Release();
        m_readBackBuffer = nullptr;
    }
    if (m_queryHeap)
    {
        m_queryHeap->Release();
        m_queryHeap = nullptr;
    }
    m_numCounters = 0;
}


void DX12Timer::EndFrame(ID3D12GraphicsCommandList *cmdList, float *resultsMs)
{
    if (!m_numCounters)
        return;

    cmdList->ResolveQueryData(m_queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, m_frameIndex * m_numCounters * 2, m_numCounters * 2, m_readBackBuffer, 0);

    if (m_firstFrame)       
    {
        // don't read on CPU timeline yet as GPU has not yet been kicked to ResolveQueryData, so there is nothing valid in m_readBackBuffer
        ZeroMemory(resultsMs, m_numCounters * sizeof(float));

        m_firstFrame = false;
    }
    else                    
    {
        // obtain the most up to date counters available, which might be stale by a frame, but they're whatever got resolved when the GPU ran ResolveQueryData
        UINT64* queryData;
        m_readBackBuffer->Map(0, nullptr, (void **)&queryData);    
        double frequencyToMs = m_frequencyToMs;

        for (UINT i = 0; i < m_numCounters; ++i)
        {
            UINT64 startTime = queryData[i * 2 + 0];
            UINT64 endTime = queryData[i * 2 + 1];
            resultsMs[i] = (endTime > startTime) ? float(frequencyToMs * double(endTime - startTime)) : 0.0f;
        }
        m_readBackBuffer->Unmap(0, nullptr);
    }
    m_frameIndex ^= 1;
}
