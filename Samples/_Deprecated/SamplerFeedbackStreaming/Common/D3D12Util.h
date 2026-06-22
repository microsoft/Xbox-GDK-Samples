//--------------------------------------------------------------------------------------
// D3D12Util.h
//
// Helpful utility classes and methods for developing Direct3D 12 apps.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <windows.h>
#if defined(_XBOX_ONE) && defined(_TITLE)
#define D3D12XBOX_NO_COMPAT_TYPEDEFS 1
#include <d3d12_xs.h>
#include <d3dx12_xs.h>
#else
#include <d3d12.h>
#include "uwp\d3dx12.h"
#endif
#include <unordered_map>
#include <vector>
#include <list>
#include <queue>
#include <memory>
#include <algorithm>
#include "util.hpp"

#if !defined(_XBOX_ONE)
#define D3D12_GPU_VIRTUAL_ADDRESS_NULL ((D3D12_GPU_VIRTUAL_ADDRESS)(0))
#define D3D12XBOX_NODE_MASK (1)
typedef IUnknown IGraphicsUnknown;
typedef ID3D12GraphicsCommandList ID3D12XboxDmaCommandList;
#define D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
typedef UINT D3D12XBOX_PROCESS_DEBUG_FLAGS;
#endif

void PrepareDepthStencilBufferForRendering(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pDepthStencilTexture);
void ResourceBarrier(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, UINT32 SubresourceIndex = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
#if defined(_XBOX_ONE) && defined(_TITLE)
void ResourceBarrier(ID3D12XboxDmaCommandList* pCmdList, ID3D12Resource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After, UINT32 SubresourceIndex = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
#endif

inline void PrepareDepthStencilBufferForRendering(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pDepthStencilTexture)
{
    ResourceBarrier(pCmdList, pDepthStencilTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

bool IsBlockCompressedFormat(DXGI_FORMAT Format);

bool IsCommonStatePromotionEnabled();

class PSOCache
{
private:
    class PSODescHashFunction
    {
    public:
        std::size_t operator() (const D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSODesc) const;
    };

    class PSODescEqualFunction
    {
    public:
        bool operator() (const D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSODescA, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSODescB) const;
    };

    ID3D12Device* m_pd3dDevice;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSOTemplate;
    std::unordered_map< D3D12_GRAPHICS_PIPELINE_STATE_DESC, ID3D12PipelineState*, PSODescHashFunction, PSODescEqualFunction > m_Map;
    std::list<void*> m_Allocations;

public:
    PSOCache()
        : m_pd3dDevice(nullptr)
    { }
    ~PSOCache()
    {
        Terminate();
    }

    void* DuplicateMemory(const void* pSrc, size_t SizeBytes);

    void Initialize(ID3D12Device* pd3dDevice, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSOTemplate);
    void Terminate();

    const D3D12_GRAPHICS_PIPELINE_STATE_DESC& GetPSOTemplate() const { return m_PSOTemplate; }
    ID3D12PipelineState* FindPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSODesc);
    ID3D12PipelineState* FindOrCreatePSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSODesc);
};

class CpuGpuHeap
{
private:
    ID3D12Device* m_pd3dDevice;
    ID3D12Fence* m_pGpuFence;
    UINT64* m_pCpuFence;
    HANDLE m_hBlockEvent;

    bool m_Readback;
    SIZE_T m_DefaultSlabSizeBytes;
    UINT32 m_MaxSlabCount;

    struct HeapSlab
    {
        ID3D12Resource* pHeap;
        BYTE* pBase;
        SIZE_T CurrentOffset;
        UINT64 Fence;
        SIZE_T SizeBytes;
    };
    std::vector<HeapSlab> m_Slabs;
    UINT32 m_CurrentSlabIndex;

    WCHAR m_strName[32];

public:
    CpuGpuHeap()
        : m_pd3dDevice(nullptr),
          m_pGpuFence(nullptr),
          m_hBlockEvent(NULL)
    {
        m_strName[0] = L'\0';
    }

    void SetName(const WCHAR* strName) { wcscpy_s(m_strName, strName); }
    HRESULT Initialize(ID3D12Device* pd3dDevice, ID3D12Fence* pGpuFence, UINT64* pCpuFence, SIZE_T HeapSizeBytesPerSlab, bool Readback = false, UINT32 MaxSlabCount = -1);
    void Terminate();

    HRESULT AllocateBufferInHeap(SIZE_T SizeBytes, BYTE** ppData, D3D12_GPU_VIRTUAL_ADDRESS* pVA, UINT32 AlignmentBytes = 16);
    HRESULT CopyBufferDataToHeap(const BYTE* pData, SIZE_T SizeBytes, ID3D12Resource** ppHeap, SIZE_T& DestOffsetWithinHeap);
    HRESULT CopyBufferDataToDefaultBuffer(
        const BYTE* pData,
        SIZE_T SizeBytes,
        ID3D12GraphicsCommandList* pCommandList,
        ID3D12Resource* pDefaultResource);
#if defined(_XBOX_ONE) && defined(_TITLE)
    HRESULT CopyBufferDataToDefaultBuffer(
        const BYTE* pData,
        SIZE_T SizeBytes,
        ID3D12XboxDmaCommandList* pCommandList,
        ID3D12Resource* pDefaultResource);
#endif

    HRESULT CopyTextureSubresourceToHeap(const BYTE* pData, UINT32 DataPitchBytes, const D3D12_SUBRESOURCE_FOOTPRINT& SubresourceDesc, ID3D12Resource** ppHeap, SIZE_T& DestOffsetWithinHeap);
    HRESULT CopyTextureSubresourceToDefaultTexture(
        const BYTE* pData,
        UINT32 DataPitchBytes,
        const D3D12_SUBRESOURCE_FOOTPRINT& SubresourceDesc,
        ID3D12GraphicsCommandList* pCommandList,
        ID3D12Resource* pDefaultResource,
        UINT32 DestSubresource,
        UINT32 DestX = 0,
        UINT32 DestY = 0,
        UINT32 DestZ = 0,
        const D3D12_BOX* pSrcBox = nullptr);
#if defined(_XBOX_ONE) && defined(_TITLE)
    HRESULT CopyTextureSubresourceToDefaultTexture(
        const BYTE* pData,
        UINT32 DataPitchBytes,
        const D3D12_SUBRESOURCE_FOOTPRINT& SubresourceDesc,
        ID3D12XboxDmaCommandList* pCommandList,
        ID3D12Resource* pDefaultResource,
        UINT32 DestSubresource,
        UINT32 DestX = 0,
        UINT32 DestY = 0,
        UINT32 DestZ = 0,
        const D3D12_BOX* pSrcBox = nullptr);
#endif

    template<typename T>
    HRESULT AllocateRootConstantBuffer(T** ppStruct, ID3D12GraphicsCommandList* pCommandList, UINT32 RootParameterIndex)
    {
        T* pStruct = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS VA = 0;
        HRESULT hr = AllocateBufferInHeap(sizeof(T), (BYTE**)&pStruct, &VA, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        if (SUCCEEDED(hr))
        {
            *ppStruct = pStruct;
            pCommandList->SetGraphicsRootConstantBufferView(RootParameterIndex, VA);
        }
        return hr;
    }

    template<typename T>
    HRESULT AllocateComputeRootConstantBuffer(T** ppStruct, ID3D12GraphicsCommandList* pCommandList, UINT32 RootParameterIndex)
    {
        T* pStruct = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS VA = 0;
        HRESULT hr = AllocateBufferInHeap(sizeof(T), (BYTE**)&pStruct, &VA, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        if (SUCCEEDED(hr))
        {
            *ppStruct = pStruct;
            pCommandList->SetComputeRootConstantBufferView(RootParameterIndex, VA);
        }
        return hr;
    }

private:
    HRESULT Allocate(SIZE_T SizeBytes, SIZE_T AlignmentBytes, HeapSlab** ppSlab, SIZE_T* pOffsetWithinSlab);
    HRESULT FindAvailableSlab(HeapSlab** ppSlab, SIZE_T SizeBytes, SIZE_T AlignmentBytes);
    HRESULT CreateNewSlab(HeapSlab** ppSlab, SIZE_T RequestedSizeBytes);
};

class LinearBufferAllocator
{
private:
    volatile LONG64 m_OffsetBytes;
    SIZE_T m_SizeBytes;
    BYTE* m_pBase;
    D3D12_GPU_VIRTUAL_ADDRESS m_BaseAddress;
    ID3D12Resource* m_pResource;

public:
    LinearBufferAllocator()
        : m_pResource(nullptr),
          m_pBase(nullptr)
    { }

    HRESULT Initialize(ID3D12Device* pd3dDevice, SIZE_T SizeBytes);

    void Terminate()
    {
        m_SizeBytes = 0;
        m_OffsetBytes = 0;
        SAFE_RELEASE(m_pResource);
        m_pBase = nullptr;
        m_BaseAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
    }

    bool Allocate(SIZE_T SizeBytes, BYTE** ppBuffer, D3D12_GPU_VIRTUAL_ADDRESS* pAddress)
    {
        const SIZE_T EndOffset = (SIZE_T)InterlockedAdd64(&m_OffsetBytes, SizeBytes);
        if (EndOffset <= m_SizeBytes)
        {
            const SIZE_T StartOffset = EndOffset - SizeBytes;
            *ppBuffer = m_pBase + StartOffset;
            *pAddress = m_BaseAddress + StartOffset;
            return true;
        }
        else
        {
            return false;
        }
    }

    template<typename T>
    bool Allocate(T** ppObject, D3D12_GPU_VIRTUAL_ADDRESS* pAddress)
    {
        return Allocate(sizeof(T), (BYTE**)ppObject, pAddress);
    }

    bool AllocateFast(SIZE_T SizeBytes, BYTE** ppBuffer, D3D12_GPU_VIRTUAL_ADDRESS* pAddress)
    {
        const SIZE_T EndOffset = (SIZE_T)(m_OffsetBytes + SizeBytes);
        m_OffsetBytes += SizeBytes;
        if (EndOffset <= m_SizeBytes)
        {
            const SIZE_T StartOffset = EndOffset - SizeBytes;
            *ppBuffer = m_pBase + StartOffset;
            *pAddress = m_BaseAddress + StartOffset;
            return true;
        }
        else
        {
            return false;
        }
    }

    template<typename T>
    bool AllocateFast(T** ppObject, D3D12_GPU_VIRTUAL_ADDRESS* pAddress)
    {
        return AllocateFast(sizeof(T), (BYTE**)ppObject, pAddress);
    }

    void Reset()
    {
        m_OffsetBytes = 0;
    }
};

struct D3D12_MAPPED_SUBRESOURCE
{
    void *pData;
    UINT RowPitch;
    UINT DepthPitch;
};

class D3D12DynamicBuffer
{
private:
    struct RenameBuffer
    {
        ID3D12Resource* pBuffer;
        UINT64 WriteFence;
    };
    std::vector<RenameBuffer> m_RenameBuffers;
    UINT32 m_CurrentBufferIndex;
    UINT32 m_ByteWidth;
    UINT32 m_VertexStrideBytes;
    ID3D12Device* m_pd3dDevice;
    ID3D12Fence* m_pGpuFence;
    UINT64* m_pCpuFence;

public:
    D3D12DynamicBuffer()
        : m_CurrentBufferIndex(0),
          m_pd3dDevice(nullptr),
          m_pGpuFence(nullptr)
    {}
    ~D3D12DynamicBuffer()
    {
        Terminate();
    }

    HRESULT Create(ID3D12Device* pd3dDevice, ID3D12Fence* pGpuFence, UINT64* pCpuFence, 
                   UINT32 ByteWidth, UINT32 VertexStrideBytes, UINT32 InitialRenameCount);
    void Terminate();

    HRESULT MapDiscard(ID3D12CommandQueue* pd3dCmdQueue, D3D12_MAPPED_SUBRESOURCE* pMapData);
    HRESULT MapNoOverwrite(ID3D12CommandQueue* pd3dCmdQueue, D3D12_MAPPED_SUBRESOURCE* pMapData);
    HRESULT Unmap(ID3D12CommandQueue* pd3dCmdQueue);
    HRESULT CopyDiscard(ID3D12CommandQueue* pd3dCmdQueue, const void* pSrc, SIZE_T SrcSizeBytes);

    template<typename T>
    HRESULT MapDiscardTyped(ID3D12CommandQueue* pd3dCmdQueue, T** ppMappedStruct)
    {
        D3D12_MAPPED_SUBRESOURCE MapData = {};
        HRESULT hr = MapDiscard(pd3dCmdQueue, &MapData);
        if (SUCCEEDED(hr))
        {
            *ppMappedStruct = (T*)MapData.pData;
        }
        else
        {
            *ppMappedStruct = nullptr;
        }
        return hr;
    }

    UINT32 GetRenameBufferCount() const { return (UINT32)m_RenameBuffers.size(); }
    ID3D12Resource* GetBufferByIndex(UINT32 Index) const { return m_RenameBuffers[Index].pBuffer; }

    UINT32 GetCurrentRenameBufferIndex() const { return m_CurrentBufferIndex; }
    ID3D12Resource* GetBuffer() const { return m_RenameBuffers[m_CurrentBufferIndex].pBuffer; }

    void SetIB(ID3D12GraphicsCommandList* pCmdList, DXGI_FORMAT IndexFormat);
    void SetVB(ID3D12GraphicsCommandList* pCmdList, UINT32 StartSlot = 0);
    void SetGraphicsRootCB(ID3D12GraphicsCommandList* pCmdList, UINT32 RootParameterIndex);
    void SetComputeRootCB(ID3D12GraphicsCommandList* pCmdList, UINT32 RootParameterIndex);
    void CreateCBView(D3D12_CPU_DESCRIPTOR_HANDLE DestHandle);
    void CreateSBView(D3D12_CPU_DESCRIPTOR_HANDLE DestHandle);

    void GetVBDesc(D3D12_VERTEX_BUFFER_VIEW* pVBDesc) const;
    void GetCBDesc(D3D12_CONSTANT_BUFFER_VIEW_DESC* pCBDesc) const;
    void GetIBDesc(D3D12_INDEX_BUFFER_VIEW* pIBDesc, DXGI_FORMAT IndexFormat) const;
    void GetSBDesc(D3D12_SHADER_RESOURCE_VIEW_DESC* pSBDesc) const;

private:
    UINT32 FindOrCreateRenameBuffer( ID3D12CommandQueue* pCmdQueue, bool ForceCreate );
};

class DescriptorHeapWrapper
{
public:
    DescriptorHeapWrapper(ID3D12DescriptorHeap* pDH = nullptr)
        : m_pDH(nullptr)
    { 
        if (pDH != nullptr)
        {
            InitializeFromExistingHeap(pDH);
        }
    }
    ~DescriptorHeapWrapper()
    {
        Terminate();
    }

    HRESULT InitializeFromExistingHeap(ID3D12DescriptorHeap* pDH)
    {
        SAFE_RELEASE(m_pDH);
        m_pDH = pDH;
        m_pDH->AddRef();

        ID3D12Device* pd3dDevice = nullptr;
        pDH->GetDevice(__uuidof(ID3D12Device), (void**)&pd3dDevice);
        m_Desc = pDH->GetDesc();
        m_HandleIncrementSize = pd3dDevice->GetDescriptorHandleIncrementSize(m_Desc.Type);
        SAFE_RELEASE(pd3dDevice);

        m_hCPUHeapStart = pDH->GetCPUDescriptorHandleForHeapStart();
        if (m_Desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
        {
            m_hGPUHeapStart = pDH->GetGPUDescriptorHandleForHeapStart();
        }
        else
        {
            m_hGPUHeapStart.ptr = 0;
        }

        return S_OK;
    }

    HRESULT Initialize(
        ID3D12Device* pDevice,
        D3D12_DESCRIPTOR_HEAP_TYPE Type,
        UINT NumDescriptors,
        bool bShaderVisible = false)
    {
        m_Desc.Type = Type;
        m_Desc.NumDescriptors = NumDescriptors;
        m_Desc.Flags = bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        m_Desc.NodeMask = 0;

        HRESULT hr = pDevice->CreateDescriptorHeap(&m_Desc,
                                                   __uuidof(ID3D12DescriptorHeap),
                                                   (void**)&m_pDH);
        if (FAILED(hr)) return hr;

        m_hCPUHeapStart = m_pDH->GetCPUDescriptorHandleForHeapStart();
        if (bShaderVisible)
        {
            m_hGPUHeapStart = m_pDH->GetGPUDescriptorHandleForHeapStart();
        }
        else
        {
            m_hGPUHeapStart.ptr = 0;
        }
        m_HandleIncrementSize = pDevice->GetDescriptorHandleIncrementSize(m_Desc.Type);
        return hr;
    }

    void Terminate()
    {
        SAFE_RELEASE(m_pDH);
    }

    operator ID3D12DescriptorHeap*() { return m_pDH; }

    D3D12_CPU_DESCRIPTOR_HANDLE hCPU(UINT index) const
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE DH(m_hCPUHeapStart);
        DH.Offset(index, m_HandleIncrementSize);
        return DH;
    }
    D3D12_GPU_DESCRIPTOR_HANDLE hGPU(UINT index) const
    {
        assert(m_Desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        CD3DX12_GPU_DESCRIPTOR_HANDLE DH(m_hGPUHeapStart);
        DH.Offset(index, m_HandleIncrementSize);
        return DH;
    }

    UINT32 GetIncrementSize() const { return m_HandleIncrementSize; }

private:
    D3D12_DESCRIPTOR_HEAP_DESC m_Desc;
    ID3D12DescriptorHeap* m_pDH;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCPUHeapStart;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGPUHeapStart;
    UINT32 m_HandleIncrementSize;
};

class DescriptorHeapSetManager
{
private:
    ID3D12Device* m_pd3dDevice;
    ID3D12CommandQueue* m_pQueue;
    ID3D12Fence* m_pGpuFence;
    UINT64* m_pCpuFence;

    struct HandleSet
    {
        UINT64 Fence;
        UINT32 HeapOffset;
    };
    std::vector<HandleSet> m_Sets;
    UINT32 m_LastSetIndex;

    ID3D12DescriptorHeap* m_pHeap;
    D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
    D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
    UINT32 m_HandleSize;
    UINT32 m_BaseHeapOffset;
    UINT32 m_HeapStride;

    struct ManagedBuffer
    {
        void* pResource;
        bool DynamicBuffer;
        union
        {
            DXGI_FORMAT IndexFormat;
            UINT32 OriginalHeapIndex;
        };
    };
    std::vector<ManagedBuffer> m_Buffers;

public:
    HRESULT Initialize(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pQueue, ID3D12Fence* pGpuFence, UINT64* pCpuFence, D3D12_DESCRIPTOR_HEAP_TYPE HeapType, ID3D12DescriptorHeap* pHeap, UINT32 HeapOffset, UINT32 HeapStride, UINT32 MaxHandleCount);
    void Terminate();

    bool AddDynamicCB(D3D12DynamicBuffer* pBuffer);
    bool AddEmptySlot();
    bool AddStaticDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* pHandle);
    bool AddStaticSRV(ID3D12Resource* pResource, D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc);

    void FinalizeDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE* pGpuHandle, D3D12_CPU_DESCRIPTOR_HANDLE* pCpuHandle, bool DynamicBuffersAsStructuredBuffers = false);
    void SetGraphicsRootDescriptorTable(ID3D12GraphicsCommandList* pCmdList, UINT32 RootParamIndex)
    {
        assert(m_HeapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle;
        FinalizeDescriptorTable(&GpuHandle, nullptr);
        pCmdList->SetGraphicsRootDescriptorTable(RootParamIndex, GpuHandle);
    }

    void SetGraphicsRootConstantBufferView(ID3D12GraphicsCommandList* pCmdList, UINT32 RootParamIndex)
    {
        assert(m_HeapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
        FinalizeDescriptorTable(nullptr, &cpuHandle);

#if defined(_XBOX_ONE) && defined(_TITLE)
        const UINT64  ptr = *(const UINT64*)cpuHandle.GetCachedReadPointerX();
#else
        const UINT64  ptr = (UINT64)cpuHandle.ptr;
#endif

        pCmdList->SetGraphicsRootConstantBufferView(RootParamIndex, ptr);
    }

    void SetGraphicsRootShaderResourceView(ID3D12GraphicsCommandList* pCmdList, UINT32 RootParamIndex)
    {
        assert(m_HeapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle;
        FinalizeDescriptorTable(nullptr, &cpuHandle);

#if defined(_XBOX_ONE) && defined(_TITLE)
        const UINT64  ptr = *(const UINT64*)cpuHandle.GetCachedReadPointerX();
#else
        const UINT64  ptr = (UINT64)cpuHandle.ptr;
#endif

        pCmdList->SetGraphicsRootShaderResourceView(RootParamIndex, ptr);
    }

    void SetIndexBuffer(ID3D12GraphicsCommandList* pCmdList)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
        FinalizeDescriptorTable(nullptr, &CpuHandle);
        if (m_Buffers.size() != 0)
        {
            D3D12_INDEX_BUFFER_VIEW* pView = (D3D12_INDEX_BUFFER_VIEW*)CpuHandle.ptr;
            pCmdList->IASetIndexBuffer(pView);
        }
    }
    void SetVertexBuffers(ID3D12GraphicsCommandList* pCmdList, UINT32 StartSlot)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
        FinalizeDescriptorTable(nullptr, &CpuHandle);
        D3D12_VERTEX_BUFFER_VIEW* pViews = (D3D12_VERTEX_BUFFER_VIEW*)CpuHandle.ptr;
        UINT32 Count = (UINT32)m_Buffers.size();
        for (UINT i = 0; i < Count; ++i)
        {
            pCmdList->IASetVertexBuffers(StartSlot, 1, pViews);
            ++StartSlot;
            pViews = (D3D12_VERTEX_BUFFER_VIEW*)((BYTE*)pViews + m_HandleSize);
        }
    }

protected:
    HandleSet* FindUnusedSet();
    ManagedBuffer* AddBuffer();
    void PrepareDescriptor(const ManagedBuffer& MB, UINT32 HeapIndex, bool DynamicBuffersAsStructuredBuffers) const;
};

class DDSLoader12
{
private:
    ID3D12Device* m_pd3dDevice;
    ID3D12CommandAllocator* m_pCmdAlloc;
    ID3D12GraphicsCommandList* m_pCmdList;
    CpuGpuHeap* m_pUploadHeap;

    std::vector<D3D12_RESOURCE_BARRIER> m_FinalDescs;

public:
    DDSLoader12()
        : m_pd3dDevice(nullptr),
          m_pCmdAlloc(nullptr),
          m_pCmdList(nullptr),
          m_pUploadHeap(nullptr)
    { };
    ~DDSLoader12()
    {
        Terminate();
    }

    HRESULT Initialize(ID3D12Device* pd3dDevice, ID3D12CommandAllocator* pCmdAlloc, CpuGpuHeap* pUploadHeap);
    void Terminate();

    void BeginLoading(ID3D12GraphicsCommandList* pExistingCmdList = nullptr);
    void FinishLoading(ID3D12CommandQueue* pQueue);

    HRESULT LoadDDSFile(_In_z_ const WCHAR* strFileName, D3D12_CPU_DESCRIPTOR_HANDLE DescHandle, _COM_Outptr_ ID3D12Resource** ppTexture, _In_opt_ D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE);
    HRESULT LoadDDSFromMemory(_In_z_ const uint8_t* ddsData, size_t ddsDataSize, D3D12_CPU_DESCRIPTOR_HANDLE DescHandle, _COM_Outptr_ ID3D12Resource** ppTexture, _In_opt_ D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE);
};

inline HRESULT CreateDefaultResource(
    ID3D12Device* pDevice, 
    D3D12_RESOURCE_DESC* pDesc, 
    D3D12_RESOURCE_STATES InitialState, 
    void** ppResource,
    DXGI_FORMAT FastClearFormat = DXGI_FORMAT_UNKNOWN,
    void* pPlacementMemory = nullptr,
    bool AllowDisplay = false)
{
    D3D12_HEAP_PROPERTIES HeapProperties = {};
    HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.CreationNodeMask = D3D12XBOX_NODE_MASK;
    HeapProperties.VisibleNodeMask = D3D12XBOX_NODE_MASK;

    D3D12_CLEAR_VALUE ClearValue;
    ZeroMemory(&ClearValue, sizeof(D3D12_CLEAR_VALUE));
    if (FastClearFormat == DXGI_FORMAT_UNKNOWN)
    {
        ClearValue.Format = pDesc->Format;
    }
    else
    {
        ClearValue.Format = FastClearFormat;
    }

    if (pDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
        ClearValue.DepthStencil.Depth = 1.0f;
    }

    D3D12_CLEAR_VALUE* pClearValue = nullptr;
    if (pDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ||
        pDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
        pClearValue = &ClearValue;
    }

#if defined(_XBOX_ONE) && defined(_TITLE)
    if (pPlacementMemory != nullptr)
    {
        return pDevice->CreatePlacedResourceX(
            (D3D12_GPU_VIRTUAL_ADDRESS)pPlacementMemory,
            pDesc,
            InitialState,
            pClearValue,
            __uuidof(ID3D12Resource),
            ppResource);
    }
    else
#endif
    {
        D3D12_HEAP_FLAGS HeapFlags = AllowDisplay ? D3D12_HEAP_FLAG_ALLOW_DISPLAY : D3D12_HEAP_FLAG_NONE;

        return pDevice->CreateCommittedResource(&HeapProperties,
            HeapFlags,
            pDesc,
            InitialState,
            pClearValue,
            __uuidof(ID3D12Resource),
            ppResource);
    }
}

HRESULT CreateDefaultBuffer(
    ID3D12Device* pd3dDevice,
    CpuGpuHeap* pUploadHeap,
    ID3D12GraphicsCommandList* pCmdList,
    SIZE_T SizeBytes,
    const void* pData,
    ID3D12Resource** ppBuffer,
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

HRESULT CreateDefaultIndexBuffer(
    ID3D12Device* pd3dDevice,
    CpuGpuHeap* pUploadHeap,
    ID3D12GraphicsCommandList* pCmdList,
    SIZE_T SizeBytes,
    DXGI_FORMAT IBFormat,
    const void* pData,
    ID3D12Resource** ppBuffer,
    D3D12_INDEX_BUFFER_VIEW* pIBView);

HRESULT CreateDefaultVertexBuffer(
    ID3D12Device* pd3dDevice,
    CpuGpuHeap* pUploadHeap,
    ID3D12GraphicsCommandList* pCmdList,
    SIZE_T SizeBytes,
    SIZE_T StrideBytes,
    const void* pData,
    ID3D12Resource** ppBuffer,
    D3D12_VERTEX_BUFFER_VIEW* pVBView);

HRESULT CreateDefaultConstantBuffer(
    ID3D12Device* pd3dDevice,
    CpuGpuHeap* pUploadHeap,
    ID3D12GraphicsCommandList* pCmdList,
    SIZE_T SizeBytes,
    const void* pData,
    ID3D12Resource** ppBuffer,
    D3D12_CPU_DESCRIPTOR_HANDLE hCBHandle);

HRESULT CreateStagingBufferForTexture(
    ID3D12Device* pd3dDevice,
    const D3D12_RESOURCE_DESC* pTextureDesc,
    bool Readback,
    ID3D12Resource** ppBuffer,
    D3D12_TEXTURE_COPY_LOCATION* pStagingCopyLocation,
    D3D12_RANGE* pEntireRange);

inline UINT32 GetSubresourceCount(const D3D12_RESOURCE_DESC& Desc)
{
    UINT32 MipCount = Desc.MipLevels;
    UINT32 SliceCount = (Desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? 1 : Desc.DepthOrArraySize;
    UINT32 MaxDimension = 1;
    if (MipCount == 0)
    {
        switch (Desc.Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            MipCount = 1;
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            MaxDimension = (UINT32)Desc.Width;
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            MaxDimension = std::max((UINT32)Desc.Width, Desc.Height);
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            MaxDimension = std::max(std::max((UINT32)Desc.Width, Desc.Height), (UINT32)Desc.DepthOrArraySize);
            break;
        }
        if (MipCount == 0)
        {
            while (MaxDimension >= 1)
            {
                ++MipCount;
                MaxDimension /= 2;
            }
        }
    }
    return MipCount * SliceCount;
}

HRESULT CreateRootSignature(ID3D12Device* pd3dDevice, 
                            const D3D12_ROOT_SIGNATURE_DESC* pDesc, 
                            ID3D12RootSignature** ppRootSignature);

class DeferredReleaseQueue
{
private:
    struct QueueEntry
    {
        IGraphicsUnknown* pObject;
        UINT64 FenceValue;
    };
    typedef std::deque<QueueEntry> ReleaseQueue;

    ReleaseQueue m_Queue;

public:
    void DeferredRelease(IGraphicsUnknown* pUnk, UINT64 CurrentFenceValue)
    {
        if (pUnk == nullptr)
        {
            return;
        }

        QueueEntry Entry = {};
        Entry.pObject = pUnk;
        Entry.FenceValue = CurrentFenceValue;
        m_Queue.push_back(Entry);
    }

    void ReleaseNow(ID3D12Fence* pFence)
    {
        if (m_Queue.empty())
        {
            return;
        }
        const UINT64 CompletedValue = pFence->GetCompletedValue();
        while (!m_Queue.empty())
        {
            QueueEntry& E = m_Queue.front();
            if (E.FenceValue > CompletedValue)
            {
                break;
            }
            SAFE_RELEASE(E.pObject);
            m_Queue.pop_front();
        }
    }
};

inline void CopySubresource(ID3D12GraphicsCommandList* pCmdList, 
                            ID3D12Resource* pDestResource,
                            UINT32 DestSubresourceIndex,
                            ID3D12Resource* pSrcResource,
                            UINT32 SrcSubresourceIndex)
{
    CD3DX12_TEXTURE_COPY_LOCATION Destination(pDestResource, DestSubresourceIndex);
    CD3DX12_TEXTURE_COPY_LOCATION Source(pSrcResource, SrcSubresourceIndex);
    pCmdList->CopyTextureRegion(&Destination, 0, 0, 0, &Source, nullptr);
}

class GpuIntervalTimer
{
private:
    static const UINT64 m_TimestampBufferDepth = 32;

    UINT64* m_pRawTimestamps;
    UINT64* m_pDeltaTimestamps;

    struct TimingInterval
    {
        UINT64 AverageTicks;
        WCHAR strName[64];
    };
    std::vector<TimingInterval> m_Intervals;
    UINT32 m_MaxIntervalCount;

    UINT64 m_FrameIndex;
    UINT64 m_CurrentIntervalIndex;

public:
    void Initialize(UINT32 MaxIntervals = 255);
    void AddInterval(const WCHAR* strName);
    void Terminate();

    void NextFrame();
    void MarkTimestamp(ID3D12GraphicsCommandList* pCmdList);

    UINT32 GetIntervalCount() const { return (UINT32)m_Intervals.size(); }
    void GetInterval(UINT32 Index, const WCHAR** ppName, FLOAT* pTimeUsec) const;

private:
    UINT32 GetTimestampIndex(UINT32 IntervalIndex) const
    {
        return (IntervalIndex * m_TimestampBufferDepth) + (m_FrameIndex % m_TimestampBufferDepth);
    }
    D3D12_GPU_VIRTUAL_ADDRESS GetTimestampAddress(UINT32 IntervalIndex) const
    {
        return (D3D12_GPU_VIRTUAL_ADDRESS)(m_pRawTimestamps + GetTimestampIndex(IntervalIndex));
    }
};
