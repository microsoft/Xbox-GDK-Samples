//--------------------------------------------------------------------------------------
// TextureStreaming.h
//
// A tile-based texture streaming engine that supports multiple partially resident
// textures, using MinLOD maps and MinLOD feedback maps.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <windows.h>
#if defined(_GAMING_XBOX)
#include <d3d12_xs.h>
#else
#include <d3d12.h>
#endif

#include <vector>
#include <deque>
#include <unordered_map>
#include "TiledResourceImage.h"
#include "TextureStreamingUtils.h"

struct TrackedTile;
struct StreamingTexture;

struct LinkedStreamingTexture
{
    StreamingTexture* pTexture;
    union
    {
        struct  
        {
            INT32 ScaleShiftX : 6;
            INT32 ScaleShiftY : 6;
            INT32 MipLevelBias : 8;
            UINT32 FirstNonStreamingMipFractionalLOD : 8;
        };

        UINT32 Flags;
    };
};
typedef std::vector<LinkedStreamingTexture> LinkedStreamingTextureVector;

struct NativeMinLODTexture
{
    UINT32 IsDirty : 1;
    StagingTexture Staging;
    ID3D12Resource* pTextureResource;
};

struct SharedMinLODTexture
{
    ID3D12Resource* pTextureResource;
    D3D12_CPU_DESCRIPTOR_HANDLE hSRV;
    D3D12_CPU_DESCRIPTOR_HANDLE hUAV;
    LinkedStreamingTextureVector Links;
    UINT32 SRVDescriptorIndex : 16;
    UINT32 UAVDescriptorIndex : 16;
    UINT32 MapWidth : 16;
    UINT32 MapHeight : 16;

    void AddLinkedTexture(StreamingTexture* pTexture);
};
typedef std::list<SharedMinLODTexture*> SharedMinLODTextureList;

struct FeedbackTexture
{
    StagingTexture Staging;
    LinkedStreamingTextureVector Links;
    ID3D12Resource* pTextureResource;
    UINT32 FirstSliceIndexWithinResource;
    D3D12_CPU_DESCRIPTOR_HANDLE hUAV;
    UINT32 DescriptorIndex;

    void AddLinkedTexture(StreamingTexture* pTexture);
};
typedef std::list<FeedbackTexture*> FeedbackTextureList;

enum StreamingTextureComponentTextures
{
    STCT_PrimaryTexture = 0,
    STCT_MinLODTexture = 1,
    STCT_FeedbackTexture = 2,
    STCT_Count
};

struct StreamingTexture
{
    UINT32 RefCount;

    USHORT TextureUniqueIndex;

    UINT32 DebugTileWidthElements;
    UINT32 DebugTileHeightElements;

    WCHAR strFileName[MAX_PATH];
    UINT64 FileNameHash;
    HANDLE hFile;
    UINT64 FileOffsetBytes;

    TiledResourceImage::Header FileHeader;

    ID3D12Resource* pTiledTextureResource;
    D3D12_CPU_DESCRIPTOR_HANDLE hTiledTextureSRV;

    NativeMinLODTexture MinLODTexture;
    D3D12_CPU_DESCRIPTOR_HANDLE hNativeMinLODTextureSRV;

    UINT32 FirstNonStreamingMipFractionalLOD : 8;
    UINT32 NoMinLODUpdates : 1;

    // The contents of ppTileArray are protected by the m_AllTilesCritSec critical section in the streaming texture manager.
    TrackedTile** ppTileArray;
    UINT64 TileArraySize;
    UINT64* pTileAccessedMask;
    TrackedTile** FindTrackedTile(UINT32 MipIndex, UINT32 Slice, UINT32 TileX, UINT32 TileY, UINT32 TileZ, bool* pFirstAccess);
    bool DeleteTrackedTile(TrackedTile* pTT);
};
typedef std::deque<StreamingTexture*> StreamingTextureQueue;

