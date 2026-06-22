//--------------------------------------------------------------------------------------
// TextureStreamingUtils.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "precomp.hpp"
#include "TextureStreamingUtils.h"

bool StagingTexture::CreateCpuOnlyStaging(ID3D12Device* pd3dDevice, const D3D12_RESOURCE_DESC& TextureDesc)
{
    const UINT32 SliceCount = TextureDesc.DepthOrArraySize;

    D3D12_RESOURCE_DESC StagingDesc = TextureDesc;
    StagingDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    StagingDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    StagingDesc.MipLevels = 1;
    StagingDesc.DepthOrArraySize = 1;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint = {};
    UINT64 TotalBytes = 0;
    pd3dDevice->GetCopyableFootprints(&StagingDesc, 0, 1, 0, &Footprint, nullptr, nullptr, &TotalBytes);
    UINT64 SliceSizeBytes = Footprint.Footprint.RowPitch * Footprint.Footprint.Height;
    if (SliceSizeBytes > TotalBytes)
    {
        TotalBytes = SliceSizeBytes;
    }
    TotalBytes *= SliceCount;

    BYTE* pBuffer = new BYTE[TotalBytes];
    if (pBuffer == nullptr)
    {
        return false;
    }

    pLockedBits = pBuffer;
    pStagingBuffer = nullptr;

    CopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    CopyLocation.pResource = nullptr;
    Footprint.Offset = 0;
    CopyLocation.PlacedFootprint = Footprint;

    CopyRange.Begin = 0;
    CopyRange.End = TotalBytes;

    return true;
}

void StagingTexture::DeleteCpuOnlyStaging()
{
    delete[] pLockedBits;
    pLockedBits = nullptr;

    ZeroMemory(&CopyLocation, sizeof(CopyLocation));
    ZeroMemory(&CopyRange, sizeof(CopyRange));
}

void BlockAllocator::Initialize(UINT32 BlockCount, bool ThreadSafe)
{
    m_BlockCount = BlockCount;

    if (ThreadSafe)
    {
        InitializeCriticalSection(&m_CritSec);
    }
    else
    {
        m_CritSec.OwningThread = INVALID_HANDLE_VALUE;
    }

    m_pBlockMap = new USHORT[BlockCount];
    for (UINT32 i = 0; i < (BlockCount - 1); ++i)
    {
        m_pBlockMap[i] = (USHORT)(i + 1);
    }
    m_pBlockMap[BlockCount - 1] = NULL_BLOCK_INDEX;
    m_FirstFreeBlock = 0;
    m_FreeCount = BlockCount;
}

void BlockAllocator::Terminate()
{
    if (m_pBlockMap != nullptr)
    {
        delete[] m_pBlockMap;
        m_pBlockMap = nullptr;
    }

    m_FirstFreeBlock = NULL_BLOCK_INDEX;
    m_BlockCount = 0;
    m_FreeCount = 0;

    if (m_CritSec.OwningThread != INVALID_HANDLE_VALUE)
    {
        DeleteCriticalSection(&m_CritSec);
        m_CritSec.OwningThread = INVALID_HANDLE_VALUE;
    }
}

USHORT BlockAllocator::AllocateBlock()
{
    EnterLock();

    if (m_FirstFreeBlock == NULL_BLOCK_INDEX)
    {
        LeaveLock();
        return NULL_BLOCK_INDEX;
    }

    assert(m_FreeCount > 0);
    --m_FreeCount;

    const USHORT Index = m_FirstFreeBlock;
    assert(Index < m_BlockCount);
    m_FirstFreeBlock = m_pBlockMap[Index];
    m_pBlockMap[Index] = NULL_BLOCK_INDEX;

    LeaveLock();
    return Index;
}

void BlockAllocator::FreeBlock(USHORT BlockIndex)
{
    assert(BlockIndex < m_BlockCount);

    EnterLock();

    assert(m_FreeCount < m_BlockCount);
    ++m_FreeCount;

    assert(m_pBlockMap[BlockIndex] == NULL_BLOCK_INDEX);
    m_pBlockMap[BlockIndex] = m_FirstFreeBlock;
    m_FirstFreeBlock = BlockIndex;

    LeaveLock();
}

HRESULT BlockAllocatedDescriptorHeap::Initialize(ID3D12Device* pd3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE HeapType, bool ShaderVisible, UINT32 DescriptorHeapCount, UINT32 DescriptorBlockSize, UINT32 BlockCount)
{
    if (BlockCount >= BlockAllocator::NULL_BLOCK_INDEX)
    {
        return E_INVALIDARG;
    }

    if (DescriptorHeapCount == 0 || BlockCount == 0 || DescriptorBlockSize == 0)
    {
        return E_INVALIDARG;
    }

    m_BlockSizeDescriptors = DescriptorBlockSize;
    const UINT32 NumDescriptors = BlockCount * DescriptorBlockSize;
    m_NumDescriptorsPerHeap = NumDescriptors;
    m_DescriptorHeapCount = DescriptorHeapCount;

    HRESULT hr = m_SingleHeap.Initialize(pd3dDevice, HeapType, NumDescriptors * DescriptorHeapCount, ShaderVisible);
    if (FAILED(hr))
    {
        Terminate();
        return hr;
    }

    m_BlockAllocator.Initialize(BlockCount, true);

    return S_OK;
}

void BlockAllocatedDescriptorHeap::Terminate()
{
    m_SingleHeap.Terminate();
    m_DescriptorHeapCount = 0;
    m_NumDescriptorsPerHeap = 0;

    m_BlockAllocator.Terminate();

    m_BlockSizeDescriptors = 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE BlockAllocatedDescriptorHeap::GetCpuDescriptorHandle(USHORT BlockIndex, UINT32 HeapIndex) const
{
    assert(BlockIndex < m_BlockAllocator.GetBlockCount() && HeapIndex < m_DescriptorHeapCount);

    const UINT32 DescriptorIndex = BlockIndex * m_BlockSizeDescriptors + HeapIndex * m_NumDescriptorsPerHeap;
    return m_SingleHeap.hCPU(DescriptorIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE BlockAllocatedDescriptorHeap::GetGpuDescriptorHandle(USHORT BlockIndex, UINT32 HeapIndex) const
{
    assert(BlockIndex < m_BlockAllocator.GetBlockCount() && HeapIndex < m_DescriptorHeapCount);

    const UINT32 DescriptorIndex = BlockIndex * m_BlockSizeDescriptors + HeapIndex * m_NumDescriptorsPerHeap;
    return m_SingleHeap.hGPU(DescriptorIndex);
}

void BlockAllocatedDescriptorHeap::SetDescriptorHeaps(ID3D12GraphicsCommandList* pCmdList)
{
    ID3D12DescriptorHeap* pHeaps = m_SingleHeap;
    pCmdList->SetDescriptorHeaps(1, &pHeaps);
}
