//--------------------------------------------------------------------------------------
// TextureStreaming.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "precomp.hpp"
#include "TextureStreaming.h"
#include "JobQueue.h"

#if XBOX_SAMPLER_FEEDBACK
#include "CSFeedbackResolve.cfxhpp"
#endif

#pragma warning (disable: 4456)

StreamingTextureManager g_StreamingTextureManager;

#if XBOX_SAMPLER_FEEDBACK
static const DXGI_FORMAT g_FeedbackElementFormat = DXGI_FORMAT_R8_UINT;
static const UINT32 g_FeedbackElementSizeBytes = sizeof(BYTE);
typedef BYTE FeedbackElementType;
#else
static const DXGI_FORMAT g_FeedbackElementFormat = DXGI_FORMAT_R32_UINT;
static const UINT32 g_FeedbackElementSizeBytes = sizeof(UINT32);
typedef UINT32 FeedbackElementType;
#endif

inline void AddLinkedTexture(LinkedStreamingTextureVector* pVector, UINT32 MapWidth, UINT32 MapHeight, StreamingTexture* pTexture)
{
    const UINT32 TextureTileWidth = pTexture->FileHeader.MipLevels[0].WidthTilesM1 + 1;
    const UINT32 TextureTileHeight = pTexture->FileHeader.MipLevels[0].HeightTilesM1 + 1;

    LinkedStreamingTexture Link = {};
    Link.pTexture = pTexture;
    Link.FirstNonStreamingMipFractionalLOD = pTexture->FirstNonStreamingMipFractionalLOD;

    if (TextureTileWidth >= MapWidth)
    {
        UINT32 WidthDivisor = TextureTileWidth / MapWidth;
        Link.ScaleShiftX = FloorLog2(WidthDivisor);
    }
    else
    {
        UINT32 WidthDivisor = MapWidth / TextureTileWidth;
        Link.ScaleShiftX = -1 * (INT32)FloorLog2(WidthDivisor);
    }

    if (TextureTileHeight >= MapHeight)
    {
        UINT32 HeightDivisor = TextureTileHeight / MapHeight;
        Link.ScaleShiftY = FloorLog2(HeightDivisor);
    }
    else
    {
        UINT32 HeightDivisor = MapHeight / TextureTileHeight;
        Link.ScaleShiftY = -1 * (INT32)FloorLog2(HeightDivisor);
    }

    pVector->push_back(Link);
}

void FeedbackTexture::AddLinkedTexture(StreamingTexture* pTexture)
{
    ::AddLinkedTexture(&Links, Staging.CopyLocation.PlacedFootprint.Footprint.Width, Staging.CopyLocation.PlacedFootprint.Footprint.Height, pTexture);
}

void SharedMinLODTexture::AddLinkedTexture(StreamingTexture* pTexture)
{
    ::AddLinkedTexture(&Links, MapWidth, MapHeight, pTexture);
}

TrackedTile** StreamingTexture::FindTrackedTile(UINT32 MipIndex, UINT32 Slice, UINT32 TileX, UINT32 TileY, UINT32 TileZ, bool* pFirstAccess)
{
    bool IsPackedMip = false;
    const UINT64 Index = TiledResourceImage::ComputeTileOffset(&FileHeader, MipIndex, Slice, TileX, TileY, TileZ, &IsPackedMip);
    assert(Index < TileArraySize);

    const UINT64 MaskIndex = (Index >> 6);
    const UINT64 MaskBit = (Index & 0x3f);
    const UINT64 TileMask = 1Ui64 << MaskBit;
    const UINT64 CurrentMask = pTileAccessedMask[MaskIndex];
    const UINT64 NewMask = CurrentMask | TileMask;
    if (pFirstAccess != nullptr)
    {
        *pFirstAccess = (NewMask != CurrentMask);
    }
    pTileAccessedMask[MaskIndex] = NewMask;

    return ppTileArray + Index;
}

bool StreamingTexture::DeleteTrackedTile(TrackedTile* pTT)
{
    const UINT32 MipCount = FileHeader.MipLevelCount;
    const UINT32 SliceIndex = pTT->Coords.Subresource / MipCount;
    const UINT32 MipIndex = pTT->Coords.Subresource % MipCount;

    TrackedTile** ppFound = FindTrackedTile(MipIndex, SliceIndex, pTT->Coords.X, pTT->Coords.Y, pTT->Coords.Z, nullptr);
    if (ppFound == nullptr || *ppFound != pTT)
    {
        assert(false);
        return false;
    }

    *ppFound = nullptr;
    return true;
}

void StreamingTextureManager::Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pInitCmdList, UINT32 TilePoolTileCount, UINT32 MaxTextureCount)
{
    HRESULT hr;

    m_LoggingEnabled = false;

    QueryPerformanceCounter(&m_CurrentTimeTicks);
    QueryPerformanceFrequency(&m_PerfFreq);
    m_NextStatsTimeTicks.QuadPart = m_CurrentTimeTicks.QuadPart + m_PerfFreq.QuadPart;
    m_LastStatsTimeTicks = m_CurrentTimeTicks;
    SetAgeOutTimeMsec(3000);
    m_AgeOutUrgentShift = 0;
    m_PauseAgeOut = false;
    m_PauseLoading = false;
    m_MaxTileLoadsPerFrame = -1;
    m_MaxTilePopulatesPerFrame = -1;
    m_MaxTileMappingsPerFrame = -1;
    m_LoadSleepMsec = 0;
    m_DecompressSleepMsec = 0;
    m_TileCountInFlight = 0;
    m_FlushRequested = false;
    m_ReadFeedbackMaps = true;
    m_RandomLoadsPerFrame = 0;
    m_DoNotLoadUpperLeft = false;
    m_MinLODMapEditDelayCount = 0;
    m_MarkTileBoundaries = false;

    ZeroMemory(&m_Stats, sizeof(m_Stats));
    m_AllTileCount = 0;
    m_TileBytesLoadedPerInterval = 0;
    m_TilesLoadedPerInterval = 0;

    m_pd3dDevice = pd3dDevice;
#if XBOX_SAMPLER_FEEDBACK
    hr = m_pd3dDevice->QueryInterface(__uuidof(ID3D12Device8), (void**)&m_pd3dDevice8);
    assert(SUCCEEDED(hr));
#endif
    InitializeCriticalSection(&m_TextureDBCritSec);
    InitializeCriticalSection(&m_StreamingListsCritSec);
    InitializeCriticalSection(&m_AllTilesCritSec);
    InitializeCriticalSection(&m_AsyncLoadCritSec);
    m_AllTilesOwningThreadID = 0;

    for (UINT32 i = 0; i < ARRAYSIZE(m_AsyncLoads); ++i)
    {
        m_AsyncLoads[i].Initialize();
    }

    m_TextureCpuDescriptorHeap.Initialize(pd3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, false, 1, 2, MaxTextureCount + 1);
    m_TextureCpuDescriptorHeap.GetDescriptorHeap()->SetName(L"Streaming CPU Tiled Texture and Native MinLOD SRVs");
    m_SharedGpuDescriptorHeap.Initialize(pd3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2048, true);
    ((ID3D12DescriptorHeap*)m_SharedGpuDescriptorHeap)->SetName(L"Streaming Shared GPU CB-SRV-UAV");

    m_MinLODFeedbackSRVUAVHeap.Initialize(pd3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, false, 1, 1, MaxTextureCount * 2);

    m_NextFenceValue = 1;
    hr = m_pd3dDevice->CreateFence(m_NextFenceValue - 1, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_pStreamingFence);
    assert(SUCCEEDED(hr));
    m_pStreamingFence->SetName(L"StreamingTextureManager Fence");

    hr = m_pd3dDevice->CreateFence(m_NextFenceValue - 1, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_pRenderingFence);
    assert(SUCCEEDED(hr));
    m_pRenderingFence->SetName(L"StreamingTextureManager Rendering Fence");

    m_TextureUploadHeap.SetName(L"StreamingTextureManager Upload");
    m_TextureUploadHeap.Initialize(m_pd3dDevice, m_pStreamingFence, &m_NextFenceValue, 128 * 1024, false);

    m_TilePoolFreeTiles.Initialize(TilePoolTileCount, true);
    CD3DX12_HEAP_DESC HeapDesc((UINT64)TilePoolTileCount * TiledResourceImage::TILE_SIZE_BYTES, D3D12_HEAP_TYPE_DEFAULT, TiledResourceImage::TILE_SIZE_BYTES);
    HeapDesc.Flags = D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;
    hr = m_pd3dDevice->CreateHeap(&HeapDesc, __uuidof(m_pTilePool), (void**)&m_pTilePool);
    assert(SUCCEEDED(hr));
    m_pTilePool->SetName(L"Tile Pool");
    m_Stats.TilePoolSizeTiles = TilePoolTileCount;
#if SUPPORT_TIER_1
    m_Tier1DefaultTileIndex = RequestTilePoolSlot(true);
#endif

    const UINT32 CompressedStagingSlotCount = 256;
    const UINT64 CompressedStagingBytes = CompressedStagingSlotCount * TiledResourceImage::TILE_SIZE_BYTES;
    m_CompressedStagingAllocator.Initialize(CompressedStagingSlotCount, true);
    m_pCompressedStagingBuffer = new BYTE[CompressedStagingBytes];
    m_Stats.TileStagingMemoryBytes += CompressedStagingBytes;

    const UINT32 DecompressedStagingSlotCount = 256;
    const UINT64 DecompressedStagingBytes = DecompressedStagingSlotCount * TiledResourceImage::TILE_SIZE_BYTES;
    m_DecompressedStagingAllocator.Initialize(DecompressedStagingSlotCount, true);

    D3D12_HEAP_PROPERTIES HeapProperties = {};
    HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.VisibleNodeMask = D3D12XBOX_NODE_MASK;
    HeapProperties.CreationNodeMask = D3D12XBOX_NODE_MASK;

    D3D12_RESOURCE_DESC BufferDesc = {};
    BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    BufferDesc.Alignment = 0;
    BufferDesc.Width = DecompressedStagingBytes;
    BufferDesc.Height = 1;
    BufferDesc.DepthOrArraySize = 1;
    BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    BufferDesc.MipLevels = 1;
    BufferDesc.SampleDesc.Count = 1;
    BufferDesc.SampleDesc.Quality = 0;
    BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    BufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    D3D12_RESOURCE_STATES InitialUsage = D3D12_RESOURCE_STATE_GENERIC_READ;

    hr = m_pd3dDevice->CreateCommittedResource(&HeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &BufferDesc,
        InitialUsage,
        nullptr,
        __uuidof(ID3D12Resource),
        (void**)&m_pDecompressedStagingResource);

    assert(SUCCEEDED(hr));
    m_pDecompressedStagingResource->SetName(L"Decompressed Staging Buffer");
    m_Stats.TileStagingMemoryBytes += DecompressedStagingBytes;

    hr = m_pDecompressedStagingResource->Map(0, nullptr, (void**)&m_pDecompressedStagingBuffer);
    assert(SUCCEEDED(hr));

    {
        CD3DX12_RESOURCE_DESC TexDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 32, 32, 1, 0, 1, 0);
        CreateDefaultResource(m_pd3dDevice, &TexDesc, D3D12_RESOURCE_STATE_GENERIC_READ, (void**)&m_pDefaultTextureObjects[STCT_PrimaryTexture]);

        USHORT DefaultTextureIndex = m_TextureCpuDescriptorHeap.AllocateBlock();
        assert(DefaultTextureIndex == 0);
        D3D12_CPU_DESCRIPTOR_HANDLE hCpu = m_TextureCpuDescriptorHeap.GetCpuDescriptorHandle(DefaultTextureIndex);
        m_hDefaultTextureViews = hCpu;
        const UINT32 HandleIncrement = m_TextureCpuDescriptorHeap.GetIncrementSize();
        m_pd3dDevice->CreateShaderResourceView(m_pDefaultTextureObjects[STCT_PrimaryTexture], nullptr, hCpu);

        TexDesc.Width = 1;
        TexDesc.Height = 1;
        TexDesc.MipLevels = 1;
        TexDesc.Format = DXGI_FORMAT_R8_UNORM;
        CreateDefaultResource(m_pd3dDevice, &TexDesc, D3D12_RESOURCE_STATE_GENERIC_READ, (void**)&m_pDefaultTextureObjects[STCT_MinLODTexture]);
        hCpu.ptr += HandleIncrement;
        m_pd3dDevice->CreateShaderResourceView(m_pDefaultTextureObjects[STCT_MinLODTexture], nullptr, hCpu);

    #if defined(_XBOX_ONE) && defined(_TITLE)
        D3D12_GPU_VIRTUAL_ADDRESS PrimaryVA = m_pDefaultTextureObjects[STCT_PrimaryTexture]->GetGPUVirtualAddress();
        pInitCmdList->FillMemoryWith32BitValueX(PrimaryVA, TexDesc.Width * TexDesc.Height * 4, 0, D3D12XBOX_COPY_FLAG_NONE);
        D3D12_GPU_VIRTUAL_ADDRESS MinLODVA = m_pDefaultTextureObjects[STCT_MinLODTexture]->GetGPUVirtualAddress();
        pInitCmdList->FillMemoryWith32BitValueX(MinLODVA, 4, 0, D3D12XBOX_COPY_FLAG_NONE);
    #endif
    }

    m_pSharedFeedbackHeap = nullptr;
    m_SharedFeedbackHeapOffsetBytes = 0;
    m_pFeedbackBuffer = nullptr;
    m_CurrentFeedbackOffsetElements = 0;

    // The feedback buffer heap defaults to 2MB of memory:
    m_FeedbackBufferSizeElements = (2 * 1024 * 1024) / g_FeedbackElementSizeBytes;

#if XBOX_SAMPLER_FEEDBACK
    D3D12_HEAP_DESC FeedbackHeapDesc = CD3DX12_HEAP_DESC(m_FeedbackBufferSizeElements * g_FeedbackElementSizeBytes, D3D12_HEAP_TYPE_DEFAULT);
    hr = m_pd3dDevice->CreateHeap(&FeedbackHeapDesc, __uuidof(m_pSharedFeedbackHeap), (void**)&m_pSharedFeedbackHeap);
    assert(SUCCEEDED(hr));
    m_pSharedFeedbackHeap->SetName(L"Shared Feedback Resource Memory Heap");

    BufferDesc.Width = FeedbackHeapDesc.SizeInBytes;
    BufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = m_pd3dDevice->CreatePlacedResource(m_pSharedFeedbackHeap, 0, &BufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, __uuidof(m_pFeedbackBuffer), (void**)&m_pFeedbackBuffer);
    assert(SUCCEEDED(hr));
    m_pFeedbackBuffer->SetName(L"Shared Feedback Buffer for UAV Clear");
#else
    BufferDesc.Width = m_FeedbackBufferSizeElements * g_FeedbackElementSizeBytes;
    BufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    hr = CreateDefaultResource(m_pd3dDevice, &BufferDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, (void**)&m_pFeedbackBuffer);
    assert(SUCCEEDED(hr));
    m_pFeedbackBuffer->SetName(L"Feedback Buffer");