union StreamingBottlenecks
{
    struct  
    {
        UINT32 TilePoolTiles : 1;
        UINT32 AsyncLoadSlots : 1;
        UINT32 DecompressedBuffers : 1;
        UINT32 CompressedBuffers : 1;
    };
    UINT32 DwordValue;
};

struct StreamingTextureStatistics
{
    UINT32 TextureCount;
    UINT32 VirtualTotalTileCount;
    UINT32 VirtualMappedTileCount;
    UINT32 TilePoolSizeTiles;
    UINT32 TilePoolAllocatedTiles;
    UINT32 SeenTileCount;
    UINT32 LoadingTileCount;
    UINT32 CompressedLoadTileCount;
    UINT32 DecompressingTileCount;
    UINT32 InFlightTileCount;
    UINT32 AgeOutMappedCount;
    UINT32 AgeOutSeenCount;
    UINT32 AllTileCount;
    UINT32 MinLODMapEditCount;

    UINT32 TrackingStructMemoryBytes;
    UINT32 MinLODResourceMemoryBytes;
    UINT32 MinLODStagingMemoryBytes;
    UINT32 FeedbackResourceMemoryBytes;
    UINT32 FeedbackStagingMemoryBytes;
    UINT32 TileStagingMemoryBytes;
    UINT32 TotalOverheadMemoryBytes;

    UINT32 VirtualTotalTileCountPerMip[7];
    UINT32 VirtualMappedTileCountPerMip[7];
    UINT32 VirtualAccessedTileCountPerMip[7];

    UINT32 KBytesLoadedPerSecond;
    UINT32 TileLoadsPerSecond;
    UINT32 PeakKBytesLoadedPerSecond;
    UINT32 PeakTileLoadsPerSecond;

    MinMaxAverage::Report StorageReadLatency;
    MinMaxAverage::Report TileSeenToLoadLatency;
    MinMaxAverage::Report TileLoadToCompleteLatency;

    StreamingBottlenecks Bottlenecks;

    LabeledCpuProbe UpdateProbes[16];
    LabeledCpuProbe RenderProbes[8];
};

struct TrackedTile
{
    volatile UINT32 Status;
    volatile bool IsMapped;

    UINT32 Pinned : 1;
    UINT32 MipTail : 1;
    UINT32 ChildRefCount : 6;
    UINT32 LastSeenFractionalLOD : 8;
    UINT32 LoadSlotIndex : 8;
    UINT64 LastSeenTicks;

    USHORT TileIndexInPool;
    StreamingTexture* pTexture;
    D3D12_TILED_RESOURCE_COORDINATE Coords;

    // The parent tile is the tile in the next smaller mip level that corresponds to the same area as this tile.
    // A packed mip level tile isn't linked by its "children" though.
    TrackedTile* pParentTile;

    LARGE_INTEGER StartTicks;
};
typedef std::list<TrackedTile*> TrackedTileList;
typedef std::deque<TrackedTile*> TrackedTileQueue;

struct TilePoolAtlas
{
    ID3D12Resource* pAtlasResource;
    UINT32 TileWidthTexels;
    UINT32 TileHeightTexels;
    UINT32 RowCount;
    UINT32 ColumnCount;
};

//-------------------------------------------------------------------------------------------------
// Streaming texture manager
//
// Tile flow:
// 1. Tiles detected in GPU feedback buffers are added to the "Seen" list via FindAndTrackTile
// 2. The Seen list is sorted by priority
// 3. The highest priority tiles on the Seen list are queued for loading, moving them to the Loading list
// 4. Asynchronously, the tiles' compressed bits are loaded and placed into Loaded state
// 5. Also asynchronously, the tiles' tile pool are mapped into the tiled resource
// 6. Loaded and mapped tiles are asynchronously decompressed into GPU staging memory, moving them to the Completed list
// 7. Aged-out tiles are removed from the Completed list, unmapped, and deleted

class StreamingTextureManager
{
private:
    enum TrackedTileStatus
    {
        TrackedTileStatus_Invalid = 0,
        TrackedTileStatus_Seen,
        TrackedTileStatus_Loading,
        TrackedTileStatus_Loaded,
        TrackedTileStatus_Decompressed,
        TrackedTileStatus_Populated,
    };

