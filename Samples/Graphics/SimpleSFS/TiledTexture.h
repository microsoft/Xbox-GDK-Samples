//--------------------------------------------------------------------------------------
// TiledTexture.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// Keep track of a tile on disk and in memory
struct Tile
{
    Tile() noexcept(false)
        : OffsetInFile(0)
        , LoadSize(0)
        , InflatedSize(0)
        , ZlibCompressed(false)
        , BCPacked(false)
        , IndexInMemoryPool(-1)
        , TimeLastSeen(0)
        , pParentTile(nullptr)
        , Coord{}
        , Status(TileStatus::Invalid) {}

    UINT64 OffsetInFile;                    // Location in file to stream from
    UINT32 LoadSize;                        // Size on disk
    UINT32 InflatedSize;                    // Size after inflate
    bool ZlibCompressed;                    // Using Zlib?
    bool BCPacked;                          // Using BCPack?
    int IndexInMemoryPool;                  // Location in memory pool where tile was streamed
    uint64_t TimeLastSeen;                  // Simple time stamp for aging out tiles
    Tile* pParentTile;                      // The next lower detail tile, e.g. if this is mip 0, the parent is mip 1
    D3D12_TILED_RESOURCE_COORDINATE Coord;  // Tile coordinates in the tiled texture

    enum class TileStatus
    {
        Invalid = 0,
        Requested,
        Loading,
        Loaded
    };

    TileStatus Status;

    // Keep track of when a tile was last seen for aging out the tile
    void SetTimeLastSeen(uint64_t timeLastSeen)
    {
        TimeLastSeen = timeLastSeen;

        // Mip levels should be continous, so if a tile is seen, it's parent tile is also seen
        if (pParentTile)
        {
            pParentTile->SetTimeLastSeen(timeLastSeen);
        }
    }
};

// Create and manage a tiled texture
struct TiledTexture
{
    TiledTexture() noexcept(false);

    // Resource info
    Microsoft::WRL::ComPtr<ID3D12Resource>  D3dResource;

    // Tiles info
    UINT                                    NumTiles;
    std::vector<Tile>                       Tiles;
    std::vector<D3D12_SUBRESOURCE_TILING>   TilingInfo;
    D3D12_TILE_SHAPE                        TileShape;
    D3D12_PACKED_MIP_INFO                   PackedMipInfo;

    bool Create(_In_ ID3D12Device* pDevice, const std::wstring& xbtcFilePath);

    Tile* GetTile(UINT tileX, UINT tileY, UINT mipIndex) const;
    Tile* GetTile(UINT tileIndex) const { assert(tileIndex < NumTiles); return (Tile*)&Tiles[tileIndex]; }
};