#endif

    m_pFeedbackStagingData = nullptr;

    BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    {
        hr = m_pd3dDevice->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &BufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&m_pFeedbackStagingBuffer);
        assert(SUCCEEDED(hr));
        m_pFeedbackStagingBuffer->SetName(L"Feedback Staging Buffer");
        m_FeedbackStagingRange.Begin = 0;
        m_FeedbackStagingRange.End = BufferDesc.Width;

        void* pStagingData = nullptr;
        m_pFeedbackStagingBuffer->Map(0, &m_FeedbackStagingRange, &pStagingData);
        memset(pStagingData, 0xFE, BufferDesc.Width);
        m_pFeedbackStagingBuffer->Unmap(0, &m_FeedbackStagingRange);

    #if XBOX_SAMPLER_FEEDBACK
        D3D12XBOX_UNORDERED_ACCESS_VIEW_DESC StagingDesc = {};
        StagingDesc.Buffer.NumElements = BufferDesc.Width;
        StagingDesc.Format = g_FeedbackElementFormat;
        StagingDesc.ResourceLocation = m_pFeedbackStagingBuffer->GetGPUVirtualAddress();
        StagingDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        m_pd3dDevice->CreatePlacedUnorderedAccessViewX(m_pFeedbackBuffer, &StagingDesc, m_SharedGpuDescriptorHeap.hCPU(1));
    #endif
    }

    {
        USHORT UAVBlockIndex = m_TextureCpuDescriptorHeap.AllocateBlock();
        m_chFeedbackBufferUAV = m_TextureCpuDescriptorHeap.GetCpuDescriptorHandle(UAVBlockIndex);
        D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
        UAVDesc.Buffer.NumElements = m_FeedbackBufferSizeElements;
        UAVDesc.Format = g_FeedbackElementFormat;
        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        m_pd3dDevice->CreateUnorderedAccessView(m_pFeedbackBuffer, nullptr, &UAVDesc, m_chFeedbackBufferUAV);
        m_ghFeedbackBufferUAV = m_SharedGpuDescriptorHeap.hGPU(0);
        m_pd3dDevice->CopyDescriptorsSimple(1, m_SharedGpuDescriptorHeap.hCPU(0), m_chFeedbackBufferUAV, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        const UINT ClearValues[4] = { -1, -1, -1, -1 };
        pInitCmdList->ClearUnorderedAccessViewUint(m_ghFeedbackBufferUAV, m_chFeedbackBufferUAV, m_pFeedbackBuffer, ClearValues, 0, nullptr);
    }

#if XBOX_SAMPLER_FEEDBACK
    CD3DX12_DESCRIPTOR_RANGE DescRange[2];
    DescRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // u0
    DescRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1); // u1

    CD3DX12_ROOT_PARAMETER RTSlot[3];
    RTSlot[0].InitAsDescriptorTable(1, &DescRange[0], D3D12_SHADER_VISIBILITY_ALL); // u0
    RTSlot[1].InitAsDescriptorTable(1, &DescRange[1], D3D12_SHADER_VISIBILITY_ALL); // u1
    RTSlot[2].InitAsConstantBufferView(0); // b0

    CD3DX12_ROOT_SIGNATURE_DESC RTLayout(ARRAYSIZE(RTSlot), RTSlot);
    ID3DBlob* pSerializedLayout = nullptr;
    hr = D3D12SerializeRootSignature(&RTLayout, D3D_ROOT_SIGNATURE_VERSION_1, &pSerializedLayout, NULL);

    hr = m_pd3dDevice->CreateRootSignature(
        D3D12XBOX_NODE_MASK,
        pSerializedLayout->GetBufferPointer(),
        pSerializedLayout->GetBufferSize(),
        __uuidof(ID3D12RootSignature),
        (void**)&m_pFeedbackResolveSignature);

    assert(SUCCEEDED(hr));

    SAFE_RELEASE(pSerializedLayout);

    D3D12_COMPUTE_PIPELINE_STATE_DESC FeedbackResolvePSODesc = {};
    FeedbackResolvePSODesc.CS.pShaderBytecode = CSFeedbackResolve_cfxhpp;
    FeedbackResolvePSODesc.CS.BytecodeLength = sizeof(CSFeedbackResolve_cfxhpp);
    FeedbackResolvePSODesc.NodeMask = D3D12XBOX_NODE_MASK;
    FeedbackResolvePSODesc.pRootSignature = m_pFeedbackResolveSignature;

    hr = m_pd3dDevice->CreateComputePipelineState(&FeedbackResolvePSODesc, __uuidof(m_pFeedbackResolvePSO), (void**)&m_pFeedbackResolvePSO);
    assert(SUCCEEDED(hr));
#endif
}

void StreamingTextureManager::SetAgeOutTimeMsec(UINT32 Msec)
{
    m_AgeOutTickCount = (m_PerfFreq.QuadPart * (UINT64)Msec) / 1000Ui64;
}

void StreamingTextureManager::FlushLoadedTiles()
{
    m_FlushRequested = true;
    LogString("Flush requested");
}

StreamingTexture* StreamingTextureManager::LoadTexture(const WCHAR* strFileName, const WCHAR* strDataFileName, HANDLE hDataFile, UINT64 DataFileOffset)
{
    const UINT64 NameHash = HashFilename(strFileName);

    ScopedCritSec Lock(&m_TextureDBCritSec);

    StreamingTexture* pST = FindExistingTexture(NameHash, strFileName);
    if (pST != nullptr)
    {
        pST->RefCount++;
        return pST;
    }

    pST = CreateAndAddNewTexture(NameHash, strFileName, strDataFileName, hDataFile, DataFileOffset);

    return pST;
}

StreamingTexture* StreamingTextureManager::FindTextureByUniqueID(UINT32 UniqueID)
{
    ScopedCritSec Lock(&m_TextureDBCritSec);

    auto iter = m_TextureDB.begin();
    auto end = m_TextureDB.end();
    while (iter != end)
    {
        StreamingTexture* pST = iter->second;
        ++iter;
        if (pST->TextureUniqueIndex == UniqueID)
        {
            return pST;
        }
    }
    return nullptr;
}

void StreamingTextureManager::UnloadTexture(StreamingTexture* pST)
{
    ScopedCritSec Lock(&m_TextureDBCritSec);

    assert(pST->RefCount >= 1);
    pST->RefCount--;
    if (pST->RefCount == 0)
    {
        DestroyTexture(pST);
    }
}

UINT64 StreamingTextureManager::HashFilename(const WCHAR* strFileName)
{
    UINT64 HashVal = 0;

    const WCHAR* p = strFileName;
    while (*p != '\0')
    {
        HashVal += (UINT64)(*p) * 193951;
        HashVal *= 399283;
        ++p;
    }

    return HashVal;
}

SharedMinLODTexture* StreamingTextureManager::CreateSharedMinLODTexture(UINT32 Width, UINT32 Height, UINT32 SliceCount)
{
    HRESULT hr;

    SharedMinLODTexture* pRT = nullptr;
    USHORT SRVIndex = BlockAllocator::NULL_BLOCK_INDEX;
    USHORT UAVIndex = BlockAllocator::NULL_BLOCK_INDEX;

    ScopedCritSec Lock(&m_TextureDBCritSec);

    SRVIndex = m_MinLODFeedbackSRVUAVHeap.AllocateBlock();
    if (SRVIndex == BlockAllocator::NULL_BLOCK_INDEX)
    {
        goto ErrorCleanup;
    }
    UAVIndex = m_MinLODFeedbackSRVUAVHeap.AllocateBlock();
    if (UAVIndex == BlockAllocator::NULL_BLOCK_INDEX)
    {
        goto ErrorCleanup;
    }

    pRT = new SharedMinLODTexture();
    pRT->SRVDescriptorIndex = SRVIndex;
    pRT->UAVDescriptorIndex = UAVIndex;
    pRT->MapWidth = Width;
    pRT->MapHeight = Height;

    D3D12_RESOURCE_DESC MinLODDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, Width, Height, (UINT16)SliceCount, 1);

    hr = CreateDefaultResource(m_pd3dDevice, &MinLODDesc, D3D12_RESOURCE_STATE_GENERIC_READ, (void**)&pRT->pTextureResource);
    if (FAILED(hr))
    {
        goto ErrorCleanup;
    }

    WCHAR strTextureName[128];
    swprintf_s(strTextureName, L"Shared MinLOD Texture %u", (UINT32)SRVIndex);
    pRT->pTextureResource->SetName(strTextureName);

    pRT->hSRV = m_MinLODFeedbackSRVUAVHeap.GetCpuDescriptorHandle(SRVIndex);
    m_pd3dDevice->CreateShaderResourceView(pRT->pTextureResource, nullptr, pRT->hSRV);

    pRT->hUAV = m_MinLODFeedbackSRVUAVHeap.GetCpuDescriptorHandle(UAVIndex);
    m_pd3dDevice->CreateUnorderedAccessView(pRT->pTextureResource, nullptr, nullptr, pRT->hUAV);

    m_SharedMinLODTextures.push_back(pRT);

    m_Stats.TrackingStructMemoryBytes += sizeof(*pRT);

    return pRT;

ErrorCleanup:

    if (pRT != nullptr)
    {
        delete pRT;
    }

    if (SRVIndex != BlockAllocator::NULL_BLOCK_INDEX)
    {
        m_MinLODFeedbackSRVUAVHeap.FreeBlock(SRVIndex);
    }

    return nullptr;
}

FeedbackTexture* StreamingTextureManager::CreateFeedbackTexture(UINT32 Width, UINT32 Height, UINT32 TileWidth, UINT32 TileHeight, UINT32 SliceCount)
{
    FeedbackTexture* pFT = nullptr;
    USHORT DescriptorIndex = BlockAllocator::NULL_BLOCK_INDEX;
    D3D12_UNORDERED_ACCESS_VIEW_DESC BufferUAVDesc = {};
    UINT32 FeedbackElementOffset = 0;

    const UINT32 RowPitch = Width * g_FeedbackElementSizeBytes;
    const UINT32 FeedbackElementCount = (RowPitch * Height * SliceCount) / g_FeedbackElementSizeBytes;

    ScopedCritSec Lock(&m_TextureDBCritSec);

    const UINT32 PreAllocationFeedbackElementOffset = m_CurrentFeedbackOffsetElements;

    DescriptorIndex = m_MinLODFeedbackSRVUAVHeap.AllocateBlock();
    if (DescriptorIndex == BlockAllocator::NULL_BLOCK_INDEX)
    {
        goto ErrorCleanup;
    }

    pFT = new FeedbackTexture();
    if (pFT == nullptr)
    {
        goto ErrorCleanup;
    }
    pFT->DescriptorIndex = DescriptorIndex;

    pFT->hUAV = m_MinLODFeedbackSRVUAVHeap.GetCpuDescriptorHandle(DescriptorIndex);

#if XBOX_SAMPLER_FEEDBACK

    D3D12_RESOURCE_DESC MinLODFeedbackDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_UNKNOWN, Width, (UINT16)Height, (UINT16)SliceCount, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    D3D12_RESOURCE_DESC1 FeedbackDesc1;
    memcpy(&FeedbackDesc1, &MinLODFeedbackDesc, sizeof(MinLODFeedbackDesc));
    FeedbackDesc1.SamplerFeedbackMipRegion.Width = TileWidth;
    FeedbackDesc1.SamplerFeedbackMipRegion.Height = TileHeight;
    FeedbackDesc1.SamplerFeedbackMipRegion.Depth = 1;
    FeedbackDesc1.Width *= TileWidth;
    FeedbackDesc1.Height *= TileHeight;
    
    FeedbackElementOffset = 0;
    HRESULT hr = FindOrCreateBatchedFeedbackResource(&FeedbackDesc1, &pFT->pTextureResource, &pFT->FirstSliceIndexWithinResource, &FeedbackElementOffset);

    if (FAILED(hr))
    {
        goto ErrorCleanup;
    }

#else

    FeedbackElementOffset = m_CurrentFeedbackOffsetElements;

    // Create a buffer UAV for the feedback texture, without creating an actual texture resource:
    pFT->pTextureResource = nullptr;
    pFT->FirstSliceIndexWithinResource = 0;
    BufferUAVDesc.Format = g_FeedbackElementFormat;
    BufferUAVDesc.Buffer.FirstElement = FeedbackElementOffset;
    BufferUAVDesc.Buffer.NumElements = FeedbackElementCount;
    BufferUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    m_pd3dDevice->CreateUnorderedAccessView(m_pFeedbackBuffer, nullptr, &BufferUAVDesc, pFT->hUAV);

    m_Stats.FeedbackResourceMemoryBytes += FeedbackElementCount * g_FeedbackElementSizeBytes;

    assert(FeedbackElementOffset + FeedbackElementCount <= m_FeedbackBufferSizeElements);
    m_CurrentFeedbackOffsetElements += FeedbackElementCount;

#endif

    ZeroMemory(&pFT->Staging, sizeof(pFT->Staging));
    pFT->Staging.CopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    pFT->Staging.CopyLocation.pResource = m_pFeedbackStagingBuffer;
    pFT->Staging.CopyRange.Begin = FeedbackElementOffset;
    pFT->Staging.CopyRange.End = FeedbackElementOffset + FeedbackElementCount;

    {
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT& PF = pFT->Staging.CopyLocation.PlacedFootprint;
        PF.Footprint.Width = Width;
        PF.Footprint.Height = Height;
        PF.Footprint.RowPitch = RowPitch;
        PF.Footprint.Depth = SliceCount;
        PF.Footprint.Format = g_FeedbackElementFormat;
        PF.Offset = FeedbackElementOffset * g_FeedbackElementSizeBytes;
    }

    m_Stats.FeedbackStagingMemoryBytes += FeedbackElementCount * g_FeedbackElementSizeBytes;
    m_Stats.TrackingStructMemoryBytes += sizeof(*pFT);

    m_FeedbackTextures.push_back(pFT);

    return pFT;

ErrorCleanup:

    if (pFT != nullptr)
    {
        delete pFT;
    }

    if (DescriptorIndex != BlockAllocator::NULL_BLOCK_INDEX)
    {
        m_MinLODFeedbackSRVUAVHeap.FreeBlock(DescriptorIndex);
    }

    m_CurrentFeedbackOffsetElements = PreAllocationFeedbackElementOffset;

    return nullptr;
}