    struct MinLODMapEdit
    {
        StreamingTexture* pTexture;
        UINT64 SubresourceIndex : 16;
        UINT64 TileX : 8;
        UINT64 TileY : 8;
        UINT64 TileZ : 8;
        UINT64 DelayCount : 8;
        UINT64 IsUnmap : 1;
        UINT64 LoadStartTicks;

        MinLODMapEdit(const TrackedTile* pTT, bool IsUnmapEdit)
        {
            pTexture = pTT->pTexture;
            SubresourceIndex = pTT->Coords.Subresource;
            TileX = pTT->Coords.X;
            TileY = pTT->Coords.Y;
            TileZ = pTT->Coords.Z;
            IsUnmap = IsUnmapEdit ? 1 : 0;
            if (!IsUnmap)
            {
                LoadStartTicks = pTT->StartTicks.QuadPart;
            }
        }
    };
    typedef std::list<MinLODMapEdit> MinLODMapEditList;

    struct FenceTrackedSlot
    {
        UINT64 CompletionFenceValue;
        UINT32 SlotIndex;
    };
    typedef std::deque<FenceTrackedSlot> FenceTrackedSlotQueue;

    // A fractional LOD is an 8 bit unsigned 5.3 value:
    static const UINT32 FRACTIONAL_LOD_SHIFT = 3;

    struct SingleTileUnmapRequest
    {
        ID3D12Resource* pResource;
        D3D12_TILED_RESOURCE_COORDINATE Coord;
    };
    typedef std::vector<SingleTileUnmapRequest> SingleTileUnmapRequestVector;

#if XBOX_SAMPLER_FEEDBACK
    struct OpaqueTextureArray
    {
        D3D12_RESOURCE_DESC1 Desc;
        UINT32 WidthTexels;
        UINT32 HeightTexels;
        UINT32 AllocatedSlices;
        ID3D12Resource* pResource;
        UINT32 StagingBaseOffsetElements;
        UINT32 StagingSlicePitchElements;
        D3D12_GPU_DESCRIPTOR_HANDLE hUAV;
    };
    typedef std::vector<OpaqueTextureArray*> OpaqueResourceDB;
#endif

private:
    volatile LONG m_TileCountInFlight;

    ID3D12Device* m_pd3dDevice;
#if XBOX_SAMPLER_FEEDBACK
    ID3D12Device8* m_pd3dDevice8;
#endif
    ID3D12Fence* m_pStreamingFence;
    ID3D12Fence* m_pRenderingFence;
    UINT64 m_NextFenceValue;
    LARGE_INTEGER m_CurrentTimeTicks;
    LARGE_INTEGER m_PerfFreq;
    DescriptorHeapWrapper m_SharedGpuDescriptorHeap;

    StreamingTextureStatistics m_Stats;
    LARGE_INTEGER m_NextStatsTimeTicks;
    LARGE_INTEGER m_LastStatsTimeTicks;
    UINT64 m_TileBytesLoadedPerInterval;
    UINT64 m_TilesLoadedPerInterval;
    StreamingBottlenecks m_Bottlenecks;
    MinMaxAverage m_StorageReadLatency;
    MinMaxAverage m_TileSeenToLoadLatency;
    MinMaxAverage m_TileLoadToCompleteLatency;

    bool m_LoggingEnabled;

    // Throttle controls:
    bool m_PauseLoading;
    bool m_PauseAgeOut;
    UINT64 m_AgeOutTickCount;
    UINT32 m_AgeOutUrgentShift;
    bool m_FlushRequested;
    UINT32 m_MaxTileLoadsPerFrame;
    UINT32 m_MaxTileMappingsPerFrame;
    UINT32 m_MaxTilePopulatesPerFrame;
    UINT32 m_LoadSleepMsec;
    UINT32 m_DecompressSleepMsec;
    bool m_ReadFeedbackMaps;
    UINT32 m_RandomLoadsPerFrame;
    bool m_DoNotLoadUpperLeft;
    bool m_MarkTileBoundaries;

