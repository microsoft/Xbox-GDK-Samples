//--------------------------------------------------------------------------------------
// TiledResourceImage.h
//
// Structure definitions for a simple file format used to store tiled images on disk.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#if TILED_RESOURCE_FILE_HEADER_DEBUG_CHECKS_ENABLED
#define TRIAssert(x) assert(x)
#else
#define TRIAssert(x) __noop
#endif

namespace TiledResourceImage
{
    static const UINT32 TILE_SHIFT = 16;
    static const UINT32 TILE_SIZE_BYTES = (1U << TILE_SHIFT);

    static const UINT32 CURRENT_VERSION = 'TRI1';

    enum MipLevelFlags
    {
        PackedMipLevel = 0x01,
    };

    enum FileHeaderFlags
    {
        HardwareNativeSwizzled = 0x01,
    };

    struct MipLevel_v1
    {
        UINT32 FileOffsetTiles;
        UINT8 WidthTilesM1;
        UINT8 HeightTilesM1;
        UINT8 DepthTilesM1;
        UINT8 Flags;
    };
    typedef MipLevel_v1 MipLevel;

    struct PackedMipTail_v1
    {
        UINT32 FirstMipIndex : 4;
        UINT32 FirstRowPitchBytes : 12;
        UINT32 FirstHeightRows : 8;
        UINT32 ElementByteShift : 3;
        UINT32 ElementSizeShift : 2;
    };

    struct Header_v1
    {
        UINT32 MagicVersion;

        UINT32 DXGIFormat : 8;
        UINT32 MipLevelCount : 4;
        UINT32 SliceCount : 13;
        UINT32 Flags : 7;

        MipLevel_v1 MipLevels[9];

        PackedMipTail_v1 MipTail;

        bool HasPackedMipTail() const { return MipTail.FirstMipIndex > 0; }
    };
    typedef Header_v1 Header;

    inline UINT64 GetMipTailOffsetAndSize(const Header* pHeader, UINT32* pFirstMipIndex, UINT32* pSizeTiles)
    {
        if (!pHeader->HasPackedMipTail())
        {
            return 0;
        }
        TRIAssert(pHeader->MipTail.FirstMipIndex < ARRAYSIZE(pHeader->MipLevels));
        const MipLevel& Mip = pHeader->MipLevels[pHeader->MipTail.FirstMipIndex];
        const UINT64 OffsetBytes = sizeof(*pHeader) + (UINT64)TILE_SIZE_BYTES * Mip.FileOffsetTiles;
        *pFirstMipIndex = pHeader->MipTail.FirstMipIndex;
        *pSizeTiles = Mip.WidthTilesM1 + 1;
        return OffsetBytes;
    }

    inline UINT32 GetMipTailSizeBytes(const Header* pHeader)
    {
        if (!pHeader->HasPackedMipTail())
        {
            return 0;
        }

        UINT32 SizeBytes = 0;
        UINT32 RowPitchBytes = pHeader->MipTail.FirstRowPitchBytes;
        UINT32 HeightRows = pHeader->MipTail.FirstHeightRows;
        const UINT32 ElementSizeBytes = 1U << pHeader->MipTail.ElementByteShift;
        for (UINT MipIndex = pHeader->MipTail.FirstMipIndex; MipIndex < pHeader->MipLevelCount; ++MipIndex)
        {
            SizeBytes += (RowPitchBytes * HeightRows) * pHeader->SliceCount;
            RowPitchBytes = std::max(ElementSizeBytes, RowPitchBytes >> 1);
            HeightRows = std::max(1U, HeightRows >> 1);
        }

        return SizeBytes;
    }

    inline UINT64 ComputeTileOffset(const Header* pHeader, UINT32 MipIndex, UINT32 Slice, UINT32 TileX, UINT32 TileY, UINT32 TileZ, bool* pIsPackedMip)
    {
        TRIAssert(pHeader != nullptr);
        TRIAssert(MipIndex < (UINT32)pHeader->MipLevelCount && Slice < (UINT32)pHeader->SliceCount);
        TRIAssert(Slice == 0 || TileZ == 0);

        bool IsPackedMipLevel = false;
        if (pHeader->HasPackedMipTail() && MipIndex >= pHeader->MipTail.FirstMipIndex)
        {
            IsPackedMipLevel = true;
            MipIndex = pHeader->MipTail.FirstMipIndex;
        }

        if (MipIndex >= ARRAYSIZE(pHeader->MipLevels))
        {
            return 0;
        }

        const MipLevel& Mip = pHeader->MipLevels[MipIndex];
        if ((Mip.Flags & PackedMipLevel) != 0)
        {
            IsPackedMipLevel = true;
        }

        UINT64 OffsetTiles = (UINT64)Mip.FileOffsetTiles;

        if (!IsPackedMipLevel)
        {
            const UINT64 FlatSliceTiles = (Mip.WidthTilesM1 + 1) * (Mip.HeightTilesM1 + 1);
            if (TileZ > 0)
            {
                TRIAssert(TileZ < (Mip.DepthTilesM1 + 1));
                OffsetTiles += FlatSliceTiles * TileZ;
            }
            else
            {
                TRIAssert(Slice < pHeader->SliceCount);
                OffsetTiles += FlatSliceTiles * Slice;
            }
            TRIAssert(TileY < (Mip.HeightTilesM1 + 1));
            OffsetTiles += (TileY * (Mip.WidthTilesM1 + 1));
            TRIAssert(TileX < (Mip.WidthTilesM1 + 1));
            OffsetTiles += TileX;
        }

        if (pIsPackedMip != nullptr)
        {
            *pIsPackedMip = IsPackedMipLevel;
        }

        return OffsetTiles;
    }