#if XBOX_SAMPLER_FEEDBACK
HRESULT StreamingTextureManager::FindOrCreateBatchedFeedbackResource(const D3D12_RESOURCE_DESC1* pOpaqueResourceDesc, ID3D12Resource** ppResource, UINT32* pFirstSliceIndex, UINT32* pFeedbackStagingOffsetElements)
{
    ScopedCritSec Lock(&m_TextureDBCritSec);

    const UINT32 FeedbackWidthTexels = pOpaqueResourceDesc->Width / pOpaqueResourceDesc->SamplerFeedbackMipRegion.Width;
    const UINT32 FeedbackHeightTexels = pOpaqueResourceDesc->Height / pOpaqueResourceDesc->SamplerFeedbackMipRegion.Height;

    for (OpaqueTextureArray* pArray : m_FeedbackResourceDB)
    {
        const D3D12_RESOURCE_DESC1& ArrayDesc = pArray->Desc;
        if (pArray->WidthTexels == FeedbackWidthTexels &&
            pArray->HeightTexels == FeedbackHeightTexels &&
            (pArray->AllocatedSlices + pOpaqueResourceDesc->DepthOrArraySize) <= ArrayDesc.DepthOrArraySize)
        {
            *pFirstSliceIndex = pArray->AllocatedSlices;
            *pFeedbackStagingOffsetElements = pArray->StagingBaseOffsetElements + (pArray->StagingSlicePitchElements * pArray->AllocatedSlices);
            pArray->AllocatedSlices += pOpaqueResourceDesc->DepthOrArraySize;
            *ppResource = pArray->pResource;
            return S_OK;
        }
    }

    const DXGI_FORMAT FeedbackFormat = DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE;

    OpaqueTextureArray* pNewArray = new OpaqueTextureArray();
    pNewArray->AllocatedSlices = 0;
    pNewArray->pResource = nullptr;
    pNewArray->Desc = *pOpaqueResourceDesc;
    pNewArray->Desc.DepthOrArraySize = 256;
    pNewArray->Desc.Format = FeedbackFormat;
    pNewArray->WidthTexels = FeedbackWidthTexels;
    pNewArray->HeightTexels = FeedbackHeightTexels;
    pNewArray->StagingSlicePitchElements = FeedbackWidthTexels * FeedbackHeightTexels;

    HRESULT hr;

    D3D12_RESOURCE_ALLOCATION_INFO AllocInfo = m_pd3dDevice8->GetResourceAllocationInfo2(D3D12XBOX_NODE_MASK, 1, &pNewArray->Desc, nullptr);

    if (m_pSharedFeedbackHeap != nullptr)
    {
        if (AllocInfo.SizeInBytes > 0)
        {
            AllocInfo.Alignment = NextMultiple(AllocInfo.Alignment, (UINT64)D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT);
            pNewArray->Desc.Alignment = AllocInfo.Alignment;

            const UINT64 HeapOffsetBytes = NextMultiple(m_SharedFeedbackHeapOffsetBytes, AllocInfo.Alignment);
            m_SharedFeedbackHeapOffsetBytes = HeapOffsetBytes + AllocInfo.SizeInBytes;

            hr = m_pd3dDevice8->CreatePlacedResource1(
                m_pSharedFeedbackHeap,
                HeapOffsetBytes,
                &pNewArray->Desc,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                __uuidof(pNewArray->pResource),
                (void**)&pNewArray->pResource);
        }
        else
        {
            assert(false);
            hr = E_FAIL;
        }
    }
    else
    {
        CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_DEFAULT);
        hr = m_pd3dDevice8->CreateCommittedResource2(
            &HeapProps,
            D3D12_HEAP_FLAG_NONE,
            &pNewArray->Desc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            nullptr,
            __uuidof(pNewArray->pResource),
            (void**)&pNewArray->pResource);
    }

    if (FAILED(hr))
    {
        delete pNewArray;
        return hr;
    }

    WCHAR strTitle[128];
    swprintf_s(strTitle, L"Feedback Array Texture %u (%u x %u %u slices)", (UINT32)m_FeedbackResourceDB.size(), (UINT32)pNewArray->Desc.Width, (UINT32)pNewArray->Desc.Height, (UINT32)pNewArray->Desc.DepthOrArraySize);
    pNewArray->pResource->SetName(strTitle);

    const UINT32 UAVIndex = (UINT32)m_FeedbackResourceDB.size() + 2;
    pNewArray->hUAV = m_SharedGpuDescriptorHeap.hGPU(UAVIndex);
    m_pd3dDevice->CreateUnorderedAccessView(pNewArray->pResource, nullptr, nullptr, m_SharedGpuDescriptorHeap.hCPU(UAVIndex));

    const UINT32 StagingSizeElements = pNewArray->StagingSlicePitchElements * pNewArray->Desc.DepthOrArraySize;
    assert(m_CurrentFeedbackOffsetElements + StagingSizeElements <= m_FeedbackBufferSizeElements);
    pNewArray->StagingBaseOffsetElements = m_CurrentFeedbackOffsetElements;
    m_CurrentFeedbackOffsetElements += StagingSizeElements;

    m_Stats.FeedbackResourceMemoryBytes += (UINT32)AllocInfo.SizeInBytes;
    m_Stats.TrackingStructMemoryBytes += sizeof(*pNewArray);

    pNewArray->AllocatedSlices = pOpaqueResourceDesc->DepthOrArraySize;
    m_FeedbackResourceDB.push_back(pNewArray);

    *ppResource = pNewArray->pResource;
    *pFirstSliceIndex = 0;
    *pFeedbackStagingOffsetElements = pNewArray->StagingBaseOffsetElements;
    return S_OK;
}
#endif

FeedbackTexture* StreamingTextureManager::CreateFeedbackTexture(StreamingTexture* pST)
{
    const TiledResourceImage::Header& FH = pST->FileHeader;
    const TiledResourceImage::MipLevel& BaseMip = FH.MipLevels[0];

    const UINT32 MetaWidth = BaseMip.WidthTilesM1 + 1;
    const UINT32 MetaHeight = BaseMip.HeightTilesM1 + 1;

    UINT32 TileWidthTexels = 0;
    UINT32 TileHeightTexels = 0;
    TiledResourceImage::GetTileShape(&FH, nullptr, nullptr, &TileWidthTexels, &TileHeightTexels);

    FeedbackTexture* pFT = CreateFeedbackTexture(MetaWidth, MetaHeight, TileWidthTexels, TileHeightTexels, FH.SliceCount);

    if (pFT != nullptr)
    {
    #if XBOX_SAMPLER_FEEDBACK
        D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
        UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        UAVDesc.Texture2DArray.ArraySize = FH.SliceCount;
        UAVDesc.Texture2DArray.FirstArraySlice = pFT->FirstSliceIndexWithinResource;
        m_pd3dDevice8->CreateSamplerFeedbackUnorderedAccessViewX(pST->pTiledTextureResource, pFT->pTextureResource, &UAVDesc, pFT->hUAV);
    #endif

        pFT->AddLinkedTexture(pST);
    }

    return pFT;
}

void StreamingTextureManager::LogString(const CHAR* strFormat, ...) const
{
    if (!IsLoggingEnabled())
    {
        return;
    }

    CHAR strLine[1024];
    SIZE_T numChars = ARRAYSIZE(strLine);

    sprintf_s(strLine, "StreamingTextureManager frame %I64u: ", m_NextFenceValue);
    const SIZE_T PrefixSize = strlen(strLine);
    numChars -= PrefixSize;

    va_list args = nullptr;

    va_start(args, strFormat);

    vsprintf_s(strLine + PrefixSize, numChars, strFormat, args);

    va_end(args);

    strcat_s(strLine, "\n");

    OutputDebugStringA(strLine);
}

void StreamingTextureManager::LogTileOperation(const TrackedTile* pTT, const CHAR* strOperation) const
{
    LogString("%s texture %u subresource %u%s XYZ <%u %u %u> tile 0x%p parent 0x%p",
        strOperation,
        pTT->pTexture->TextureUniqueIndex,
        pTT->Coords.Subresource,
        pTT->MipTail ? " (mip tail)" : "",
        pTT->Coords.X,
        pTT->Coords.Y,
        pTT->Coords.Z,
        pTT,
        pTT->pParentTile);
}

StreamingTexture* StreamingTextureManager::FindExistingTexture(UINT64 Hash, const WCHAR* strFileName) const
{
    auto range = m_TextureDB.equal_range(Hash);
    auto iter = range.first;
    auto end = range.second;
    while (iter != end)
    {
        StreamingTexture* pST = iter->second;
        ++iter;

        if (_wcsicmp(pST->strFileName, strFileName) == 0)
        {
            return pST;
        }
    }
    return nullptr;
}

StreamingTexture* StreamingTextureManager::CreateAndAddNewTexture(UINT64 Hash, const WCHAR* strFileName, const WCHAR* strDataFileName, HANDLE hDataFile, UINT64 DataFileOffset)
{
    UINT64 FileOffsetBytes = 0;
    HANDLE hFile = nullptr;
    if (strDataFileName != nullptr)
    {
        hFile = ImplOpenTextureDataFile(strDataFileName);
        assert(hFile != NULL);
    }
    else
    {
        hFile = hDataFile;
        FileOffsetBytes = DataFileOffset;
    }

    StreamingTexture* pST = new StreamingTexture();
    pST->hFile = hFile;
    pST->FileOffsetBytes = FileOffsetBytes;
    wcscpy_s(pST->strFileName, strFileName);
    pST->FileNameHash = Hash;
    pST->RefCount = 1;
    const USHORT PrimaryDHBlockIndex = m_TextureCpuDescriptorHeap.AllocateBlock();
    assert(PrimaryDHBlockIndex != BlockAllocator::NULL_BLOCK_INDEX);
    pST->TextureUniqueIndex = PrimaryDHBlockIndex;

    pST->hTiledTextureSRV = m_TextureCpuDescriptorHeap.GetCpuDescriptorHandle(PrimaryDHBlockIndex);
    pST->hNativeMinLODTextureSRV = pST->hTiledTextureSRV;
    pST->hNativeMinLODTextureSRV.ptr += m_TextureCpuDescriptorHeap.GetIncrementSize();

    pST->NoMinLODUpdates = 0;

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ImplCreateStandInTexture(pST);
    }
    else
    {
        ImageReadTextureFileHeader(pST);

        {
            ScopedCritSec Lock(&m_StreamingListsCritSec);
            m_PackedMipPendingQueue.push_back(pST);
        }

        const UINT64 TileCount = TiledResourceImage::GetTotalTileCount(&pST->FileHeader);
        const UINT64 TileBufferSizeBytes = TileCount * sizeof(TrackedTile*);
        pST->ppTileArray = (TrackedTile**)malloc(TileBufferSizeBytes);
        ZeroMemory(pST->ppTileArray, TileBufferSizeBytes);
        pST->TileArraySize = TileCount;

        const UINT64 TileMaskQwordCount = (TileCount + 0x3f) >> 6;
        pST->pTileAccessedMask = new UINT64[TileMaskQwordCount];
        ZeroMemory(pST->pTileAccessedMask, sizeof(UINT64) * TileMaskQwordCount);
    }

    m_TextureDB.emplace(Hash, pST);

    m_Stats.TextureCount++;
    m_Stats.VirtualTotalTileCount += TiledResourceImage::GetTotalTileCount(&pST->FileHeader);
    m_Stats.TrackingStructMemoryBytes += sizeof(*pST);
    TiledResourceImage::GetTotalTileCountPerMip(&pST->FileHeader, m_Stats.VirtualTotalTileCountPerMip, ARRAYSIZE(m_Stats.VirtualTotalTileCountPerMip), false);

#if SUPPORT_TIER_1
    {
        ScopedCritSec Lock(&m_StreamingListsCritSec);
        m_DefaultTileMapPendingQueue.push_back(pST);
    }
#endif

    return pST;
}

TilePoolAtlas* StreamingTextureManager::FindOrCreateTilePoolAtlas(const TiledResourceImage::Header* pHeader, ID3D12CommandQueue* pCmdQueue)
{
    const DXGI_FORMAT Format = (DXGI_FORMAT)pHeader->DXGIFormat;

    if (Format >= ARRAYSIZE(m_pTilePoolAtlas))
    {
        assert(false);
        return nullptr;
    }

    ScopedCritSec Lock(&m_TextureDBCritSec);

    if (m_pTilePoolAtlas[Format] != nullptr)
    {
        return m_pTilePoolAtlas[Format];
    }

    if (pCmdQueue == nullptr)
    {
        return nullptr;
    }

    TilePoolAtlas* pNewAtlas = new TilePoolAtlas();
    if (pNewAtlas == nullptr)
    {
        assert(false);
        return nullptr;
    }
    ZeroMemory(pNewAtlas, sizeof(*pNewAtlas));

    UINT32 TileWidthTexels = 0;
    UINT32 TileHeightTexels = 0;
    TiledResourceImage::GetTileShape(pHeader, nullptr, nullptr, &TileWidthTexels, &TileHeightTexels);

    const UINT32 TexWidth = 16384;
    const UINT32 TexHeight = 16384;
    const UINT32 TileWidth = TexWidth / TileWidthTexels;
    const UINT32 TileHeight = TexHeight / TileHeightTexels;
    const UINT32 SliceTiles = TileWidth * TileHeight;
    const UINT32 TilePoolTileCount = GetTilePoolTileCount();
    const UINT32 SliceCount = (TilePoolTileCount + SliceTiles - 1) / SliceTiles;

    CD3DX12_RESOURCE_DESC TexDesc = CD3DX12_RESOURCE_DESC::Tex2D(Format, TexWidth, TexHeight, (UINT16)SliceCount, 1);
    TexDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
    ID3D12Resource* pResource = nullptr;

    HRESULT hr = m_pd3dDevice->CreateReservedResource(&TexDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&pResource);
    assert(SUCCEEDED(hr));

    WCHAR strTextureName[64];
    swprintf_s(strTextureName, L"Tile Pool Atlas Format %u", Format);
    pResource->SetName(strTextureName);

    D3D12_TILED_RESOURCE_COORDINATE ZeroCoord = {};
    D3D12_TILE_REGION_SIZE FullRegion = {};
    FullRegion.UseBox = false;
    FullRegion.NumTiles = TilePoolTileCount;
    D3D12_TILE_RANGE_FLAGS NoFlags = D3D12_TILE_RANGE_FLAG_NONE;
    UINT32 ZeroOffset = 0;
    pCmdQueue->UpdateTileMappings(pResource, 1, &ZeroCoord, &FullRegion, m_pTilePool, 1, &NoFlags, &ZeroOffset, &TilePoolTileCount, D3D12_TILE_MAPPING_FLAG_NONE);

    pNewAtlas->pAtlasResource = pResource;
    pNewAtlas->TileWidthTexels = TileWidthTexels;
    pNewAtlas->TileHeightTexels = TileHeightTexels;
    pNewAtlas->ColumnCount = TileWidth;
    pNewAtlas->RowCount = TileHeight;

    m_pTilePoolAtlas[Format] = pNewAtlas;

    m_Stats.TrackingStructMemoryBytes += sizeof(*pNewAtlas);

    return pNewAtlas;
}