    CRITICAL_SECTION m_AsyncLoadCritSec;
    AsynchronousLoadOperation m_AsyncLoads[256];

    typedef std::unordered_multimap<UINT64, StreamingTexture*> StreamingTextureMap;
    StreamingTextureMap m_TextureDB;
    BlockAllocatedDescriptorHeap m_TextureCpuDescriptorHeap;
    CRITICAL_SECTION m_TextureDBCritSec;

    SharedMinLODTextureList m_SharedMinLODTextures;
    FeedbackTextureList m_FeedbackTextures;
#if XBOX_SAMPLER_FEEDBACK
    OpaqueResourceDB m_FeedbackResourceDB;
#endif
    BlockAllocatedDescriptorHeap m_MinLODFeedbackSRVUAVHeap;

    CRITICAL_SECTION m_AllTilesCritSec;
    DWORD m_AllTilesOwningThreadID;
    UINT32 m_AllTileCount;

    BlockAllocator m_TilePoolFreeTiles;
    ID3D12Heap* m_pTilePool;

    TilePoolAtlas* m_pTilePoolAtlas[128];

    CRITICAL_SECTION m_StreamingListsCritSec;
    StreamingTextureQueue m_PackedMipPendingQueue;
    TrackedTileList m_EmptyTrackedTileStructs;
    TrackedTileQueue m_SeenTiles[8];
    TrackedTileList m_LoadingTiles;
    TrackedTileList m_CompleteTiles[8];
    TrackedTileQueue m_NeedsMappedTiles;
    TrackedTileQueue m_NeedsPopulatedTiles;

    MinLODMapEditList m_MinLODMapEdits;
    UINT32 m_MinLODMapEditDelayCount;

#if SUPPORT_TIER_1
    USHORT m_Tier1DefaultTileIndex;
    StreamingTextureQueue m_DefaultTileMapPendingQueue;
#endif

    ID3D12Heap* m_pSharedFeedbackHeap;
    UINT64 m_SharedFeedbackHeapOffsetBytes;

    ID3D12Resource* m_pFeedbackBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE m_chFeedbackBufferUAV;
    D3D12_GPU_DESCRIPTOR_HANDLE m_ghFeedbackBufferUAV;
    UINT32 m_FeedbackBufferSizeElements;
    UINT32 m_CurrentFeedbackOffsetElements;

    ID3D12Resource* m_pFeedbackStagingBuffer;
    D3D12_RANGE m_FeedbackStagingRange;
    const UINT32* m_pFeedbackStagingData;

    BlockAllocator m_CompressedStagingAllocator;
    BYTE* m_pCompressedStagingBuffer;

    BlockAllocator m_DecompressedStagingAllocator;
    ID3D12Resource* m_pDecompressedStagingResource;
    BYTE* m_pDecompressedStagingBuffer;
    FenceTrackedSlotQueue m_DecompressedReleaseQueue;

    CpuGpuHeap m_TextureUploadHeap;