    inline UINT64 ComputeFileOffsetBytes(const Header* pHeader, UINT32 MipIndex, UINT32 Slice, UINT32 TileX, UINT32 TileY, UINT32 TileZ, bool* pIsPackedMip)
    {
        UINT64 OffsetBytes = ComputeTileOffset(pHeader, MipIndex, Slice, TileX, TileY, TileZ, pIsPackedMip) << TILE_SHIFT;
        return OffsetBytes;
    }

    inline bool GetTileShape(const Header* pHeader, UINT32* pWidthElements, UINT32* pHeightElements, UINT32* pWidthTexels, UINT32* pHeightTexels)
    {
        UINT32 TileWidthElements = 0;
        UINT32 TileHeightElements = 0;
        switch (pHeader->MipTail.ElementByteShift)
        {
        case 0:
            TileWidthElements = 256;
            TileHeightElements = 256;
            break;
        case 1:
            TileWidthElements = 256;
            TileHeightElements = 128;
            break;
        case 2:
            TileWidthElements = 128;
            TileHeightElements = 128;
            break;
        case 3:
            TileWidthElements = 128;
            TileHeightElements = 64;
            break;
        case 4:
            TileWidthElements = 64;
            TileHeightElements = 64;
            break;
        default:
            return false;
        }

        // TODO: adjust width and height for sample count, when supported

        if (pWidthElements != nullptr)
        {
            *pWidthElements = TileWidthElements;
        }
        if (pHeightElements != nullptr)
        {
            *pHeightElements = TileHeightElements;
        }
        if (pWidthTexels != nullptr)
        {
            *pWidthTexels = TileWidthElements << pHeader->MipTail.ElementSizeShift;
        }
        if (pHeightTexels != nullptr)
        {
            *pHeightTexels = TileHeightElements << pHeader->MipTail.ElementSizeShift;
        }

        return true;
    }

    inline bool GetMipDimensions(const Header* pHeader, UINT32 MipIndex, UINT32* pWidthTexels, UINT32* pHeightTexels)
    {
        if (MipIndex >= pHeader->MipLevelCount)
        {
            return false;
        }
        UINT32 TileWidthTexels = 0;
        UINT32 TileHeightTexels = 0;
        GetTileShape(pHeader, nullptr, nullptr, &TileWidthTexels, &TileHeightTexels);
        UINT32 BaseMipWidth = (pHeader->MipLevels[0].WidthTilesM1 + 1) * TileWidthTexels;
        UINT32 BaseMipHeight = (pHeader->MipLevels[0].HeightTilesM1 + 1) * TileHeightTexels;
        const UINT32 MinMipSize = 1U << pHeader->MipTail.ElementSizeShift;
        if (pWidthTexels != nullptr)
        {
            *pWidthTexels = std::max(MinMipSize, BaseMipWidth >> MipIndex);
        }
        if (pHeightTexels != nullptr)
        {
            *pHeightTexels = std::max(MinMipSize, BaseMipHeight >> MipIndex);
        }
        return true;
    }

    inline UINT32 GetTotalTileCount(const Header* pHeader)
    {
        UINT32 TileCount = 0;

        UINT32 EndMipIndex = pHeader->MipLevelCount;
        if (pHeader->HasPackedMipTail())
        {
            EndMipIndex = pHeader->MipTail.FirstMipIndex;
            
            // TODO: properly compute tile count of mip tail
            ++TileCount;
        }

        for (UINT32 i = 0; i < EndMipIndex; ++i)
        {
            const MipLevel& Mip = pHeader->MipLevels[i];
            UINT32 SliceTileCount = (Mip.WidthTilesM1 + 1) * (Mip.HeightTilesM1 + 1) * (Mip.DepthTilesM1 + 1);
            TileCount += SliceTileCount * pHeader->SliceCount;
        }

        return TileCount;
    }

    inline void GetTotalTileCountPerMip(const Header* pHeader, UINT32* pPerMipCounts, UINT32 PerMipArraySize, bool RemoveCount)
    {
        UINT32 EndMipIndex = pHeader->MipLevelCount;
        if (pHeader->HasPackedMipTail())
        {
            EndMipIndex = pHeader->MipTail.FirstMipIndex;

            // TODO: properly compute tile count of mip tail
            UINT32 OutputIndex = std::min(EndMipIndex, PerMipArraySize - 1);
            if (RemoveCount)
            {
                pPerMipCounts[OutputIndex] -= 1;
            }
            else
            {
                pPerMipCounts[OutputIndex] += 1;
            }
        }

        for (UINT32 i = 0; i < EndMipIndex; ++i)
        {
            const MipLevel& Mip = pHeader->MipLevels[i];
            UINT32 SliceTileCount = (Mip.WidthTilesM1 + 1) * (Mip.HeightTilesM1 + 1) * (Mip.DepthTilesM1 + 1);
            UINT32 MipTileCount = SliceTileCount * pHeader->SliceCount;

            UINT32 OutputIndex = std::min(i, PerMipArraySize - 1);
            if (RemoveCount)
            {
                pPerMipCounts[OutputIndex] -= MipTileCount;
            }
            else
            {
                pPerMipCounts[OutputIndex] += MipTileCount;
            }
        }
    }
};
