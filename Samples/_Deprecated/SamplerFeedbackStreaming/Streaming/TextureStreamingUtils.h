//--------------------------------------------------------------------------------------
// TextureStreamingUtils.h
//
// Utility classes used within the tile streaming engine.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <windows.h>
#if defined(_GAMING_XBOX)
#include <d3d12_xs.h>
#if XGS_BUILD
#include <pix3.h>
#else
#include <pix.h>
#endif
#else
#include <d3d12.h>
#include <pix.h>
#endif

struct StagingTexture
{
    ID3D12Resource* pStagingBuffer;
    void* pLockedBits;
    D3D12_TEXTURE_COPY_LOCATION CopyLocation;
    D3D12_RANGE CopyRange;

    bool CreateCpuOnlyStaging(ID3D12Device* pd3dDevice, const D3D12_RESOURCE_DESC& TextureDesc);
    void DeleteCpuOnlyStaging();

    void* Map()
    {
        if (pStagingBuffer != nullptr)
        {
            HRESULT hr = pStagingBuffer->Map(0, nullptr, &pLockedBits);
            return SUCCEEDED(hr) ? pLockedBits : nullptr;
        }
        return pLockedBits;
    }

    void Unmap()
    {
        if (pStagingBuffer != nullptr)
        {
            pStagingBuffer->Unmap(0, nullptr);
            pLockedBits = nullptr;
        }
    }
};

class BlockAllocator
{
private:
    CRITICAL_SECTION m_CritSec;
    UINT32 m_BlockCount;
    USHORT* m_pBlockMap;
    USHORT m_FirstFreeBlock;

    UINT32 m_FreeCount;

public:
    static const USHORT NULL_BLOCK_INDEX = -1;

    BlockAllocator()
        : m_pBlockMap(nullptr),
        m_BlockCount(0),
        m_FreeCount(0),
        m_FirstFreeBlock(NULL_BLOCK_INDEX)
    {
        m_CritSec.OwningThread = INVALID_HANDLE_VALUE;
    }

    void Initialize(UINT32 BlockCount, bool ThreadSafe);
    void Terminate();

    UINT32 GetBlockCount() const { return m_BlockCount; }
    UINT32 GetFreeCount() const { return m_FreeCount; }
    UINT32 GetAllocatedCount() const { return GetBlockCount() - GetFreeCount(); }

    USHORT AllocateBlock();
    void FreeBlock(USHORT BlockIndex);

private:
    void EnterLock()
    {
        if (m_CritSec.OwningThread != INVALID_HANDLE_VALUE)
        {
            EnterCriticalSection(&m_CritSec);
        }
    }

    void LeaveLock()
    {
        if (m_CritSec.OwningThread != INVALID_HANDLE_VALUE)
        {
            LeaveCriticalSection(&m_CritSec);
        }
    }
};

class BlockAllocatedDescriptorHeap
{
private:
    UINT32 m_BlockSizeDescriptors;

    BlockAllocator m_BlockAllocator;

    DescriptorHeapWrapper m_SingleHeap;
    UINT32 m_DescriptorHeapCount;
    UINT32 m_NumDescriptorsPerHeap;

public:
    BlockAllocatedDescriptorHeap()
    {
        m_DescriptorHeapCount = 0;
        m_BlockSizeDescriptors = 0;
        m_NumDescriptorsPerHeap = 0;
    }

    HRESULT Initialize(ID3D12Device* pd3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE HeapType, bool ShaderVisible, UINT32 DescriptorHeapCount, UINT32 DescriptorBlockSize, UINT32 BlockCount);
    void Terminate();

    USHORT AllocateBlock() { return m_BlockAllocator.AllocateBlock(); }
    void FreeBlock(USHORT BlockIndex) { m_BlockAllocator.FreeBlock(BlockIndex); }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle(USHORT BlockIndex, UINT32 HeapIndex = 0) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(USHORT BlockIndex, UINT32 HeapIndex = 0) const;

    UINT32 GetIncrementSize() const { return m_SingleHeap.GetIncrementSize(); }
    UINT32 GetBlockSize() const { return m_BlockSizeDescriptors; }

    void SetDescriptorHeaps(ID3D12GraphicsCommandList* pCmdList);
    ID3D12DescriptorHeap* GetDescriptorHeap() { return m_SingleHeap; }
};

class LockTable
{
private:
    static const UINT32 m_LockCount = 37;
    CRITICAL_SECTION m_CritSecs[m_LockCount];

public:
    void Initialize()
    {
        for (UINT32 i = 0; i < m_LockCount; ++i)
        {
            InitializeCriticalSection(&m_CritSecs[i]);
        }
    }
    void Terminate()
    {
        for (UINT32 i = 0; i < m_LockCount; ++i)
        {
            DeleteCriticalSection(&m_CritSecs[i]);
        }
    }