    D3D12_CPU_DESCRIPTOR_HANDLE m_hDefaultTextureViews;
    ID3D12Resource* m_pDefaultTextureObjects[STCT_Count];

#if XBOX_SAMPLER_FEEDBACK
    ID3D12RootSignature* m_pFeedbackResolveSignature;
    ID3D12PipelineState* m_pFeedbackResolvePSO;
#endif

public:
    void Initialize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pInitCmdList, UINT32 TilePoolTileCount, UINT32 MaxTextureCount = 10000);
    StreamingTexture* LoadTexture(const WCHAR* strFileName, const WCHAR* strDataFileName, HANDLE hDataFile = NULL, UINT64 DataFileOffset = 0);
    void UnloadTexture(StreamingTexture* pST);
    StreamingTexture* FindTextureByUniqueID(UINT32 UniqueID);

    FeedbackTexture* CreateFeedbackTexture(StreamingTexture* pST);
    FeedbackTexture* CreateFeedbackTexture(UINT32 Width, UINT32 Height, UINT32 TileWidth, UINT32 TileHeight, UINT32 SliceCount);
    SharedMinLODTexture* CreateSharedMinLODTexture(UINT32 Width, UINT32 Height, UINT32 SliceCount);

    void FindAndTrackRegion2D(StreamingTexture* pST, UINT32 SliceIndex, FLOAT LOD, FLOAT TextureU, FLOAT TextureV);
    void FindAndTrackRegion(StreamingTexture* pST, UINT32 SliceIndex, UINT32 MostDetailedFractionalLOD, UINT32 ResidencyTileX, UINT32 ResidencyTileY, UINT32 ResidencyTileZ);
    void TrackMipLevel(StreamingTexture* pST, UINT32 SliceIndex, UINT32 LOD);

    void UpdateStreaming(ID3D12CommandQueue* pUpdateQueue, ID3D12CommandQueue* pRenderQueue = nullptr);
    void Render(ID3D12GraphicsCommandList* pCmdList);

    StreamingTextureStatistics GetStatistics() const;

    void FreezeAgeOut(bool Frozen) { m_PauseAgeOut = Frozen; }
    bool IsAgeOutFrozen() const { return m_PauseAgeOut; }
    void FreezeLoading(bool Frozen) { m_PauseLoading = Frozen; }
    bool IsLoadingFrozen() const { return m_PauseLoading; }
    UINT32 GetLoadSleepMsec() const { return m_LoadSleepMsec; }
    void SetLoadSleepMsec(UINT32 Msec) { m_LoadSleepMsec = Msec; }
    UINT32 GetDecompressSleepMsec() const { return m_DecompressSleepMsec; }
    void SetDecompressSleepMsec(UINT32 Msec) { m_DecompressSleepMsec = Msec; }
    void SetAgeOutTimeMsec(UINT32 Msec);
    void SetRandomLoadsPerFrame(UINT32 Count) { m_RandomLoadsPerFrame = Count; }
    UINT32 GetRandomLoadsPerFrame() const { return m_RandomLoadsPerFrame; }
    void SetDoNotLoadUpperLeft(bool DoNotLoad) { m_DoNotLoadUpperLeft = DoNotLoad; }
    void SetMarkTileBoundaries(bool Enabled) { m_MarkTileBoundaries = Enabled; }

    void FlushLoadedTiles();

    UINT32 GetTilePoolTileCount() const { return m_TilePoolFreeTiles.GetBlockCount(); }
    void SetMinLODMapEditDelayCount(UINT32 Count) { m_MinLODMapEditDelayCount = Count; }
    UINT32* GetMinLODMapEditDelayCount() { return &m_MinLODMapEditDelayCount; }

private:

    bool IsLoggingEnabled() const { return m_LoggingEnabled; }
    void SetLoggingEnabled(bool Enabled) { m_LoggingEnabled = Enabled; }
    void LogString(const CHAR* strFormat, ...) const;

    static UINT64 HashFilename(const WCHAR* strFileName);
    void LogTileOperation(const TrackedTile* pTT, const CHAR* strOperation) const;

    TrackedTile* FindAndTrackTile(StreamingTexture* pST, UINT32 SliceIndex, UINT32 FractionalLOD, UINT32 MipTileX, UINT32 MipTileY, UINT32 MipTileZ, TrackedTile* pParentTile);
    void QueueSeenTile(TrackedTile* pTT, UINT32 MipIndex);

    UINT32 GetTileInFlightCount();
    void ClearAllPendingTiles();

#if SUPPORT_TIER_1
    void ProcessPendingDefaultMappings(ID3D12CommandQueue* pCmdQueue);