#if SUPPORT_TIER_1
void StreamingTextureManager::ProcessPendingDefaultMappings(ID3D12CommandQueue* pCmdQueue)
{
    if (m_DefaultTileMapPendingQueue.empty())
    {
        return;
    }

    assert(m_Tier1DefaultTileIndex < GetTilePoolTileCount());

    ScopedCritSec Lock(&m_StreamingListsCritSec);

    while (!m_DefaultTileMapPendingQueue.empty())
    {
        StreamingTexture* pST = m_DefaultTileMapPendingQueue.front();
        m_DefaultTileMapPendingQueue.pop_front();

        const UINT32 TileCount = TiledResourceImage::GetTotalTileCount(&pST->FileHeader);
        ID3D12Resource* pTexture = pST->pTextureResources[STCT_PrimaryTexture];
        assert(pTexture != nullptr);

        D3D12_TILED_RESOURCE_COORDINATE StartCoord = {};
        D3D12_TILE_REGION_SIZE FullSize = {};
        FullSize.UseBox = FALSE;
        FullSize.NumTiles = TileCount;
        D3D12_TILE_RANGE_FLAGS SingleRangeFlag = D3D12_TILE_RANGE_FLAG_REUSE_SINGLE_TILE;
        const UINT RangeStartOffset = m_Tier1DefaultTileIndex;

        pCmdQueue->UpdateTileMappings(pTexture, 1, &StartCoord, &FullSize, m_pTilePool, 1, &SingleRangeFlag, &RangeStartOffset, &TileCount, D3D12_TILE_MAPPING_FLAG_NONE);
    }
}
#endif

void StreamingTextureManager::LoadPendingPackedMips()
{
    if (m_PackedMipPendingQueue.empty())
    {
        return;
    }

    ScopedCritSec Lock(&m_StreamingListsCritSec);

    while (!m_PackedMipPendingQueue.empty())
    {
        StreamingTexture* pST = m_PackedMipPendingQueue.front();
        bool Success = ImageLoadPackedMips(pST);
        if (Success)
        {
            m_PackedMipPendingQueue.pop_front();
        }
        else
        {
            break;
        }
    }
}

void StreamingTextureManager::DestroyTexture(StreamingTexture* pST)
{
    // TODO: remove all linked tiles from tile tracker & tile pool

    auto range = m_TextureDB.equal_range(pST->FileNameHash);
    auto iter = range.first;
    auto end = range.second;
    while (iter != end)
    {
        StreamingTexture* pFoundST = iter->second;

        if (pFoundST == pST)
        {
            m_TextureDB.erase(iter);
            break;
        }

        ++iter;
    }

    // TODO: release D3D resources

    pST->MinLODTexture.Staging.DeleteCpuOnlyStaging();

    m_TextureCpuDescriptorHeap.FreeBlock(pST->TextureUniqueIndex);

    if (pST->ppTileArray != nullptr)
    {
        free(pST->ppTileArray);
        pST->ppTileArray = nullptr;
    }

    if (pST->pTileAccessedMask != nullptr)
    {
        delete[] pST->pTileAccessedMask;
        pST->pTileAccessedMask = nullptr;
    }

    m_Stats.TextureCount--;
    m_Stats.VirtualTotalTileCount -= TiledResourceImage::GetTotalTileCount(&pST->FileHeader);
    TiledResourceImage::GetTotalTileCountPerMip(&pST->FileHeader, m_Stats.VirtualTotalTileCountPerMip, ARRAYSIZE(m_Stats.VirtualTotalTileCountPerMip), true);

    delete pST;
}

TrackedTile* StreamingTextureManager::CreateTrackedTile()
{
    TrackedTile* pTT = nullptr;

    if (1)
    {
        ScopedCritSec Lock(&m_StreamingListsCritSec);
        if (!m_EmptyTrackedTileStructs.empty())
        {
            pTT = m_EmptyTrackedTileStructs.front();
            m_EmptyTrackedTileStructs.pop_front();
        }
    }

    if (pTT == nullptr)
    {
        pTT = new TrackedTile();
    }

    ZeroMemory(pTT, sizeof(*pTT));
    pTT->LastSeenFractionalLOD = 255;
    pTT->TileIndexInPool = BlockAllocator::NULL_BLOCK_INDEX;

    m_Stats.TrackingStructMemoryBytes += sizeof(*pTT);

    return pTT;
}

void StreamingTextureManager::FreeTrackedTile(TrackedTile* pTT)
{
    ScopedCritSec Lock(&m_StreamingListsCritSec);

    memset(pTT, 0xFF, sizeof(*pTT));
    m_EmptyTrackedTileStructs.push_back(pTT);
}

UINT32 StreamingTextureManager::RequestCompressedLoadSlot(bool Block)
{
    USHORT Slot = m_CompressedStagingAllocator.AllocateBlock();
    while (Block && Slot == BlockAllocator::NULL_BLOCK_INDEX)
    {
        Slot = m_CompressedStagingAllocator.AllocateBlock();
    }
    return (UINT32)Slot;
}

BYTE* StreamingTextureManager::GetCompressedLoadSlotBuffer(UINT32 Slot) const
{
    assert(Slot < m_CompressedStagingAllocator.GetBlockCount());
    return m_pCompressedStagingBuffer + (Slot * TiledResourceImage::TILE_SIZE_BYTES);
}

BYTE* StreamingTextureManager::GetDecompressedStagingSlotBuffer(UINT32 Slot) const
{
    assert(Slot < m_DecompressedStagingAllocator.GetBlockCount());
    return m_pDecompressedStagingBuffer + (Slot * TiledResourceImage::TILE_SIZE_BYTES);
}

USHORT StreamingTextureManager::RequestTilePoolSlot(bool Block)
{
    USHORT Slot = m_TilePoolFreeTiles.AllocateBlock();
    while (Block && Slot == BlockAllocator::NULL_BLOCK_INDEX)
    {
        Slot = m_TilePoolFreeTiles.AllocateBlock();
    }
    return (UINT32)Slot;
}

void StreamingTextureManager::QueueTrackedTileForLoading(TrackedTile* pTT, AsynchronousLoadOperation* pALO)
{
    assert(pTT->Status == TrackedTileStatus_Seen || pTT->Status == TrackedTileStatus_Invalid);
    assert(!pTT->IsMapped);

    assert(!pALO->IsAssignedToRead());

    {
        LONG NewCount = InterlockedIncrement(&m_TileCountInFlight);
        assert(NewCount > 0);

        ScopedCritSec Lock(&m_StreamingListsCritSec);
        pTT->Status = TrackedTileStatus_Loading;
        m_LoadingTiles.push_back(pTT);
        if (pTT->MipTail)
        {
            // Mip tail tiles need to be mapped to their tiled resource before population:
            m_NeedsMappedTiles.push_back(pTT);
        }
        m_Stats.LoadingTileCount++;
    }

    LogTileOperation(pTT, "Starting load");
    
    ImageLoadTile(pTT, pALO);
}

void StreamingTextureManager::UpdateStreaming(ID3D12CommandQueue* pUpdateQueue, ID3D12CommandQueue* pRenderQueue)
{
    PIX_CMDQUEUE_BLOCK(pUpdateQueue, L"UpdateStreaming");

    if (pRenderQueue != nullptr)
    {
        pRenderQueue->Signal(m_pRenderingFence, m_NextFenceValue);
        pUpdateQueue->Wait(m_pRenderingFence, m_NextFenceValue);

        pRenderQueue->Wait(m_pStreamingFence, m_NextFenceValue);
    }

    QueryPerformanceCounter(&m_CurrentTimeTicks);
    m_Bottlenecks.DwordValue = 0;

    CpuProbeWriter Probes(m_Stats.UpdateProbes, ARRAYSIZE(m_Stats.UpdateProbes));

#if SUPPORT_TIER_1
    ProcessPendingDefaultMappings(pUpdateQueue);
    Probes.WriteProbe("Process Pending Default Mappings");
#endif

    LoadPendingPackedMips();

    Probes.WriteProbe("Pending Packed Mips");

    ProcessFeedbackMaps(pUpdateQueue);

    Probes.WriteProbe("Process Feedback Maps", m_Stats.VirtualTotalTileCount);

    PerformRandomLoads();

    Probes.WriteProbe("Random Loads");

    AgeOutTiles(pUpdateQueue);

    Probes.WriteProbe("Age Out Tiles", m_Stats.VirtualMappedTileCount);

    LoadSeenTiles();

    Probes.WriteProbe("Load Seen Tiles");

    ProcessPendingTileMappings(pUpdateQueue);

    Probes.WriteProbe("Process Pending Tile Mappings");

    CheckAsyncLoadsForCompletion();
    ProcessCompletedLoadTiles(pUpdateQueue);
    ProcessReleasedSlots();

    Probes.WriteProbe("Async Load Completions");

    // Process MinLOD maps to apply new edits that were the result of this
    // frame's new unmappings during AgeOutTiles:
    const UINT64 MinLODMapEditCount = (UINT64)m_MinLODMapEdits.size();
    ProcessNativeMinLODMapEdits();

    Probes.WriteProbe("Process MinLOD Map Unmappings", MinLODMapEditCount);

    if (m_FlushRequested && GetTileInFlightCount() == 0)
    {
        LogString("Flush completed");
        m_FlushRequested = false;
    }

    if (m_CurrentTimeTicks.QuadPart >= m_NextStatsTimeTicks.QuadPart)
    {
        const UINT64 TickDelta = m_CurrentTimeTicks.QuadPart - m_LastStatsTimeTicks.QuadPart;
        DOUBLE TimeSeconds = (DOUBLE)TickDelta / (DOUBLE)m_PerfFreq.QuadPart;
        m_Stats.KBytesLoadedPerSecond = (UINT32)((DOUBLE)m_TileBytesLoadedPerInterval / (TimeSeconds * 1024.0));
        m_Stats.TileLoadsPerSecond = (UINT32)((DOUBLE)m_TilesLoadedPerInterval / TimeSeconds);
        m_Stats.PeakKBytesLoadedPerSecond = std::max(m_Stats.PeakKBytesLoadedPerSecond, m_Stats.KBytesLoadedPerSecond);
        m_Stats.PeakTileLoadsPerSecond = std::max(m_Stats.PeakTileLoadsPerSecond, m_Stats.TileLoadsPerSecond);

        m_TileBytesLoadedPerInterval = 0;
        m_TilesLoadedPerInterval = 0;
        m_NextStatsTimeTicks.QuadPart = m_CurrentTimeTicks.QuadPart + (m_PerfFreq.QuadPart >> 3);
        m_LastStatsTimeTicks = m_CurrentTimeTicks;
    }

    m_Stats.StorageReadLatency = m_StorageReadLatency.GenerateReport();
    m_Stats.TileSeenToLoadLatency = m_TileSeenToLoadLatency.GenerateReport();
    m_Stats.TileLoadToCompleteLatency = m_TileLoadToCompleteLatency.GenerateReport();

    pUpdateQueue->Signal(m_pStreamingFence, m_NextFenceValue++);
}

void StreamingTextureManager::PerformRandomLoads()
{
    for (UINT32 i = 0; i < m_RandomLoadsPerFrame; ++i)
    {
        ScopedCritSec Lock(&m_TextureDBCritSec);
        const UINT32 RandomTextureIndex = rand() % (UINT32)m_TextureDB.size();
        auto iter = m_TextureDB.begin();
        for (UINT32 n = 0; n < RandomTextureIndex; ++n)
        {
            ++iter;
        }
        StreamingTexture* pST = iter->second;
        const UINT32 SliceIndex = rand() % pST->FileHeader.SliceCount;
        const FLOAT TextureU = (FLOAT)rand() / (FLOAT)RAND_MAX;
        const FLOAT TextureV = (FLOAT)rand() / (FLOAT)RAND_MAX;
        FindAndTrackRegion2D(pST, SliceIndex, 0, TextureU, TextureV);
    }
}

UINT32 StreamingTextureManager::GetTileInFlightCount()
{
    LONG Count = InterlockedIncrement(&m_TileCountInFlight) - 1;
    InterlockedDecrement(&m_TileCountInFlight);
    assert(Count >= 0);
    return (UINT32)Count;
}

void StreamingTextureManager::ClearAllPendingTiles()
{
}

void StreamingTextureManager::ProcessFeedbackMaps(ID3D12CommandQueue* pCmdQueue)
{
    PIX_CMDQUEUE_BLOCK(pCmdQueue, L"ProcessFeedbackMaps");

    if (!m_ReadFeedbackMaps || m_FlushRequested || m_NextFenceValue < 10)
    {
        return;
    }

    MapFeedbackStagingBuffer();

    {
        ScopedCritSec TexDBLock(&m_TextureDBCritSec);
        ScopedCritSec AllTilesLock(&m_AllTilesCritSec, &m_AllTilesOwningThreadID);
        for (FeedbackTexture* pFT : m_FeedbackTextures)
        {
            ProcessFeedbackMap(pFT);
        }
    }

    UnmapFeedbackStagingBuffer();
}

void StreamingTextureManager::MapFeedbackStagingBuffer()
{
    assert(m_pFeedbackStagingData == nullptr);
    HRESULT hr = m_pFeedbackStagingBuffer->Map(0, &m_FeedbackStagingRange, (void**)&m_pFeedbackStagingData);
    assert(SUCCEEDED(hr));
}

void StreamingTextureManager::UnmapFeedbackStagingBuffer()
{
    assert(m_pFeedbackStagingData != nullptr);
    m_pFeedbackStagingBuffer->Unmap(0, nullptr);
    m_pFeedbackStagingData = nullptr;
}

void StreamingTextureManager::ProcessFeedbackMap(FeedbackTexture* pFT)
{
    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& PF = pFT->Staging.CopyLocation.PlacedFootprint;
    const UINT32 LinkedTextureCount = (UINT32)pFT->Links.size();

    const BYTE* pFeedbackTexels = nullptr;
    pFeedbackTexels = (const BYTE*)m_pFeedbackStagingData + PF.Offset;

    const UINT32 SliceCount = PF.Footprint.Depth;
    const UINT32 StagingMapWidth = PF.Footprint.Width;
    const UINT32 StagingMapHeight = PF.Footprint.Height;
    const UINT32 StagingMapPitch = PF.Footprint.RowPitch;

    const UINT32 HalfY = StagingMapHeight >> 1;
    const UINT32 HalfX = StagingMapWidth >> 1;

    for (UINT32 SliceIndex = 0; SliceIndex < SliceCount; ++SliceIndex)
    {
        const BYTE* pStagingSlice = pFeedbackTexels + SliceIndex * (StagingMapPitch * StagingMapHeight);
        for (UINT32 Y = 0; Y < StagingMapHeight; ++Y)
        {
            const UINT32 StartX = (m_DoNotLoadUpperLeft && Y < HalfY) ? HalfX : 0;
            const FeedbackElementType* pStagingRow = (const FeedbackElementType*)(pStagingSlice + (StagingMapPitch * Y));
            for (UINT32 X = StartX; X < StagingMapWidth; ++X)
            {
                const UINT32 StagingFractionalLOD = pStagingRow[X];

                for (UINT32 i = 0; i < LinkedTextureCount; ++i)
                {
                    const LinkedStreamingTexture& LinkedTexture = pFT->Links[i];

                    // TODO: handle scale factor between feedback map and streaming texture

                    if (StagingFractionalLOD < LinkedTexture.FirstNonStreamingMipFractionalLOD)
                    {
                        FindAndTrackRegion(LinkedTexture.pTexture, SliceIndex, StagingFractionalLOD, X, Y, 0);
                    }
                }
            }
        }
    }
}

