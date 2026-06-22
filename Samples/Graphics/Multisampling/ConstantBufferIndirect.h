//--------------------------------------------------------------------------------------
// ConstantBufferIndirect.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Util.h"

#if defined(_GAMING_XBOX)
#define CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT 64
#else
#define CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT
#endif

//-------------------------------------------------------------------------------------------------------------
// Name: ConstantBufferIndirect
// Desc: A dynamic constant buffer within a descriptor table.
//-------------------------------------------------------------------------------------------------------------
template< typename t_struct >
struct ConstantBufferIndirect
{
    ConstantBufferIndirect() : 
        m_heapOffset(0),
        m_incrementSize(0),
        m_uploadHeap(nullptr),
        m_instances(0),
        m_buffer(nullptr),
        m_instance(0)
    {}

    uint32_t Initialize(ID3D12Device* device,
        uint32_t heapOffset,
        uint32_t instances,
        ID3D12DescriptorHeap* uploadHeap)
    {
        m_heapOffset = heapOffset;
        m_instances = instances;
        m_uploadHeap = uploadHeap;
        m_incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // Create a resource big enough to contain m_iInstances copies of t_struct
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(
            m_instances * m_alignedSize												// uint32_t64 width,
                                                                                    // D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
                                                                                    // uint32_t64 alignment = 0 )
        );
        const D3D12_HEAP_PROPERTIES defaultHeapProperties =
        {
            D3D12_HEAP_TYPE_UPLOAD,                                                 // D3D12_HEAP_TYPE Type;
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,                                        // D3D12_CPU_PAGE_PROPERTY CPUPageProperty;
            D3D12_MEMORY_POOL_UNKNOWN,                                              // D3D12_MEMORY_POOL MemoryPoolPreference;
        };
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_buffer.ReleaseAndGetAddressOf())));

        // Name the constant buffer according to the type of t_struct
        wchar_t name[256] = L"";
        swprintf_s(name, L"%hs", typeid(t_struct).name());
        m_buffer->SetName(name);

        for (uint32_t instance = 0; instance < m_instances; ++instance)
        {
            // Assign to descriptor
            D3D12_CONSTANT_BUFFER_VIEW_DESC descConstantBufferView =
            {
                m_buffer->GetGPUVirtualAddress()
                + instance * m_alignedSize,											// D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
                m_alignedSize,														// uint32_t SizeInBytes;
            };

            auto heapStartCpu = m_uploadHeap->GetCPUDescriptorHandleForHeapStart();
            auto descriptorCbv = CD3DX12_CPU_DESCRIPTOR_HANDLE(heapStartCpu, m_heapOffset + instance, m_incrementSize);

            device->CreateConstantBufferView(&descConstantBufferView, descriptorCbv);
        }

        return heapOffset + instances;
    }

    void Deinitialize()
    {
        m_buffer.Reset();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE ReplaceContents(const t_struct& data)
    {
        m_instance = ++m_instance % m_instances;

        // Replace the contents of the m_iInstance'th element in the buffer
        // We do this unconditionally, without checking fences, for now
        // This relies upon the declaration correctly supplying m_iInstances
        D3D12_RANGE range =
        {
            m_instance * m_alignedSize,                                           // SIZE_T Begin;
            m_instance * m_alignedSize + sizeof(t_struct),                      // SIZE_T End;
        };
        void* pDst;
        m_buffer->Map(0, nullptr, &pDst);                                        // We will not read, only write
        pDst = reinterpret_cast<BYTE*>(pDst) + m_instance * m_alignedSize;    // advance by aligned size
        *reinterpret_cast<t_struct*>(pDst) = data;
        m_buffer->Unmap(0, &range);

        auto heapStartCpu = m_uploadHeap->GetCPUDescriptorHandleForHeapStart();
        auto descriptorCbv = CD3DX12_CPU_DESCRIPTOR_HANDLE(heapStartCpu, m_heapOffset + m_instance, m_incrementSize);

        return descriptorCbv;
    }

    static uint32_t GetSlot()
    {
        return t_struct::c_slot;
    }

private:
    static const SIZE_T m_alignedSize = NextMultipleConstexpr(sizeof(t_struct), CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    uint32_t m_heapOffset;									// the offset within the descriptor table where this constant buffer lies
    uint32_t m_incrementSize;								// the descriptor increment size
    ID3D12DescriptorHeap* m_uploadHeap;					// the heap where descriptors reside

    uint32_t m_instances;									// the number of dynamic multiple-buffers
    Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;	// the resource containing the multiple-buffers
    uint32_t m_instance;									// the current dynamic buffer
};