#endif
    void LoadPendingPackedMips();
    void AgeOutTiles(ID3D12CommandQueue* pCmdQueue);
    void UnmapTileNow(SingleTileUnmapRequestVector& UnmapVector, TrackedTile* pTT);
    void LoadSeenTiles();
    void ProcessReleasedSlots();
    void CheckAsyncLoadsForCompletion();
    void ProcessCompletedLoadTiles(ID3D12CommandQueue* pCmdQueue);
    void ProcessPendingTileMappings(ID3D12CommandQueue* pCmdQueue);
    void ProcessFeedbackMaps(ID3D12CommandQueue* pCmdQueue);
    void ProcessFeedbackMap(FeedbackTexture* pFT);
    void ProcessNativeMinLODMapEdits();
    void CombineMinLODMaps();
    bool ApplyMinLODMapUnmap(const MinLODMapEdit& RME, StreamingTexture* pST, const StagingTexture& StagingTexture, BYTE* pMinLODTexels);
    void PerformRandomLoads();
    void DiscardExpiredSeenTile(TrackedTile* pTT);

    void RenderTileDataCopies(ID3D12GraphicsCommandList* pCmdList);

    UINT64 GetAgeOutTime() const;

    bool ImplReadFileSynchronous(const WCHAR* strFileName, void* pBuffer, SIZE_T BufferSizeBytes);
    HANDLE ImplOpenTextureDataFile(const WCHAR* strFileName) const;
    void ImplCloseTextureDataFile(HANDLE hFile) const;
    void ImplCreateStandInTexture(StreamingTexture* pST);

    void ImageReadTextureFileHeader(StreamingTexture* pST);
    bool ImageLoadPackedMips(StreamingTexture* pST);
    void ImageLoadTile(TrackedTile* pTT, AsynchronousLoadOperation* pALO);
    void ImageDecompressTile(TrackedTile* pTT, BYTE* pDecompressedBuffer, UINT32 DecompressedStagingSlotIndex);
    void ImageUploadPackedMipTail(TrackedTile* pTT, ID3D12GraphicsCommandList* pCmdList, const BYTE* pDecompressedBuffer);

    void ImageMarkTileBoundaries(BYTE* pDecompressedTile, DXGI_FORMAT Format, UINT32 TileWidthElements, UINT32 TileHeightElements);

    StreamingTexture* FindExistingTexture(UINT64 Hash, const WCHAR* strFileName) const;
    StreamingTexture* CreateAndAddNewTexture(UINT64 Hash, const WCHAR* strFileName, const WCHAR* strDataFileName, HANDLE hDataFile, UINT64 DataFileOffset);
    void DestroyTexture(StreamingTexture* pST);

    void CreateNativeMinLODTexture(StreamingTexture* pST, UINT32 DefaultLOD);
#if XBOX_SAMPLER_FEEDBACK
    HRESULT FindOrCreateBatchedFeedbackResource(const D3D12_RESOURCE_DESC1* pOpaqueResourceDesc, ID3D12Resource** ppResource, UINT32* pFirstSliceIndex, UINT32* pFeedbackStagingOffsetElements);
    void SynchronizeFeedbackArray(ID3D12GraphicsCommandList* pCmdList, OpaqueTextureArray* pTexArray);
