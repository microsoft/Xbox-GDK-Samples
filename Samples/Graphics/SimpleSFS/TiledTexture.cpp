//--------------------------------------------------------------------------------------
// TiledTexture.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "TiledTexture.h"

TiledTexture::TiledTexture() noexcept(false)
    : NumTiles(0)
    , TileShape{}
    , PackedMipInfo{}
{
    Tiles.resize(NumTiles);
    TilingInfo.resize(NumTiles);
}

bool TiledTexture::Create(_In_ ID3D12Device* pDevice, const std::wstring& xbtcFilePath)
{
    assert(pDevice);
    
    XBTCArchiveReader xbtcFile;

    if (!xbtcFile.OpenFromFile(xbtcFilePath.c_str()))
    {
        return false;
    }

    // For this simple sample, we're only working with a single texture in the xbtc file, so just look at the first texture in the file
    const uint32_t textureIndex = 0;
    auto textureInfo = xbtcFile.GetTextureInfo(textureIndex);

    // Check that this is a tiled texture
    if (textureInfo == nullptr ||
#if _GXDK_VER < 0x4A611B35 /* GDK Edition 210600 */
        textureInfo->TileMode != XBTC_TILING_SCARLETT_64KB_PRT)
#else
        textureInfo->Granularity == XBTC_STREAM_GRANULARITY_WHOLE_MIPS)
#endif
    {
        xbtcFile.Close();
        return false;
    }

    // Create tiled texture
    CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D((DXGI_FORMAT)textureInfo->Format, textureInfo->Width + 1, UINT(textureInfo->Height) + 1, 1,
                                                textureInfo->MipCount, 1, 0, D3D12_RESOURCE_FLAG_NONE, D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE);

    DX::ThrowIfFailed(pDevice->CreateReservedResource(&texDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)(D3dResource.ReleaseAndGetAddressOf())));
    D3dResource->SetName(L"TiledTexture");

    // Get the number of tiles and mips
#pragma warning(suppress : 6387) // 7th param is intentionally nullptr when not querying subresource tilings
    pDevice->GetResourceTiling(D3dResource.Get(), &NumTiles, &PackedMipInfo, &TileShape, nullptr, 0, nullptr);

    // Get tiling info
    UINT numSubresourceTilings = static_cast<UINT>(texDesc.MipLevels * texDesc.DepthOrArraySize);
    Tiles.resize(NumTiles);
    TilingInfo.resize(size_t(PackedMipInfo.NumStandardMips + PackedMipInfo.NumPackedMips));
    pDevice->GetResourceTiling(D3dResource.Get(), nullptr, nullptr, nullptr, &numSubresourceTilings, 0, &TilingInfo[0]);

    // GetResourceTiling returns unusable info for the mip tail, e.g. -1 for StartTileIndexInOverallResource. Overwrite it with useful info
    for (UINT mip = 0; mip < PackedMipInfo.NumPackedMips; mip++)
    {
        UINT index = mip + PackedMipInfo.NumStandardMips;
        TilingInfo[index].WidthInTiles = 1;
        TilingInfo[index].HeightInTiles = 1;
        TilingInfo[index].DepthInTiles = 1;
        TilingInfo[index].StartTileIndexInOverallResource = PackedMipInfo.StartTileIndexInOverallResource;
    }

    // Verify that the texture data on disk has the same number of tiles in memory
    if (NumTiles != UINT(xbtcFile.GetResourceStreamCount(textureIndex)))
    {
        xbtcFile.Close();
        return false;
    }

    // Get the base offset in the file for the first tile of the texture
    UINT64 baseOffset = UINT64(xbtcFile.GetResourceSeekOffset(textureIndex));

    // Setup the file offset and load size for each tile
    for (UINT mip = 0; mip < PackedMipInfo.NumStandardMips; mip++)
    {
        auto tileIndex = TilingInfo[mip].StartTileIndexInOverallResource;

        for (UINT y = 0; y < TilingInfo[mip].HeightInTiles; y++)
        {
            for (UINT x = 0; x < TilingInfo[mip].WidthInTiles; x++)
            {
                auto tileInfo = xbtcFile.GetStreamInfo(tileIndex);
                Tiles[tileIndex].LoadSize = tileInfo->CompressedSize;
                Tiles[tileIndex].InflatedSize = tileInfo->InflatedSize;
                Tiles[tileIndex].OffsetInFile = baseOffset + tileInfo->RelativeOffset;
                Tiles[tileIndex].Coord = { x, y, 0, mip };
                Tiles[tileIndex].ZlibCompressed = (tileInfo->CompressedSize != tileInfo->InflatedSize);
                Tiles[tileIndex].BCPacked = (tileInfo->UncompressedSize != tileInfo->InflatedSize);

                auto parentTileX = x >> 1;
                auto parentTileY = y >> 1;
                Tiles[tileIndex].pParentTile = GetTile(parentTileX, parentTileY, mip + 1);

                tileIndex++;

                // Tiles are required to be 64KB
                assert(tileInfo->UncompressedSize == D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES);
            }
        }
    }

    // Set the same info for the miptail tile
    auto mipTailTileIndex = PackedMipInfo.StartTileIndexInOverallResource;
    auto tileInfo = xbtcFile.GetStreamInfo(mipTailTileIndex);
    Tiles[mipTailTileIndex].LoadSize = tileInfo->CompressedSize;
    Tiles[mipTailTileIndex].InflatedSize = tileInfo->InflatedSize;
    Tiles[mipTailTileIndex].OffsetInFile = baseOffset + tileInfo->RelativeOffset;
    Tiles[mipTailTileIndex].Coord = { 0, 0, 0, PackedMipInfo.NumStandardMips };
    Tiles[mipTailTileIndex].ZlibCompressed = (tileInfo->CompressedSize != tileInfo->InflatedSize);
    Tiles[mipTailTileIndex].BCPacked = (tileInfo->UncompressedSize != tileInfo->InflatedSize);

    xbtcFile.Close();

    return true;
}

// Return the tile given the tile coordinates
Tile* TiledTexture::GetTile(UINT tileX, UINT tileY, UINT mipIndex) const
{
    UINT numMips = UINT(PackedMipInfo.NumStandardMips + PackedMipInfo.NumPackedMips);

    if (mipIndex >= numMips ||
        tileX >= TilingInfo[mipIndex].WidthInTiles ||
        tileY >= TilingInfo[mipIndex].HeightInTiles)
    {
        return nullptr;
    }

    auto tileIndex = TilingInfo[mipIndex].StartTileIndexInOverallResource + (tileY * TilingInfo[mipIndex].WidthInTiles) + tileX;
    assert(tileIndex < NumTiles);

    return GetTile(tileIndex);
}