void StreamingTextureManager::ProcessNativeMinLODMapEdits()
{
    if (m_MinLODMapEdits.empty())
    {
        return;
    }

    m_MinLODMapEdits.sort(MinLODMapEditPredicate);

    StreamingTexture* pCurrentTexture = nullptr;
    StagingTexture* pMinLODStagingTexture = nullptr;
    BYTE* pMinLODStagingTexels = nullptr;

    auto iter = m_MinLODMapEdits.begin();
    auto end = m_MinLODMapEdits.end();
    while (iter != end)
    {
        auto nextiter = iter;
        ++nextiter;
        MinLODMapEdit& RME = *iter;

        if (RME.DelayCount > 0 && !m_FlushRequested)
        {
            --RME.DelayCount;
            iter = nextiter;
            continue;
        }

        if (RME.pTexture != pCurrentTexture)
        {
            if (pCurrentTexture != nullptr)
            {
                pMinLODStagingTexels = nullptr;
                pMinLODStagingTexture->Unmap();
            }

            pCurrentTexture = RME.pTexture;
            pMinLODStagingTexture = &pCurrentTexture->MinLODTexture.Staging;
            pMinLODStagingTexels = (BYTE*)pMinLODStagingTexture->Map();
            assert(pMinLODStagingTexels != nullptr);
            pCurrentTexture->MinLODTexture.IsDirty = 1;
        }

        bool OperationCompleted = ApplyMinLODMapUnmap(RME, pCurrentTexture, *pMinLODStagingTexture, pMinLODStagingTexels);
        if (OperationCompleted)
        {
            m_MinLODMapEdits.erase(iter);
        }
        iter = nextiter;
    }

    if (pCurrentTexture != nullptr)
    {
        pMinLODStagingTexels = nullptr;
        pMinLODStagingTexture->Unmap();
    }
}

void StreamingTextureManager::CombineMinLODMaps()
{
    // TODO

#if 0
    ScopedCritSec Lock(&m_TextureDBCritSec);

    for (SharedMinLODTexture* pRT : m_SharedMinLODTextures)
    {
    }
#endif
}

bool StreamingTextureManager::ApplyMinLODMapUnmap(const MinLODMapEdit& RME, StreamingTexture* pST, const StagingTexture& StagingTexture, BYTE* pMinLODStagingTexels)
{
    if (pST->NoMinLODUpdates)
    {
        return true;
    }

    const UINT32 MipCount = pST->FileHeader.MipLevelCount;
    const UINT32 SliceIndex = RME.SubresourceIndex / MipCount;
    const UINT32 MipIndex = RME.SubresourceIndex % MipCount;
    const UINT32 RowPitchBytes = StagingTexture.CopyLocation.PlacedFootprint.Footprint.RowPitch;
    const UINT32 SlicePitchBytes = RowPitchBytes * StagingTexture.CopyLocation.PlacedFootprint.Footprint.Height;

    const UINT32 RectWidth = 1U << MipIndex;
    const UINT32 RectHeight = 1U << MipIndex;
    const UINT32 MapXPos = RME.TileX << MipIndex;
    const UINT32 MapYPos = RME.TileY << MipIndex;
    assert(MapXPos + RectWidth <= StagingTexture.CopyLocation.PlacedFootprint.Footprint.Width);
    assert(MapYPos + RectHeight <= StagingTexture.CopyLocation.PlacedFootprint.Footprint.Height);

    BYTE NewValue = (BYTE)(MipIndex + 1) << FRACTIONAL_LOD_SHIFT;
    BYTE OldValue = (BYTE)MipIndex << FRACTIONAL_LOD_SHIFT;
    if (!RME.IsUnmap)
    {
        BYTE Temp = NewValue;
        NewValue = OldValue;
        OldValue = Temp;

        const UINT64 DeltaTicks = m_CurrentTimeTicks.QuadPart - RME.LoadStartTicks;
        m_TileLoadToCompleteLatency.AddSample(DeltaTicks);
    }

    LogString("MinLOD %smap texture %u subresource %u XYZ <%u %u %u>", RME.IsUnmap ? "un" : "", pST->TextureUniqueIndex, (UINT32)RME.SubresourceIndex, (UINT32)RME.TileX, (UINT32)RME.TileY, (UINT32)RME.TileZ);

    BYTE* pRow = pMinLODStagingTexels + SliceIndex * SlicePitchBytes + MapYPos * RowPitchBytes;
    for (UINT32 y = 0; y < RectHeight; ++y)
    {
        BYTE* pTexel = pRow + MapXPos;
        for (UINT32 x = 0; x < RectWidth; ++x)
        {
            //assert(pTexel[x] == OldValue);
            pTexel[x] = NewValue;
        }
        pRow += RowPitchBytes;
    }

    return true;
}

UINT64 StreamingTextureManager::GetAgeOutTime() const
{
    UINT64 AgedOutFrameDelta = m_AgeOutTickCount >> m_AgeOutUrgentShift;
    if (m_FlushRequested)
    {
        AgedOutFrameDelta = 0;
    }

    if (m_CurrentTimeTicks.QuadPart < AgedOutFrameDelta)
    {
        return 0;
    }

    return m_CurrentTimeTicks.QuadPart - AgedOutFrameDelta;
}

void StreamingTextureManager::AgeOutTiles(ID3D12CommandQueue* pCmdQueue)
{
    PIX_CMDQUEUE_BLOCK(pCmdQueue, L"AgeOutTiles");

    if (m_PauseAgeOut && !m_FlushRequested)
    {
        return;
    }

    const UINT64 AgedOutTimeTicks = GetAgeOutTime();
    if (AgedOutTimeTicks == 0)
    {
        return;
    }

    UINT32 UnmappingCount = m_MaxTileMappingsPerFrame;
    SingleTileUnmapRequestVector UnmapRequests;

    ScopedCritSec Lock(&m_StreamingListsCritSec);

    for (UINT32 CompleteListIndex = 0; CompleteListIndex < ARRAYSIZE(m_CompleteTiles); ++CompleteListIndex)
    {
        TrackedTileList& CompleteTiles = m_CompleteTiles[CompleteListIndex];
        CompleteTiles.sort(CompletedSortPredicate);

        while (!CompleteTiles.empty() && UnmappingCount > 0)
        {
            TrackedTile* pTT = CompleteTiles.front();
            assert(pTT->Status == TrackedTileStatus_Populated);

            if (pTT->Pinned ||
                pTT->ChildRefCount > 0 ||
                pTT->LastSeenTicks > AgedOutTimeTicks)
            {
                break;
            }

            CompleteTiles.pop_front();

            const UINT32 MipIndex = pTT->Coords.Subresource % pTT->pTexture->FileHeader.MipLevelCount;

            UnmapTileNow(UnmapRequests, pTT);
            --UnmappingCount;
            m_Stats.VirtualMappedTileCount--;
            m_Stats.AgeOutMappedCount++;

            const UINT32 StatsMipIndex = std::min(MipIndex, (UINT32)ARRAYSIZE(m_Stats.VirtualMappedTileCountPerMip) - 1);
            m_Stats.VirtualMappedTileCountPerMip[StatsMipIndex] -= 1;
        }
    }

    if (!UnmapRequests.empty())
    {
        std::sort(UnmapRequests.begin(), UnmapRequests.end(), SingleTileUnmapRequestPredicate);
        const UINT32 UnmapCount = (UINT32)UnmapRequests.size();
        static const UINT32 MaxUnmapsPerCall = 64;
        D3D12_TILED_RESOURCE_COORDINATE CoordArray[MaxUnmapsPerCall];
        D3D12_TILE_REGION_SIZE OneTileSizeArray[MaxUnmapsPerCall];
        static const D3D12_TILE_REGION_SIZE OneTile = { 1, FALSE, 1, 1, 1 };
        for (UINT32 i = 0; i < ARRAYSIZE(OneTileSizeArray); ++i)
        {
            OneTileSizeArray[i] = OneTile;
        }
        UINT32 CurrentUnmapCount = 0;
        ID3D12Resource* pCurrentTexture = nullptr;
        for (UINT32 i = 0; i < UnmapCount; ++i)
        {
            const SingleTileUnmapRequest& Request = UnmapRequests[i];

            if (Request.pResource != pCurrentTexture ||
                CurrentUnmapCount >= MaxUnmapsPerCall)
            {
                if (CurrentUnmapCount > 0 && pCurrentTexture != nullptr)
                {
                    UINT TileIndexInPool = 0;
                    D3D12_TILE_RANGE_FLAGS RangeFlag = D3D12_TILE_RANGE_FLAG_NULL;
                    UINT RangeTileCount = CurrentUnmapCount;

                #if SUPPORT_TIER_1
                    RangeFlag = D3D12_TILE_RANGE_FLAG_REUSE_SINGLE_TILE;
                    TileIndexInPool = m_Tier1DefaultTileIndex;
                #endif

                    pCmdQueue->UpdateTileMappings(
                        pCurrentTexture,
                        CurrentUnmapCount,
                        CoordArray,
                        OneTileSizeArray,
                        m_pTilePool,
                        1,
                        &RangeFlag,
                        &TileIndexInPool,
                        &RangeTileCount,
                        D3D12_TILE_MAPPING_FLAG_NONE);
                }
                CurrentUnmapCount = 0;
                pCurrentTexture = Request.pResource;
            }

            CoordArray[CurrentUnmapCount] = Request.Coord;
            ++CurrentUnmapCount;
        }

        if (CurrentUnmapCount > 0 && pCurrentTexture != nullptr)
        {
            UINT TileIndexInPool = 0;
            D3D12_TILE_RANGE_FLAGS RangeFlag = D3D12_TILE_RANGE_FLAG_NULL;
            UINT RangeTileCount = CurrentUnmapCount;

            pCmdQueue->UpdateTileMappings(
                pCurrentTexture,
                CurrentUnmapCount,
                CoordArray,
                OneTileSizeArray,
                m_pTilePool,
                1,
                &RangeFlag,
                &TileIndexInPool,
                &RangeTileCount,
                D3D12_TILE_MAPPING_FLAG_NONE);
        }
    }
}

void StreamingTextureManager::UnmapTileNow(SingleTileUnmapRequestVector& UnmapVector, TrackedTile* pTT)
{
//     static const D3D12_TILE_REGION_SIZE OneTile = { 1, FALSE, 1, 1, 1 };

    assert(pTT->ChildRefCount == 0);
    if (pTT->pParentTile != nullptr)
    {
        assert(pTT->pParentTile->ChildRefCount > 0);
        pTT->pParentTile->ChildRefCount--;
    }

    ID3D12Resource* pPrimaryTexture = pTT->pTexture->pTiledTextureResource;
    assert(pPrimaryTexture != nullptr);
//     UINT TileIndexInPool = 0;
//     D3D12_TILE_RANGE_FLAGS RangeFlag = D3D12_TILE_RANGE_FLAG_NULL;
//     UINT RangeTileCount = 1;
//     pCmdQueue->UpdateTileMappings(pPrimaryTexture, 1, &pTT->Coords, &OneTile, m_pTilePool, 1, &RangeFlag, &TileIndexInPool, &RangeTileCount, D3D12_TILE_MAPPING_FLAG_NO_HAZARD);
    SingleTileUnmapRequest Request = {};
    Request.pResource = pPrimaryTexture;
    Request.Coord = pTT->Coords;
    UnmapVector.push_back(Request);

    ReleaseTilePoolSlot(pTT->TileIndexInPool);

    {
        ScopedCritSec Lock(&m_AllTilesCritSec, &m_AllTilesOwningThreadID);
        StreamingTexture* pST = pTT->pTexture;
        pST->DeleteTrackedTile(pTT);
        m_AllTileCount--;
    }

    LogTileOperation(pTT, "Unmapping");

    // Queue unmapped tile for MinLOD map update
    MinLODMapEdit MinLODEdit(pTT, true);
    m_MinLODMapEdits.push_back(MinLODEdit);

    FreeTrackedTile(pTT);
}

void StreamingTextureManager::LoadSeenTiles()
{
    if (m_PauseLoading || m_FlushRequested)
    {
        return;
    }

    const UINT64 AgeOutTime = GetAgeOutTime();

    ScopedCritSec Lock(&m_StreamingListsCritSec);

    ScopedCritSec AsyncLock(&m_AsyncLoadCritSec);
    UINT32 AsyncLoadIndex = 0;

    bool NoFreeTiles = false;

    UINT32 LoadCount = m_MaxTileLoadsPerFrame;
    for (INT32 i = ARRAYSIZE(m_SeenTiles) - 1; i >= 0; --i)
    {
        TrackedTileQueue& SeenTiles = m_SeenTiles[i];
        auto iter = SeenTiles.begin();
        auto end = SeenTiles.end();
        while (iter != end && LoadCount > 0)
        {
            TrackedTile* pTT = *iter;
            auto nextiter = iter;
            ++nextiter;

            if (pTT->LastSeenTicks < AgeOutTime &&
                pTT->ChildRefCount == 0)
            {
                SeenTiles.erase(iter);
                m_Stats.SeenTileCount--;
                m_Stats.AgeOutSeenCount++;
                DiscardExpiredSeenTile(pTT);
                iter = nextiter;
                continue;
            }

            if (NoFreeTiles)
            {
                break;
            }

            while (AsyncLoadIndex < ARRAYSIZE(m_AsyncLoads) && m_AsyncLoads[AsyncLoadIndex].IsAssignedToRead())
            {
                ++AsyncLoadIndex;
            }
            if (AsyncLoadIndex >= ARRAYSIZE(m_AsyncLoads))
            {
                m_Bottlenecks.AsyncLoadSlots = 1;
                break;
            }

            USHORT TilePoolTile = RequestTilePoolSlot(false);
            if (TilePoolTile == BlockAllocator::NULL_BLOCK_INDEX)
            {
                m_Bottlenecks.TilePoolTiles = 1;
                NoFreeTiles = true;
                break;
            }

            UINT32 CompressedLoadSlot = RequestCompressedLoadSlot(false);
            if (CompressedLoadSlot == BlockAllocator::NULL_BLOCK_INDEX)
            {
                m_Bottlenecks.CompressedBuffers = 1;
                ReleaseTilePoolSlot(TilePoolTile);
                break;
            }

            SeenTiles.erase(iter);
            m_Stats.SeenTileCount--;

            // Capture seen to load latency
            const UINT64 DeltaTicks = m_CurrentTimeTicks.QuadPart - pTT->StartTicks.QuadPart;
            m_TileSeenToLoadLatency.AddSample(DeltaTicks);

            // Start new timing for load to complete latency
            pTT->StartTicks.QuadPart = m_CurrentTimeTicks.QuadPart;

            assert(pTT->Status == TrackedTileStatus_Seen);
            pTT->TileIndexInPool = TilePoolTile;
            pTT->LoadSlotIndex = CompressedLoadSlot;
            pTT->Pinned = 0;
            pTT->MipTail = 0;
            QueueTrackedTileForLoading(pTT, &m_AsyncLoads[AsyncLoadIndex]);
            --LoadCount;
            ++AsyncLoadIndex;

            iter = nextiter;
        }
    }

    if (NoFreeTiles)
    {
        m_AgeOutUrgentShift = std::min(8U, m_AgeOutUrgentShift + 1);
    }
    else if (m_AgeOutUrgentShift > 0)
    {
        --m_AgeOutUrgentShift;
    }
}