#endif

    TrackedTile* CreateTrackedTile();
    void FreeTrackedTile(TrackedTile* pTT);

    void QueueTrackedTileForLoading(TrackedTile* pTT, AsynchronousLoadOperation* pALO);

    bool SynchronizeNativeMinLODMap(ID3D12GraphicsCommandList* pCmdList, StreamingTexture* pST);
    void MapFeedbackStagingBuffer();
    void UnmapFeedbackStagingBuffer();

    USHORT RequestTilePoolSlot(bool Block);
    void ReleaseTilePoolSlot(USHORT Slot) { m_TilePoolFreeTiles.FreeBlock(Slot); }

    UINT32 RequestCompressedLoadSlot(bool Block);
    BYTE* GetCompressedLoadSlotBuffer(UINT32 Slot) const;
    void ReleaseCompressedLoadSlot(UINT32 Slot) { m_CompressedStagingAllocator.FreeBlock((USHORT)Slot); }

    UINT32 RequestDecompressedStagingSlot() { return (UINT32)m_DecompressedStagingAllocator.AllocateBlock(); }
    BYTE* GetDecompressedStagingSlotBuffer(UINT32 Slot) const;

    TilePoolAtlas* FindOrCreateTilePoolAtlas(const TiledResourceImage::Header* pHeader, ID3D12CommandQueue* pCmdQueue);

    UINT32 ComputeCompleteListIndex(const TrackedTile* pTT) const;

    // LoadCompleteSortPredicate sorts the Loaded tiles by priority, to move to the next stage (Populated).
    // Tiles that are both loaded and mapped are high priority.
    // Tiles that have a higher LOD index are higher priority (i.e. process coarse mips first).
    static bool LoadCompleteSortPredicate(const TrackedTile* pA, const TrackedTile* pB)
    {
        if (pA->Status == TrackedTileStatus_Loaded && pB->Status == TrackedTileStatus_Loading)
        {
            return true;
        }
        if (pB->Status == TrackedTileStatus_Loaded && pA->Status == TrackedTileStatus_Loading)
        {
            return false;
        }
        if (pA->IsMapped && !pB->IsMapped)
        {
            return true;
        }
        if (pB->IsMapped && !pA->IsMapped)
        {
            return false;
        }
        return pA->LastSeenFractionalLOD > pB->LastSeenFractionalLOD;
    }

    // SeenToLoadingSortPredicate sorts the Seen tiles by priority, to move to the next stage (Loading).
    // Tiles that have a higher LOD index are higher priority (i.e. process coarse mips first).
    // Tiles that have been seen more recently (higher last seen frame index) have a higher priority.
    static bool SeenToLoadingSortPredicate(const TrackedTile* pA, const TrackedTile* pB)
    {
        if (pA->LastSeenFractionalLOD != pB->LastSeenFractionalLOD)
        {
            return pA->LastSeenFractionalLOD > pB->LastSeenFractionalLOD;
        }
        return pA->LastSeenTicks > pB->LastSeenTicks;
    }

    // CompletedSortPredicate sorts the Populated tiles by priority, to be selected for age-out.
    // Tiles that are not pinned are high priority.
    // Tiles that have been seen less recently (lower last seen frame index) have a higher priority.
    static bool CompletedSortPredicate(const TrackedTile* pA, const TrackedTile* pB)
    {
        if (!pA->Pinned && pB->Pinned)
        {
            return true;
        }
        if (!pB->Pinned && pA->Pinned)
        {
            return false;
        }
        return pA->LastSeenTicks < pB->LastSeenTicks;
    }

    static bool MinLODMapEditPredicate(const MinLODMapEdit& A, const MinLODMapEdit& B)
    {
        if (A.pTexture != B.pTexture)
        {
            return A.pTexture < B.pTexture;
        }
        if (A.IsUnmap != B.IsUnmap)
        {
            return A.IsUnmap < B.IsUnmap;
        }
        if (A.IsUnmap)
        {
            return A.SubresourceIndex < B.SubresourceIndex;
        }
        return A.SubresourceIndex > B.SubresourceIndex;
    }

    static bool SingleTileUnmapRequestPredicate(const SingleTileUnmapRequest& A, const SingleTileUnmapRequest& B)
    {
        return A.pResource < B.pResource;
    }

    static UINT32 WorkerDecompressTile(StreamingTextureManager* pSTM, TrackedTile* pTT, void* pDecompressedBufferIndex)
    {
        assert(pTT->Status == TrackedTileStatus_Loaded /*&& pTT->IsMapped*/);
        UINT32 DecompressSlotIndex = (UINT32)pDecompressedBufferIndex;
        BYTE* pDecompressedBuffer = pSTM->GetDecompressedStagingSlotBuffer(DecompressSlotIndex);
        pSTM->ImageDecompressTile(pTT, pDecompressedBuffer, DecompressSlotIndex);
        return 0;
    }
};

extern StreamingTextureManager g_StreamingTextureManager;
