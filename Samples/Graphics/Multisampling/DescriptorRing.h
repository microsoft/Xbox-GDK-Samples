//--------------------------------------------------------------------------------------
// DescriptorRing.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------------------
// Name: DescriptorRing
// Desc: A ring of shader-visible descriptors which are gathered into tables, matching the
//  declaration order in the shader.
//
// We don't bother checking for overwrite of descriptors which are still in flight.
// Just bump the ring size if this ever happens.
//-------------------------------------------------------------------------------------------------------------
template< D3D12_DESCRIPTOR_HEAP_TYPE t_type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV >
struct DescriptorRing
{
private:
	struct Table;

public:
	DescriptorRing()
		: m_device(nullptr)
		, m_shaderHeap(nullptr)
		, m_nextTableStart(0U)
		, m_ringStart(0U)
		, m_ringSize(0U)
	{
	}

	void Initialize(ID3D12Device* device, ID3D12DescriptorHeap* shaderHeap, uint32_t ringStart, uint32_t ringSize)
	{
		m_device = device;
		m_shaderHeap = shaderHeap;

		m_incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_ringStart = ringStart;
		m_ringSize = ringSize;

		m_nextTableStart = m_ringStart;
	}

	// Set usedSize to how many descriptors you will bind.
	// Set declaredSize to how many descriptors are declared by the root signature.
	inline Table AllocateTable(uint32_t usedSize, uint32_t declaredSize)
	{
		if (m_nextTableStart + declaredSize > m_ringStart + m_ringSize)
		{
			m_nextTableStart = m_ringStart;
		}

		auto tableStart = m_nextTableStart;

		m_nextTableStart += usedSize;

		return Table(tableStart, usedSize);
	}

	inline void SetDescriptor(Table table, D3D12_CPU_DESCRIPTOR_HANDLE descriptorCPU, uint32_t slot)  const
	{
		assert(slot <= table.m_size);

		auto heapStartCpu = m_shaderHeap->GetCPUDescriptorHandleForHeapStart();
		auto descriptorDest = CD3DX12_CPU_DESCRIPTOR_HANDLE(heapStartCpu, table.m_start + slot, m_incrementSize);

		// Copy descriptor to shader-visible heap
		m_device->CopyDescriptorsSimple(1U,
			descriptorDest,
			descriptorCPU,
			t_type);
	}

	inline void SetGraphicsTable(ID3D12GraphicsCommandList* commandList, Table table, uint32_t rootElement)
	{
		auto heapStartGpu = m_shaderHeap->GetGPUDescriptorHandleForHeapStart();
		auto descriptorDest = CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStartGpu, table.m_start, m_incrementSize);

		commandList->SetGraphicsRootDescriptorTable(rootElement, descriptorDest);
	}

	inline void SetComputeTable(ID3D12GraphicsCommandList* commandList, Table table, uint32_t rootElement)
	{
		auto heapStartGpu = m_shaderHeap->GetGPUDescriptorHandleForHeapStart();
		auto descriptorDest = CD3DX12_GPU_DESCRIPTOR_HANDLE(heapStartGpu, table.m_start, m_incrementSize);

		commandList->SetComputeRootDescriptorTable(rootElement, descriptorDest);
	}

private:
	struct Table
	{
		Table(uint32_t start, uint32_t size)
			: m_start(start)
			, m_size(size)
		{}
		uint32_t m_start;
		uint32_t m_size;
	};

	ID3D12Device* m_device;
	ID3D12DescriptorHeap* m_shaderHeap;					// the heap where descriptors reside
	uint32_t m_incrementSize = 0;							// the descriptor increment size

	uint32_t m_nextTableStart;

	uint32_t m_ringStart;
	uint32_t m_ringSize;
};