void StreamingTextureManager::DiscardExpiredSeenTile(TrackedTile* pTT)
{
    assert(pTT->Status == TrackedTileStatus_Seen);
    assert(pTT->ChildRefCount == 0);

    if (pTT->pParentTile != nullptr)
    {
        assert(pTT->pParentTile->ChildRefCount > 0);
        pTT->pParentTile->ChildRefCount--;
    }

    {
        ScopedCritSec Lock(&m_AllTilesCritSec, &m_AllTilesOwningThreadID);
        StreamingTexture* pST = pTT->pTexture;
        pST->DeleteTrackedTile(pTT);
        m_AllTileCount--;
    }

    FreeTrackedTile(pTT);
}

void StreamingTextureManager::ProcessReleasedSlots()
{
    // Process any pending releases of decompression slots:
    const UINT64 CurrentCompletedFence = m_pStreamingFence->GetCompletedValue();
    bool ScanComplete = false;
    while (!m_DecompressedReleaseQueue.empty() && !ScanComplete)
    {
        ScopedCritSec Lock(&m_StreamingListsCritSec);
        if (!m_DecompressedReleaseQueue.empty())
        {
            FenceTrackedSlot Slot = m_DecompressedReleaseQueue.front();
            if (Slot.CompletionFenceValue > CurrentCompletedFence)
            {
                ScanComplete = true;
            }
            else
            {
                m_DecompressedReleaseQueue.pop_front();
                m_DecompressedStagingAllocator.FreeBlock((USHORT)Slot.SlotIndex);
            }
        }
    }
}

void StreamingTextureManager::CheckAsyncLoadsForCompletion()
{
    ScopedCritSec Lock(&m_AsyncLoadCritSec);

    // Loop through the async load operations and look for ones that have completed:
    for (UINT32 i = 0; i < ARRAYSIZE(m_AsyncLoads); ++i)
    {
        AsynchronousLoadOperation& ALO = m_AsyncLoads[i];

        // Skip the async load operations that are not assigned to a read operation at the moment:
        if (!ALO.IsAssignedToRead())
        {
            continue;
        }

        if (ALO.IsComplete())
        {
            const UINT64 DeltaTicks = m_CurrentTimeTicks.QuadPart - ALO.GetStartTicks();
            if (DeltaTicks > 0)
            {
                m_StorageReadLatency.AddSample(DeltaTicks);
            }

            // This load has completed; mark the associated tracked tile as Loaded:
            TrackedTile* pTT = (TrackedTile*)ALO.GetContext();
            ScopedCritSec StreamingLock(&m_StreamingListsCritSec);
            assert(pTT->Status == TrackedTileStatus_Loading);
            pTT->Status = TrackedTileStatus_Loaded;

            m_TileBytesLoadedPerInterval += ALO.GetBufferSizeBytes();
            ++m_TilesLoadedPerInterval;

            // Reset the async load operation struct so it can be used again:
            ALO.Reset();
        }
    }
}

void StreamingTextureManager::ProcessCompletedLoadTiles(ID3D12CommandQueue* pCmdQueue)
{
    // For all of the tiles that have completed loading, schedule the next step (decompression to graphics staging memory):
    if (!m_LoadingTiles.empty())
    {
        ScopedCritSec Lock(&m_StreamingListsCritSec);

        m_LoadingTiles.sort(LoadCompleteSortPredicate);

        UINT32 TilePopulateCount = m_MaxTilePopulatesPerFrame;

        while (!m_LoadingTiles.empty() && TilePopulateCount > 0)
        {
            // Examine the first tracked tile in the loading list and determine if it has completed loading and mapping.
            // If it hasn't, then give up this frame:
            TrackedTile* pTT = m_LoadingTiles.front();
            if (pTT->Status != TrackedTileStatus_Loaded /* || !pTT->IsMapped*/)
            {
                break;
            }

            // Attempt to allocate a decompressed staging buffer. If we don't have any available, give up:
            UINT32 DecompressedStagingSlot = RequestDecompressedStagingSlot();
            if (DecompressedStagingSlot == BlockAllocator::NULL_BLOCK_INDEX)
            {
                m_Bottlenecks.DecompressedBuffers = 1;
                break;
            }

            m_LoadingTiles.pop_front();
            --TilePopulateCount;
            m_Stats.LoadingTileCount--;

            FindOrCreateTilePoolAtlas(&pTT->pTexture->FileHeader, pCmdQueue);

            // Schedule a job to decompress the tile:
            g_JobQueue.AddJob((JobWorkerFunction)WorkerDecompressTile, this, pTT, (void*)DecompressedStagingSlot);
        }
    }
}

void StreamingTextureManager::ProcessPendingTileMappings(ID3D12CommandQueue* pCmdQueue)
{
    static const D3D12_TILE_REGION_SIZE OneTile = { 1, FALSE, 1, 1, 1 };

    // For all of the tiles needing mapping to their resource, process that queue now:
    UINT32 MappingCount = m_MaxTileMappingsPerFrame;
    while (!m_NeedsMappedTiles.empty() && MappingCount > 0)
    {
        TrackedTile* pTile = nullptr;
        {
            ScopedCritSec Lock(&m_StreamingListsCritSec);
            if (!m_NeedsMappedTiles.empty())
            {
                pTile = m_NeedsMappedTiles.front();
                m_NeedsMappedTiles.pop_front();
            }
        }
        if (pTile != nullptr)
        {
            assert(!pTile->IsMapped);

            ID3D12Resource* pPrimaryTexture = pTile->pTexture->pTiledTextureResource;
            assert(pPrimaryTexture != nullptr);
            UINT TileIndexInPool = (UINT)pTile->TileIndexInPool;
            D3D12_TILE_RANGE_FLAGS RangeFlag = D3D12_TILE_RANGE_FLAG_NONE;
            UINT RangeTileCount = 1;
            pCmdQueue->UpdateTileMappings(pPrimaryTexture, 1, &pTile->Coords, &OneTile, m_pTilePool, 1, &RangeFlag, &TileIndexInPool, &RangeTileCount, D3D12_TILE_MAPPING_FLAG_NONE);

            pTile->IsMapped = true;
            --MappingCount;

            if (!pTile->MipTail)
            {
                ScopedCritSec Lock(&m_StreamingListsCritSec);

                const UINT32 CompleteListIndex = ComputeCompleteListIndex(pTile);
                m_CompleteTiles[CompleteListIndex].push_back(pTile);

                // Queue MinLOD map update:
                MinLODMapEdit MinLODEdit(pTile, false);
                MinLODEdit.DelayCount = m_MinLODMapEditDelayCount;
                m_MinLODMapEdits.push_back(MinLODEdit);
            }
        }
    }
}

UINT32 StreamingTextureManager::ComputeCompleteListIndex(const TrackedTile* pTT) const
{
    const UINT32 MipIndex = pTT->LastSeenFractionalLOD >> FRACTIONAL_LOD_SHIFT;
    const UINT32 CompleteListIndex = std::min(MipIndex, (UINT32)ARRAYSIZE(m_CompleteTiles) - 1);
    return CompleteListIndex;
}

