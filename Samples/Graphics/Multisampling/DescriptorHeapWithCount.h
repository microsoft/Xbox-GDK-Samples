//--------------------------------------------------------------------------------------
// DescriptorHeapWithCount.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

template < uint32_t t_max  >
struct DescriptorHeapWithCount
{
    DescriptorHeapWithCount() :
        m_incrementSize(0),
        m_count(0U)
    {}

    void Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible)
    {
        m_incrementSize = device->GetDescriptorHandleIncrementSize(type);

        auto flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        D3D12_DESCRIPTOR_HEAP_DESC descHeapSrv =
        {
            type,					// D3D12_DESCRIPTOR_HEAP_TYPE Type;
            t_max,					// uint32_t NumDescriptors;
            flags,					// D3D12_DESCRIPTOR_HEAP_FLAGS Flags;
                                    // uint32_t NodeMask;
        };
        DX::ThrowIfFailed(device->CreateDescriptorHeap(&descHeapSrv, IID_GRAPHICS_PPV_ARGS(m_heap.ReleaseAndGetAddressOf())));
    }

    uint32_t GetMax() const
    {
        return t_max;
    }

    ID3D12DescriptorHeap* GetHeap() const
    {
        return m_heap.Get();
    }

    void AdvanceIndex()
    {
        m_count++;
    }

    uint32_t GetCurrentIndex() const
    {
        return m_count;
    }

    void SetNextIndex(uint32_t iCount)
    {
        assert(iCount < t_max);
        m_count = iCount;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentCpuHandle() const
    {
        assert(m_count + 1 < t_max);
        auto heapStartCpu = m_heap->GetCPUDescriptorHandleForHeapStart();
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(heapStartCpu, m_count, m_incrementSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetCurrentGpuHandle() const
    {
        assert(m_count < t_max);
        auto heapStartGpu = m_heap->GetGPUDescriptorHandleForHeapStart();
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStartGpu, m_count, m_incrementSize);
    }

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;
    uint32_t m_incrementSize;                                // the descriptor increment size
    uint32_t m_count;
};