    void EnterLock(void* pObject)
    {
        EnterCriticalSection(&m_CritSecs[HashObjectPointer(pObject)]);
    }
    void LeaveLock(void* pObject)
    {
        LeaveCriticalSection(&m_CritSecs[HashObjectPointer(pObject)]);
    }

private:
    UINT32 HashObjectPointer(void* pObject) const
    {
        return (UINT32)((UINT64)pObject % m_LockCount);
    }
};

class AsynchronousLoadOperation
{
private:
    OVERLAPPED m_Ovl;
    HANDLE m_hCompletionEvent;
    BYTE* m_pReadDestination;
    UINT32 m_ReadSizeBytes;
    bool m_IsComplete;
    void* m_pContext;
    LARGE_INTEGER m_StartTime;

public:
    void Initialize()
    {
        m_hCompletionEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        m_pReadDestination = nullptr;
        m_ReadSizeBytes = 0;
        m_pContext = nullptr;
        m_IsComplete = true;
    }

    void Terminate()
    {
        CloseHandle(m_hCompletionEvent);
        m_hCompletionEvent = NULL;
    }

    const BYTE* GetBuffer() const { return m_pReadDestination; }
    SIZE_T GetBufferSizeBytes() const { return m_ReadSizeBytes; }

    bool IsAssignedToRead() const { return m_pReadDestination != nullptr; }

    bool IsComplete()
    {
        if (m_IsComplete)
        {
            return true;
        }
        m_IsComplete = ((DWORD)(m_Ovl.Internal) != STATUS_PENDING);
        return m_IsComplete;
    }

    UINT64 GetStartTicks() const { return m_StartTime.QuadPart; }

    void Reset()
    {
        m_IsComplete = false;
        m_pReadDestination = nullptr;
        m_ReadSizeBytes = 0;
        m_pContext = nullptr;
    }

    bool BeginRead(HANDLE hFile, UINT64 FileOffsetBytes, UINT32 SizeBytes, BYTE* pDestination, void* pContext, LARGE_INTEGER StartTime)
    {
        if (IsAssignedToRead())
        {
            return false;
        }

        ResetEvent(m_hCompletionEvent);

        ZeroMemory(&m_Ovl, sizeof(m_Ovl));
        m_Ovl.hEvent = m_hCompletionEvent;
        m_Ovl.Pointer = (void*)FileOffsetBytes;
        m_pReadDestination = pDestination;
        m_ReadSizeBytes = SizeBytes;
        m_IsComplete = false;
        m_pContext = pContext;
        m_StartTime = StartTime;

        BOOL Success = ReadFile(hFile, m_pReadDestination, m_ReadSizeBytes, nullptr, &m_Ovl);
        if (!Success)
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                Success = TRUE;
            }
            else
            {
                Reset();
            }
        }

        return Success == TRUE;
    }

    HANDLE GetCompletionEvent() { return m_hCompletionEvent; }
    void* GetContext() const { return m_pContext; }
};

struct LabeledCpuProbe
{
    UINT64 TickCount;
    const CHAR* strLabel;
    UINT64 Divisor;
};

class CpuProbeWriter
{
private:
    LabeledCpuProbe* m_pCurrentProbe;
    UINT32 m_ProbesRemaining;

public:
    CpuProbeWriter(LabeledCpuProbe* pProbes, UINT32 ProbeCount)
    {
        m_pCurrentProbe = pProbes;
        m_ProbesRemaining = ProbeCount;
        WriteProbe(nullptr);
    }

    UINT32 GetProbesRemaining() const { return m_ProbesRemaining; }

    void WriteProbe(const CHAR* strLabel, UINT64 Divisor = 0)
    {
        assert(m_ProbesRemaining > 0);
        LARGE_INTEGER Timestamp;
        QueryPerformanceCounter(&Timestamp);
        m_pCurrentProbe->TickCount = Timestamp.QuadPart;
        m_pCurrentProbe->strLabel = strLabel;
        m_pCurrentProbe->Divisor = Divisor;
        --m_ProbesRemaining;
        ++m_pCurrentProbe;
    }
};

#if 0

class PixCmdQueueScopedBlock
{
private:
    ID3D12CommandQueue* m_pCmdQueue;

public:
    PixCmdQueueScopedBlock(ID3D12CommandQueue* pCmdQueue, const WCHAR* strName)
    {
        PIXBeginEvent(pCmdQueue, PIX_COLOR_DEFAULT, strName);
        m_pCmdQueue = pCmdQueue;
    }