void StreamingTextureManager::Render(ID3D12GraphicsCommandList* pCmdList)
{
    PIX_CMDLIST_BLOCK(pCmdList, L"RenderStreaming");

    CpuProbeWriter Probes(m_Stats.RenderProbes, ARRAYSIZE(m_Stats.RenderProbes));

    RenderTileDataCopies(pCmdList);

    Probes.WriteProbe("Render Tile Data Copies");

    // Process MinLOD maps to apply new edits that were the result of the 
    // new mappings during RenderTileDataCopies:
    const UINT64 MinLODMapEditCount = (UINT64)m_MinLODMapEdits.size();
    ProcessNativeMinLODMapEdits();

    Probes.WriteProbe("Process MinLOD Map New Mappings", MinLODMapEditCount);

    CombineMinLODMaps();

    Probes.WriteProbe("Combine MinLOD Maps");

    {
        PIX_CMDLIST_BLOCK(pCmdList, L"Feedback Readback & Reset");

        ScopedCritSec Lock(&m_TextureDBCritSec);

        UINT64 OperationCount = 0;
    #if XBOX_SAMPLER_FEEDBACK
        OperationCount = (UINT64)m_FeedbackResourceDB.size();
        for (OpaqueTextureArray* pTexArray : m_FeedbackResourceDB)
        {
            SynchronizeFeedbackArray(pCmdList, pTexArray);
        }
    #else
        // Synchronize all feedback maps in one fell swoop:
        ResourceBarrier(pCmdList, m_pFeedbackBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        pCmdList->CopyResource(m_pFeedbackStagingBuffer, m_pFeedbackBuffer);
        ResourceBarrier(pCmdList, m_pFeedbackBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    #endif

        if (m_pFeedbackBuffer != nullptr)
        {
            const UINT ClearValues[4] = { -1, -1, -1, -1 };
            pCmdList->ClearUnorderedAccessViewUint(m_ghFeedbackBufferUAV, m_chFeedbackBufferUAV, m_pFeedbackBuffer, ClearValues, 0, nullptr);
        }

        Probes.WriteProbe("Feedback Map Readback and Reset", OperationCount);

        OperationCount = 0;
        for (auto& TexturePair : m_TextureDB)
        {
            bool Updated = SynchronizeNativeMinLODMap(pCmdList, TexturePair.second);
            if (Updated)
            {
                ++OperationCount;
            }
        }

        Probes.WriteProbe("Synchronize MinLOD Maps", OperationCount);
    }
}

void StreamingTextureManager::RenderTileDataCopies(ID3D12GraphicsCommandList* pCmdList)
{
    PIX_CMDLIST_BLOCK(pCmdList, L"RenderTileDataCopies");

    if (m_NeedsPopulatedTiles.empty())
    {
        return;
    }

    static const D3D12_TILE_REGION_SIZE OneTile = { 1, FALSE, 1, 1, 1 };

    D3D12_TEXTURE_COPY_LOCATION DecompressedStagingSrcLocation = {};
    DecompressedStagingSrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    DecompressedStagingSrcLocation.pResource = m_pDecompressedStagingResource;
    DecompressedStagingSrcLocation.PlacedFootprint.Footprint.Depth = 1;

    D3D12_BOX SrcBox = {};
    SrcBox.back = 1;

    UINT32 PopulateCount = std::min(m_MaxTilePopulatesPerFrame, (UINT32)m_NeedsPopulatedTiles.size());
    while (!m_NeedsPopulatedTiles.empty() && PopulateCount > 0)
    {
        TrackedTile* pTT = nullptr;
        {
            ScopedCritSec Lock(&m_StreamingListsCritSec);
            if (!m_NeedsPopulatedTiles.empty())
            {
                pTT = m_NeedsPopulatedTiles.front();
                m_NeedsPopulatedTiles.pop_front();
            }
        }
        if (pTT != nullptr)
        {
            assert(pTT->Status == TrackedTileStatus_Decompressed);
            // Check if the parent tile has been populated already:
            if (pTT->pParentTile != nullptr)
            {
                if (pTT->pParentTile->Status != TrackedTileStatus_Populated)
                {
                    // Parent tile has not been populated, which means that this tile "raced" ahead of the parent tile in the load/decompress queues.
                    // Decrement the populate count for this frame and punt this tile to the end of the list; try again later.
                    --PopulateCount;
                    ScopedCritSec Lock(&m_StreamingListsCritSec);
                    m_NeedsPopulatedTiles.push_back(pTT);
                    continue;
                }
            }
            ID3D12Resource* pPrimaryTexture = pTT->pTexture->pTiledTextureResource;
            UINT64 DecompressedStagingOffsetBytes = pTT->LoadSlotIndex * TiledResourceImage::TILE_SIZE_BYTES;

            bool DeferDecompressedSlotRelease = true;
            if (pTT->MipTail)
            {
                ImageUploadPackedMipTail(pTT, pCmdList, GetDecompressedStagingSlotBuffer(pTT->LoadSlotIndex));
                DeferDecompressedSlotRelease = false;
            }
            else
            {
                D3D12_TILED_RESOURCE_COORDINATE DestCoord = pTT->Coords;
                if (1)
                {
                    TilePoolAtlas* pAtlas = FindOrCreateTilePoolAtlas(&pTT->pTexture->FileHeader, nullptr);
                    if (pAtlas != nullptr)
                    {
                        UINT32 SliceTiles = pAtlas->RowCount * pAtlas->ColumnCount;
                        const UINT32 TilePoolIndex = pTT->TileIndexInPool;
                        DestCoord.Subresource = TilePoolIndex / SliceTiles;
                        DestCoord.X = TilePoolIndex % pAtlas->ColumnCount;
                        DestCoord.Y = (TilePoolIndex % SliceTiles) / pAtlas->ColumnCount;
                        pPrimaryTexture = pAtlas->pAtlasResource;
                    }
                }

                ResourceBarrier(pCmdList, pPrimaryTexture, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);

                if (0)
                {
                    const TiledResourceImage::Header& FH = pTT->pTexture->FileHeader;

                    UINT32 TileWidthTexels = 0;
                    UINT32 TileHeightTexels = 0;
                    UINT32 TileWidthElements = 0;
                    UINT32 TileHeightElements = 0;
                    TiledResourceImage::GetTileShape(&FH, &TileWidthElements, &TileHeightElements, &TileWidthTexels, &TileHeightTexels);

                    CD3DX12_TEXTURE_COPY_LOCATION DestLocation(pPrimaryTexture, DestCoord.Subresource);

                    SrcBox.right = TileWidthTexels;
                    SrcBox.bottom = TileHeightTexels;

                    D3D12_PLACED_SUBRESOURCE_FOOTPRINT& FP = DecompressedStagingSrcLocation.PlacedFootprint;
                    FP.Offset = DecompressedStagingOffsetBytes;
                    FP.Footprint.Format = (DXGI_FORMAT)FH.DXGIFormat;
                    FP.Footprint.Width = TileWidthTexels;
                    FP.Footprint.Height = TileHeightTexels;
                    FP.Footprint.RowPitch = TileWidthElements << FH.MipTail.ElementByteShift;
                    assert((FP.Footprint.RowPitch * TileHeightElements) == TiledResourceImage::TILE_SIZE_BYTES);

                    pCmdList->CopyTextureRegion(&DestLocation,
                        DestCoord.X * TileWidthTexels,
                        DestCoord.Y * TileHeightTexels,
                        0,
                        &DecompressedStagingSrcLocation,
                        &SrcBox);
                }
                else
                {
                    pCmdList->CopyTiles(
                        pPrimaryTexture,
                        &DestCoord,
                        &OneTile,
                        m_pDecompressedStagingResource,
                        DecompressedStagingOffsetBytes,
                        D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
                }

                ResourceBarrier(pCmdList, pPrimaryTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
            }

            {
                ScopedCritSec Lock(&m_StreamingListsCritSec);

                pTT->Status = TrackedTileStatus_Populated;

                if (!pTT->MipTail)
                {
                    // Non mip tail tiles still need to be mapped to their tiled resource:
                    m_NeedsMappedTiles.push_back(pTT);
                }

                if (DeferDecompressedSlotRelease)
                {
                    FenceTrackedSlot RDS = {};
                    RDS.CompletionFenceValue = m_NextFenceValue;
                    RDS.SlotIndex = pTT->LoadSlotIndex;
                    m_DecompressedReleaseQueue.push_back(RDS);
                }
                else
                {
                    m_DecompressedStagingAllocator.FreeBlock(pTT->LoadSlotIndex);
                }

                pTT->LoadSlotIndex = 0;

                m_Stats.VirtualMappedTileCount++;
                const UINT32 MipIndex = pTT->Coords.Subresource % pTT->pTexture->FileHeader.MipLevelCount;
                const UINT32 StatsMipIndex = std::min(MipIndex, (UINT32)ARRAYSIZE(m_Stats.VirtualMappedTileCountPerMip) - 1);
                m_Stats.VirtualMappedTileCountPerMip[StatsMipIndex] += 1;
                if (pTT->MipTail)
                {
                    m_Stats.VirtualAccessedTileCountPerMip[StatsMipIndex] += 1;
                }
            }

            --PopulateCount;

            LONG NewCount = InterlockedDecrement(&m_TileCountInFlight);
            assert(NewCount >= 0);
        }
    }
}

StreamingTextureStatistics StreamingTextureManager::GetStatistics() const
{
    StreamingTextureStatistics Stats = m_Stats;
    Stats.CompressedLoadTileCount = m_CompressedStagingAllocator.GetAllocatedCount();
    Stats.DecompressingTileCount = m_DecompressedStagingAllocator.GetAllocatedCount();
    Stats.TilePoolAllocatedTiles = m_TilePoolFreeTiles.GetAllocatedCount();
    Stats.InFlightTileCount = (UINT32)m_TileCountInFlight;
    Stats.AllTileCount = m_AllTileCount;
    Stats.Bottlenecks.DwordValue = m_Bottlenecks.DwordValue;
    Stats.MinLODMapEditCount = m_MinLODMapEdits.size();
    Stats.TotalOverheadMemoryBytes = Stats.TrackingStructMemoryBytes +
                                     Stats.MinLODResourceMemoryBytes +
                                     Stats.MinLODStagingMemoryBytes +
                                     Stats.FeedbackResourceMemoryBytes +
                                     Stats.FeedbackStagingMemoryBytes +
                                     Stats.TileStagingMemoryBytes;
    return Stats;
}

bool StreamingTextureManager::SynchronizeNativeMinLODMap(ID3D12GraphicsCommandList* pCmdList, StreamingTexture* pST)
{
    if (pST->MinLODTexture.IsDirty)
    {
        LogString("Updating native MinLOD map texture %u", (UINT32)pST->TextureUniqueIndex);

        pST->MinLODTexture.IsDirty = 0;

        ID3D12Resource* pMinLODTexture = pST->MinLODTexture.pTextureResource;
        assert(pMinLODTexture != nullptr);

        ResourceBarrier(pCmdList, pMinLODTexture, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);

        const D3D12_TEXTURE_COPY_LOCATION SrcLocation = pST->MinLODTexture.Staging.CopyLocation;
        const BYTE* pSrcBytes = (const BYTE*)pST->MinLODTexture.Staging.pLockedBits;
        const UINT64 SrcSliceSizeBytes = SrcLocation.PlacedFootprint.Footprint.RowPitch * SrcLocation.PlacedFootprint.Footprint.Height;

        for (UINT32 i = 0; i < pST->FileHeader.SliceCount; ++i)
        {
            m_TextureUploadHeap.CopyTextureSubresourceToDefaultTexture(
                pSrcBytes,
                SrcLocation.PlacedFootprint.Footprint.RowPitch,
                SrcLocation.PlacedFootprint.Footprint,
                pCmdList,
                pMinLODTexture,
                i,
                0, 0, 0,
                nullptr);

            pSrcBytes += SrcSliceSizeBytes;
        }

        ResourceBarrier(pCmdList, pMinLODTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

        return true;
    }

    return false;
}

#if XBOX_SAMPLER_FEEDBACK
void StreamingTextureManager::SynchronizeFeedbackArray(ID3D12GraphicsCommandList* pCmdList, OpaqueTextureArray* pTexArray)
{
    ID3D12Resource* pOpaqueFeedbackResource = pTexArray->pResource;
    const UINT32 SliceCount = pTexArray->Desc.DepthOrArraySize;

    ResourceBarrier(pCmdList, pOpaqueFeedbackResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

    pCmdList->SetComputeRootSignature(m_pFeedbackResolveSignature);
    pCmdList->SetPipelineState(m_pFeedbackResolvePSO);

    pCmdList->SetComputeRootDescriptorTable(0, pTexArray->hUAV);

    pCmdList->SetComputeRootDescriptorTable(1, m_SharedGpuDescriptorHeap.hGPU(1));

    const UINT32 DestOffsetElements = pTexArray->StagingBaseOffsetElements;

    const UINT32 FeedbackWidthElements = pTexArray->Desc.Width / pTexArray->Desc.SamplerFeedbackMipRegion.Width;
    const UINT32 FeedbackHeightElements = pTexArray->Desc.Height / pTexArray->Desc.SamplerFeedbackMipRegion.Height;

    XMUINT4* pRowSliceStride = nullptr;
    m_TextureUploadHeap.AllocateComputeRootConstantBuffer(&pRowSliceStride, pCmdList, 2);
    pRowSliceStride->x = FeedbackWidthElements;
    pRowSliceStride->y = FeedbackWidthElements * FeedbackHeightElements;
    pRowSliceStride->z = FeedbackHeightElements;
    pRowSliceStride->w = DestOffsetElements;

    const UINT32 ThreadGroupX = (FeedbackWidthElements + 7) >> 3;
    const UINT32 ThreadGroupY = (FeedbackHeightElements + 7) >> 3;
    const UINT32 ThreadGroupZ = pTexArray->AllocatedSlices;

    pCmdList->Dispatch(ThreadGroupX, ThreadGroupY, ThreadGroupZ);

    ResourceBarrier(pCmdList, pOpaqueFeedbackResource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}
#endif

void StreamingTextureManager::TrackMipLevel(StreamingTexture* pST, UINT32 SliceIndex, UINT32 LOD)
{
    const TiledResourceImage::Header& FH = pST->FileHeader;
    if (SliceIndex >= FH.SliceCount)
    {
        return;
    }
    if (LOD >= FH.MipLevelCount)
    {
        return;
    }
    if (FH.HasPackedMipTail() && LOD >= FH.MipTail.FirstMipIndex)
    {
        return;
    }
    ScopedCritSec Lock(&m_AllTilesCritSec, &m_AllTilesOwningThreadID);
    const UINT32 FractionalLOD = LOD << FRACTIONAL_LOD_SHIFT;
    for (UINT32 y = 0; y <= FH.MipLevels[LOD].HeightTilesM1; ++y)
    {
        for (UINT32 x = 0; x <= FH.MipLevels[LOD].WidthTilesM1; ++x)
        {
            FindAndTrackRegion(pST, SliceIndex, FractionalLOD, x, y, 0);
        }
    }
}

void StreamingTextureManager::FindAndTrackRegion2D(StreamingTexture* pST, UINT32 SliceIndex, FLOAT LOD, FLOAT TextureU, FLOAT TextureV)
{
    const UINT32 EndMipIndex = pST->FirstNonStreamingMipFractionalLOD >> FRACTIONAL_LOD_SHIFT;

    const UINT32 MostDetailedFractionalLOD = (UINT32)(LOD * (FLOAT)(1U << FRACTIONAL_LOD_SHIFT));
    const UINT32 StartMipIndex = MostDetailedFractionalLOD >> FRACTIONAL_LOD_SHIFT;
    if (StartMipIndex >= EndMipIndex)
    {
        return;
    }

    ScopedCritSec Lock(&m_AllTilesCritSec, &m_AllTilesOwningThreadID);
    const TiledResourceImage::MipLevel& Mip = pST->FileHeader.MipLevels[StartMipIndex];
    const UINT32 TileX = std::min((UINT32)(TextureU * (FLOAT)(Mip.WidthTilesM1 + 1)), (UINT32)Mip.WidthTilesM1);
    const UINT32 TileY = std::min((UINT32)(TextureV * (FLOAT)(Mip.HeightTilesM1 + 1)), (UINT32)Mip.HeightTilesM1);
    FindAndTrackRegion(pST, SliceIndex, MostDetailedFractionalLOD, TileX, TileY, 0);
}

void StreamingTextureManager::FindAndTrackRegion(StreamingTexture* pST, UINT32 SliceIndex, UINT32 MostDetailedFractionalLOD, UINT32 BaseMipTileX, UINT32 BaseMipTileY, UINT32 BaseMipTileZ)
{
    const INT32 EndMipIndex = (INT32)(pST->FirstNonStreamingMipFractionalLOD >> FRACTIONAL_LOD_SHIFT);

    const INT32 StartMipIndex = (INT32)(MostDetailedFractionalLOD >> FRACTIONAL_LOD_SHIFT);

    TrackedTile* FindLog[15] = {};

    TrackedTile* pParentTile = nullptr;
    for (INT32 MipIndex = EndMipIndex - 1; MipIndex >= StartMipIndex; --MipIndex)
    {
        assert(MipIndex >= 0);
        UINT32 FractionalLOD = (UINT32)MipIndex << FRACTIONAL_LOD_SHIFT;
        if (MipIndex == StartMipIndex)
        {
            FractionalLOD = MostDetailedFractionalLOD;
        }
        UINT32 TileX = BaseMipTileX >> MipIndex;
        UINT32 TileY = BaseMipTileY >> MipIndex;
        UINT32 TileZ = BaseMipTileZ >> MipIndex;
        pParentTile = FindAndTrackTile(pST, SliceIndex, FractionalLOD, TileX, TileY, TileZ, pParentTile);
        FindLog[MipIndex] = pParentTile;
    }
}

TrackedTile* StreamingTextureManager::FindAndTrackTile(StreamingTexture* pST, UINT32 SliceIndex, UINT32 FractionalLOD, UINT32 MipTileX, UINT32 MipTileY, UINT32 MipTileZ, TrackedTile* pParentTile)
{
    const UINT32 MipIndex = (FractionalLOD & 0xFF) >> FRACTIONAL_LOD_SHIFT;
    if (pST->FileHeader.HasPackedMipTail())
    {
        if (MipIndex >= pST->FileHeader.MipTail.FirstMipIndex)
        {
            return nullptr;
        }
    }
    else if (MipIndex >= pST->FileHeader.MipLevelCount)
    {
        return nullptr;
    }

    if (SliceIndex >= pST->FileHeader.SliceCount)
    {
        return nullptr;
    }

    TrackedTile* pTT = nullptr;

    assert(m_AllTilesOwningThreadID == GetCurrentThreadId());

    bool IsFirstAccess = false;
    TrackedTile** ppFound = pST->FindTrackedTile(MipIndex, SliceIndex, MipTileX, MipTileY, MipTileZ, &IsFirstAccess);
    if (ppFound == nullptr)
    {
        return nullptr;
    }
    if (*ppFound != nullptr)
    {
        assert(!IsFirstAccess);
        pTT = *ppFound;
        assert(pTT->pParentTile == pParentTile);
        pTT->LastSeenTicks = m_CurrentTimeTicks.QuadPart;
        return pTT;
    }

    if (IsFirstAccess)
    {
        const UINT32 StatsMipIndex = std::min(MipIndex, (UINT32)ARRAYSIZE(m_Stats.VirtualAccessedTileCountPerMip) - 1);
        m_Stats.VirtualAccessedTileCountPerMip[StatsMipIndex]++;
    }

    pTT = CreateTrackedTile();
    assert(pTT != nullptr);

    pTT->pTexture = pST;
    pTT->LastSeenFractionalLOD = FractionalLOD;
    pTT->LastSeenTicks = m_CurrentTimeTicks.QuadPart;
    pTT->Status = TrackedTileStatus_Seen;
    pTT->Coords.Subresource = SliceIndex * pST->FileHeader.MipLevelCount + MipIndex;
    pTT->Coords.X = MipTileX;
    pTT->Coords.Y = MipTileY;
    pTT->Coords.Z = MipTileZ;
    pTT->pParentTile = pParentTile;
    if (pParentTile != nullptr)
    {
        pParentTile->ChildRefCount++;
    }
    pTT->StartTicks.QuadPart = m_CurrentTimeTicks.QuadPart;

    QueueSeenTile(pTT, MipIndex);

    LogTileOperation(pTT, "Seen");

    *ppFound = pTT;
    ++m_AllTileCount;

    return pTT;
}

void StreamingTextureManager::QueueSeenTile(TrackedTile* pTT, UINT32 MipIndex)
{
    assert(pTT->MipTail == 0);
    const UINT32 SeenQueueIndex = std::min(MipIndex, (UINT32)ARRAYSIZE(m_SeenTiles) - 1);

    ScopedCritSec Lock(&m_StreamingListsCritSec);
    m_SeenTiles[SeenQueueIndex].push_back(pTT);
    m_Stats.SeenTileCount++;
}

//-------------------------------------------------------------------------------------------------
HANDLE StreamingTextureManager::ImplOpenTextureDataFile(const WCHAR* strFileName) const
{
    CREATEFILE2_EXTENDED_PARAMETERS ExtParams = {};
    ExtParams.dwSize = sizeof(ExtParams);
    ExtParams.dwFileFlags = FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING;
    HANDLE hFile = CreateFile2(strFileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &ExtParams);
    return hFile;
}

void StreamingTextureManager::ImplCloseTextureDataFile(HANDLE hFile) const
{
    CloseHandle(hFile);
}

bool StreamingTextureManager::ImplReadFileSynchronous(const WCHAR* strFileName, void* pBuffer, SIZE_T SizeBytes)
{
    assert(pBuffer != nullptr && SizeBytes > 0);

    UINT32 Size = (UINT32)SizeBytes;
    HRESULT hr = LoadFile(strFileName, &pBuffer, &Size);
    return SUCCEEDED(hr);
}

void StreamingTextureManager::ImageReadTextureFileHeader(StreamingTexture* pST)
{
    bool Success = ImplReadFileSynchronous(pST->strFileName, &pST->FileHeader, sizeof(pST->FileHeader));
    assert(Success);

    const TiledResourceImage::Header& FH = pST->FileHeader;
    assert(FH.MagicVersion == TiledResourceImage::CURRENT_VERSION);

    HRESULT hr;

    UINT32 TileWidthTexels = 0;
    UINT32 TileHeightTexels = 0;
    TiledResourceImage::GetTileShape(&FH, &pST->DebugTileWidthElements, &pST->DebugTileHeightElements, &TileWidthTexels, &TileHeightTexels);

    const TiledResourceImage::MipLevel& BaseMip = FH.MipLevels[0];

    const UINT32 WidthTexels = TileWidthTexels * (BaseMip.WidthTilesM1 + 1);
    const UINT32 HeightTexels = TileHeightTexels * (BaseMip.HeightTilesM1 + 1);

    // Create texture2D reserved resource
    CD3DX12_RESOURCE_DESC TexDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        (DXGI_FORMAT)FH.DXGIFormat,
        WidthTexels,
        HeightTexels,
        FH.SliceCount,
        FH.MipLevelCount,
        1,
        0,
        D3D12_RESOURCE_FLAG_NONE,
        D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE);

    hr = m_pd3dDevice->CreateReservedResource(&TexDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)(&pST->pTiledTextureResource));
    assert(SUCCEEDED(hr));

    WCHAR strTextureName[100];
    const WCHAR* strFileNameOnly = wcsrchr(pST->strFileName, L'\\');
    if (strFileNameOnly != nullptr)
    {
        ++strFileNameOnly;
    }
    else
    {
        strFileNameOnly = L"\0";
    }
    swprintf_s(strTextureName, L"Tiled Texture %u %s", pST->TextureUniqueIndex, strFileNameOnly);
    pST->pTiledTextureResource->SetName(strTextureName);

    m_pd3dDevice->CreateShaderResourceView(pST->pTiledTextureResource, nullptr, pST->hTiledTextureSRV);

    UINT32 FirstNonStreamingMipLevel = 0;
    if (FH.HasPackedMipTail())
    {
        FirstNonStreamingMipLevel = FH.MipTail.FirstMipIndex;
    }
    else
    {
        FirstNonStreamingMipLevel = FH.MipLevelCount;
    }
    pST->FirstNonStreamingMipFractionalLOD = FirstNonStreamingMipLevel << FRACTIONAL_LOD_SHIFT;

    CreateNativeMinLODTexture(pST, FirstNonStreamingMipLevel);
}

void StreamingTextureManager::CreateNativeMinLODTexture(StreamingTexture* pST, UINT32 DefaultLOD)
{
    WCHAR strTextureName[128];

    ZeroMemory(&pST->MinLODTexture, sizeof(pST->MinLODTexture));

    const TiledResourceImage::Header& FH = pST->FileHeader;
    const TiledResourceImage::MipLevel& BaseMip = FH.MipLevels[0];

    D3D12_RESOURCE_DESC MinLODDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, BaseMip.WidthTilesM1 + 1, BaseMip.HeightTilesM1 + 1, FH.SliceCount, 1);
    D3D12_RESOURCE_ALLOCATION_INFO AllocInfo = m_pd3dDevice->GetResourceAllocationInfo(D3D12XBOX_NODE_MASK, 1, &MinLODDesc);
    m_Stats.MinLODResourceMemoryBytes += (UINT32)AllocInfo.SizeInBytes;

    HRESULT hr = CreateDefaultResource(m_pd3dDevice, &MinLODDesc, D3D12_RESOURCE_STATE_GENERIC_READ, (void**)&pST->MinLODTexture.pTextureResource);
    assert(SUCCEEDED(hr));
    swprintf_s(strTextureName, L"Native MinLOD Texture %u", pST->TextureUniqueIndex);
    pST->MinLODTexture.pTextureResource->SetName(strTextureName);

    D3D12_CPU_DESCRIPTOR_HANDLE hSRV = pST->hNativeMinLODTextureSRV;
#if XBOX_SAMPLER_FEEDBACK
    m_pd3dDevice8->CreateMinMipShaderResourceViewX(pST->pTiledTextureResource, nullptr, pST->MinLODTexture.pTextureResource, nullptr, hSRV);
#else
    m_pd3dDevice->CreateShaderResourceView(pST->MinLODTexture.pTextureResource, nullptr, hSRV);
#endif

    StagingTexture& Staging = pST->MinLODTexture.Staging;
    bool Success = Staging.CreateCpuOnlyStaging(m_pd3dDevice, MinLODDesc);
    assert(Success);
    m_Stats.MinLODStagingMemoryBytes += (UINT32)Staging.CopyRange.End;

    DefaultLOD <<= FRACTIONAL_LOD_SHIFT;
    assert(DefaultLOD <= 0xFF);

    BYTE* pMinLODStagingTexels = (BYTE*)Staging.Map();
    assert(pMinLODStagingTexels != nullptr);
    pMinLODStagingTexels += Staging.CopyRange.Begin;
    const SIZE_T MinLODSizeBytes = Staging.CopyRange.End - Staging.CopyRange.Begin;
    memset(pMinLODStagingTexels, DefaultLOD, MinLODSizeBytes);
    Staging.Unmap();

    pST->MinLODTexture.IsDirty = 1;
}

void StreamingTextureManager::ImplCreateStandInTexture(StreamingTexture* pST)
{
    // TODO: based on pST->strFileName, generate a stand-in texture in pST. 
    // pST->hFile should remain INVALID_HANDLE_VALUE.

    m_pd3dDevice->CopyDescriptorsSimple(2, pST->hTiledTextureSRV, m_hDefaultTextureViews, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

bool StreamingTextureManager::ImageLoadPackedMips(StreamingTexture* pST)
{
    UINT32 PackedMipIndex = 0;
    UINT32 PackedMipSizeTiles = 0;
    TiledResourceImage::GetMipTailOffsetAndSize(&pST->FileHeader, &PackedMipIndex, &PackedMipSizeTiles);

    if (PackedMipSizeTiles == 0)
    {
        return true;
    }

    // Packed mips that span more than one tile are not yet supported...
    assert(PackedMipSizeTiles == 1);
    if (PackedMipSizeTiles > 1)
    {
        return true;
    }

    D3D12_TILED_RESOURCE_COORDINATE PackedMipCoord = {};
    PackedMipCoord.Subresource = PackedMipIndex;

    ScopedCritSec Lock(&m_AsyncLoadCritSec);
    UINT32 AsyncLoadIndex = 0;

    for (UINT32 i = 0; i < PackedMipSizeTiles; ++i)
    {
        while (m_AsyncLoads[AsyncLoadIndex].IsAssignedToRead())
        {
            ++AsyncLoadIndex;
            if (AsyncLoadIndex >= ARRAYSIZE(m_AsyncLoads))
            {
                m_Bottlenecks.AsyncLoadSlots = 1;
                return false;
            }
        }

        USHORT TilePoolTile = RequestTilePoolSlot(false);
        if (TilePoolTile == BlockAllocator::NULL_BLOCK_INDEX)
        {
            m_Bottlenecks.TilePoolTiles = 1;
            return false;
        }

        UINT32 CompressedLoadSlot = RequestCompressedLoadSlot(false);
        if (CompressedLoadSlot == BlockAllocator::NULL_BLOCK_INDEX)
        {
            m_Bottlenecks.CompressedBuffers = 1;
            ReleaseTilePoolSlot(TilePoolTile);
            return false;
        }

        TrackedTile* pTT = CreateTrackedTile();
        pTT->pTexture = pST;
        pTT->TileIndexInPool = TilePoolTile;
        pTT->Pinned = 1;
        pTT->MipTail = 1;
        pTT->Coords = PackedMipCoord;
        pTT->LastSeenFractionalLOD = PackedMipIndex << FRACTIONAL_LOD_SHIFT;
        pTT->LastSeenTicks = m_CurrentTimeTicks.QuadPart;
        pTT->LoadSlotIndex = CompressedLoadSlot;
        QueueTrackedTileForLoading(pTT, &m_AsyncLoads[AsyncLoadIndex]);
        ++AsyncLoadIndex;
    }

    return true;
}

void StreamingTextureManager::ImageLoadTile(TrackedTile* pTT, AsynchronousLoadOperation* pALO)
{
    UINT32 CompressedSlotIndex = pTT->LoadSlotIndex;
    BYTE* pCompressedBuffer = GetCompressedLoadSlotBuffer(CompressedSlotIndex);

    const StreamingTexture* pST = pTT->pTexture;
    UINT32 Mip = pTT->Coords.Subresource % pST->FileHeader.MipLevelCount;
    UINT32 Slice = pTT->Coords.Subresource / pST->FileHeader.MipLevelCount;

    bool IsPackedMipTail = false;
    UINT64 FileOffsetBytes = TiledResourceImage::ComputeFileOffsetBytes(&pST->FileHeader, Mip, Slice, pTT->Coords.X, pTT->Coords.Y, pTT->Coords.Z, &IsPackedMipTail);
    FileOffsetBytes += pST->FileOffsetBytes;

    UINT32 ReadSizeBytes = TiledResourceImage::TILE_SIZE_BYTES;
    if (IsPackedMipTail)
    {
        ReadSizeBytes = TiledResourceImage::GetMipTailSizeBytes(&pST->FileHeader);
        ReadSizeBytes = NextMultiple(ReadSizeBytes, TiledResourceImage::TILE_SIZE_BYTES);
    }

    assert((bool)pTT->MipTail == IsPackedMipTail);

    bool Success = pALO->BeginRead(pST->hFile, FileOffsetBytes, ReadSizeBytes, pCompressedBuffer, pTT, m_CurrentTimeTicks);
    assert(Success);

    assert(pTT->Status == TrackedTileStatus_Loading);
}

void StreamingTextureManager::ImageDecompressTile(TrackedTile* pTT, BYTE* pDecompressedBuffer, UINT32 DecompressedStagingSlotIndex)
{
    // Decompress from the compressed buffer to the decompressed buffer:
    const BYTE* pCompressedBuffer = GetCompressedLoadSlotBuffer(pTT->LoadSlotIndex);
    memcpy(pDecompressedBuffer, pCompressedBuffer, TiledResourceImage::TILE_SIZE_BYTES);

    if (m_MarkTileBoundaries && !pTT->MipTail)
    {
        StreamingTexture* pST = pTT->pTexture;
        ImageMarkTileBoundaries(pDecompressedBuffer, (DXGI_FORMAT)pST->FileHeader.DXGIFormat, pST->DebugTileWidthElements, pST->DebugTileHeightElements);
    }

    if (m_DecompressSleepMsec > 0 && !m_FlushRequested)
    {
        Sleep(m_DecompressSleepMsec);
    }

    // Release the compressed load slot so that another tile load can use it:
    ReleaseCompressedLoadSlot(pTT->LoadSlotIndex);

    // Mark the tile as occupying the decompressed slot now:
    pTT->LoadSlotIndex = DecompressedStagingSlotIndex;
    pTT->Status = TrackedTileStatus_Decompressed;

    // Add this tile to the list for population:
    {
        ScopedCritSec Lock(&m_StreamingListsCritSec);
        m_NeedsPopulatedTiles.push_back(pTT);
    }
}

void StreamingTextureManager::ImageUploadPackedMipTail(TrackedTile* pTT, ID3D12GraphicsCommandList* pCmdList, const BYTE* pDecompressedBuffer)
{
    assert(pTT->MipTail);
    const TiledResourceImage::Header& FH = pTT->pTexture->FileHeader;
    assert(FH.HasPackedMipTail());

    const UINT32 ElementSizeBytes = 1U << FH.MipTail.ElementByteShift;
    const UINT32 ElementSizeTexels = 1U << FH.MipTail.ElementSizeShift;

    ID3D12Resource* pTexture = pTT->pTexture->pTiledTextureResource;

    ResourceBarrier(pCmdList, pTexture, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);

    const BYTE* p = pDecompressedBuffer;
    D3D12_SUBRESOURCE_FOOTPRINT FP = {};
    FP.Format = (DXGI_FORMAT)FH.DXGIFormat;
    FP.Depth = 1;
    for (UINT32 MipIndex = FH.MipTail.FirstMipIndex; MipIndex < FH.MipLevelCount; ++MipIndex)
    {
        const UINT32 TailMipIndex = MipIndex - FH.MipTail.FirstMipIndex;
        const UINT32 SrcRowPitchBytes = std::max(ElementSizeBytes, FH.MipTail.FirstRowPitchBytes >> TailMipIndex);
        const UINT32 SrcRowCount = std::max(1U, FH.MipTail.FirstHeightRows >> TailMipIndex);
        FP.RowPitch = NextMultiple(SrcRowPitchBytes, (UINT32)D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT);
        TiledResourceImage::GetMipDimensions(&FH, MipIndex, &FP.Width, &FP.Height);

        for (UINT32 SliceIndex = 0; SliceIndex < FH.SliceCount; ++SliceIndex)
        {
            const UINT32 DestSubresource = SliceIndex * FH.MipLevelCount + MipIndex;
            m_TextureUploadHeap.CopyTextureSubresourceToDefaultTexture(p, SrcRowPitchBytes, FP, pCmdList, pTexture, DestSubresource, 0, 0, 0, nullptr);
            p += SrcRowPitchBytes * SrcRowCount;
        }
    }

    ResourceBarrier(pCmdList, pTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void StreamingTextureManager::ImageMarkTileBoundaries(BYTE* pDecompressedTile, DXGI_FORMAT Format, UINT32 TileWidthElements, UINT32 TileHeightElements)
{
    BYTE TexelBytes[16];
    BYTE* pTexelBytes = &TexelBytes[0];
    memset(pTexelBytes, 0xFF, sizeof(TexelBytes));
    SIZE_T TexelByteCount = 0;
    bool SimpleFill = false;

    switch (Format)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        TexelByteCount = 4;
        SimpleFill = true;
        break;
    case DXGI_FORMAT_R8_UNORM:
        TexelByteCount = 1;
        SimpleFill = true;
        break;
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        TexelByteCount = 8;
        ZeroMemory(&TexelBytes[4], 4);
        break;
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        TexelByteCount = 8;
        ZeroMemory(&TexelBytes[2], 6);
        break;
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        TexelByteCount = 16;
        ZeroMemory(&TexelBytes[12], 4);
        break;
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        TexelByteCount = 16;
        ZeroMemory(&TexelBytes[2], 6);
        ZeroMemory(&TexelBytes[12], 4);
        break;
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
        TexelByteCount = 16;
        ZeroMemory(&TexelBytes[2], 6);
        ZeroMemory(&TexelBytes[10], 6);
        break;
    default:
        // unsupported format; implement
        assert(FALSE);
        break;
    }

    if (TexelByteCount > 0)
    {
        const UINT32 RowByteCount = TileWidthElements * TexelByteCount;
        assert((RowByteCount * TileHeightElements) == TiledResourceImage::TILE_SIZE_BYTES);

        if (SimpleFill)
        {
            memset(pDecompressedTile, TexelBytes[0], RowByteCount);
            BYTE* pRow = pDecompressedTile + RowByteCount;
            for (UINT32 i = 1; i < TileHeightElements; ++i)
            {
                memset(pRow, TexelBytes[0], TexelByteCount);
                pRow += RowByteCount;
            }
        }
        else
        {
            BYTE* pTexel = pDecompressedTile;
            for (UINT32 i = 0; i < TileWidthElements; ++i)
            {
                memcpy(pTexel, pTexelBytes, TexelByteCount);
                pTexel += TexelByteCount;
            }
            BYTE* pRow = pDecompressedTile + RowByteCount;
            for (UINT32 i = 1; i < TileHeightElements; ++i)
            {
                memcpy(pRow, pTexelBytes, TexelByteCount);
                pRow += RowByteCount;
            }
        }
    }
}