    ~PixCmdQueueScopedBlock()
    {
        PIXEndEvent(m_pCmdQueue);
    }
};
#define PIX_CMDQUEUE_BLOCK(cq,x) PixCmdQueueScopedBlock Pix(cq,x)

class PixCmdListScopedBlock
{
private:
    ID3D12GraphicsCommandList* m_pCmdList;

public:
    PixCmdListScopedBlock(ID3D12GraphicsCommandList* pCmdList, const WCHAR* strName)
    {
        PIXBeginEvent(pCmdList, PIX_COLOR_DEFAULT, strName);
        m_pCmdList = pCmdList;
    }

    ~PixCmdListScopedBlock()
    {
        PIXEndEvent(m_pCmdList);
    }
};
#define PIX_CMDLIST_BLOCK(cl,x) PixCmdListScopedBlock Pix(cl,x)

#else

#define PIX_CMDQUEUE_BLOCK(cq,x)
#define PIX_CMDLIST_BLOCK(cl,x)

#endif

class MinMaxAverage
{
public:
    struct Report
    {
        UINT64 MinValue;
        UINT64 MaxValue;
        UINT64 AverageValue;
    };

private:
    Report m_Report;
    UINT64 m_WindowValues[100];
    UINT64 m_SampleCount;

public:
    MinMaxAverage()
    {
        Reset();
    }

    void Reset()
    {
        m_Report.MinValue = -1;
        m_Report.MaxValue = 0;
        m_Report.AverageValue = 0;
        m_SampleCount = 0;
    }

    void AddSample(UINT64 Value)
    {
        m_Report.MinValue = std::min(m_Report.MinValue, Value);
        m_Report.MaxValue = std::max(m_Report.MaxValue, Value);
        m_WindowValues[m_SampleCount % ARRAYSIZE(m_WindowValues)] = Value;
        ++m_SampleCount;
    }

    Report GenerateReport()
    {
        Report Result = {};
        if (m_SampleCount > 0)
        {
            const UINT64 WindowSize = std::min(ARRAYSIZE(m_WindowValues), m_SampleCount);
            UINT64 Sum = 0;
            for (UINT64 i = 0; i < WindowSize; ++i)
            {
                Sum += m_WindowValues[i];
            }
            m_Report.AverageValue = Sum / WindowSize;
            Result = m_Report;
        }
        return Result;
    }
};

class ScopedCritSec
{
private:
    CRITICAL_SECTION* m_pCritSec;
    DWORD* m_pOwningThreadId;

public:
    ScopedCritSec(CRITICAL_SECTION* pCritSec, DWORD* pOwningThreadId = nullptr)
        : m_pCritSec(pCritSec),
        m_pOwningThreadId(pOwningThreadId)
    {
        EnterCriticalSection(pCritSec);
        if (pOwningThreadId != nullptr)
        {
            *pOwningThreadId = GetCurrentThreadId();
        }
    }

    ~ScopedCritSec()
    {
        if (m_pOwningThreadId != nullptr)
        {
            *m_pOwningThreadId = 0;
        }
        LeaveCriticalSection(m_pCritSec);
        m_pCritSec = nullptr;
    }
};

__forceinline
UINT8 FloorLog2(UINT32 v)
{
    if (v == 0)
    {
        return 0;
    }

    unsigned long result = 0;

    _BitScanReverse(&result, v);

    return UINT8(result);
}

static const FLOAT g_FilterOffsetValues[] =
{
    1.0f / 4.0f,
    1.0f / 8.0f,
    1.0f / 16.0f,
    1.0f / 32.0f,
    1.0f / 64.0f,
    1.0f / 128.0f,
};

static const WCHAR* g_strFilterOffsetValues[] =
{
    L"1/4",
    L"1/8",
    L"1/16",
    L"1/32",
    L"1/64",
    L"1/128",
};
C_ASSERT(ARRAYSIZE(g_FilterOffsetValues) == ARRAYSIZE(g_strFilterOffsetValues));

static const FLOAT g_FilterSlopeValues[] =
{
    2.5f,
    3.0f,
    4.0f,
    5.0f,
    8.0f,
    16.0f,
    32.0f,
    64.0f
};

static const WCHAR* g_strFilterSlopeValues[] =
{
    L"2.5",
    L"3.0",
    L"4.0",
    L"5.0",
    L"8.0",
    L"16.0",
    L"32.0",
    L"64.0"
};
C_ASSERT(ARRAYSIZE(g_FilterSlopeValues) == ARRAYSIZE(g_strFilterSlopeValues));
