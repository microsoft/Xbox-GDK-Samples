//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

#define NUM_THREADS (64)

#define RootSig \
    "DescriptorTable(SRV(t0, numDescriptors=1)),"   \
    "SRV(t1),"                                      \
    "DescriptorTable(UAV(u0, numDescriptors=1)),"   \
    "RootConstants(b0, num32bitconstants=6)"

Texture2D<uint>     BinIds : register(t0);
ByteAddressBuffer   Coords : register(t1);
RWTexture2D<float3> Output : register(u0);

cbuffer ShaderParams : register (b0)
{
    uint numBins;
    uint numPixels;
    uint texSizeXInPixels;
    uint texSizeYInPixels;
    uint log2MacroTileSizeX;
    uint log2MacroTileSizeY;
}

[RootSignature(RootSig)]
[numthreads(NUM_THREADS, 1, 1)]
void main(uint ThreadId : SV_DispatchThreadId)
{
    if (ThreadId >= numPixels)
        return;

    uint Index = ThreadId;
    const uint2 PixelCoords = UnpackU16FromU32(Coords.Load(Index << 2));
    const uint BinId = BinIds[PixelCoords];

    uint3 Accum = 0;
    uint NumBits = firstbitlow(numBins);
    uint NumTriples = 0;
    for (uint i = 0; i < NumBits; i += 3, ++NumTriples)
    {
        Accum.x += (BinId >> (i + 0)) & 0x1;
        Accum.y += (BinId >> (i + 1)) & 0x1;
        Accum.z += (BinId >> (i + 2)) & 0x1;
    }

    uint2 OutputCoord;
    {
        const uint NumPixelsPerRowOfTiles = texSizeXInPixels << log2MacroTileSizeY;

        const uint TileY = (Index / NumPixelsPerRowOfTiles);

        // Make index relative to row of tiles
        Index %= NumPixelsPerRowOfTiles;

        // all tiles up to the current tile can cover more pixels than the actual resolution
        const uint NumPixelsCoveredByTilesY = (TileY + 1) << log2MacroTileSizeY;
        const uint ClapmedTileSizeY = min(NumPixelsCoveredByTilesY, texSizeYInPixels) - (TileY << log2MacroTileSizeY);

        // recompute the size of the tile in pixels even it's the bottom tile
        const uint NumPixelsPerTileInCurrentRow = ClapmedTileSizeY << log2MacroTileSizeX;

        const uint TileX = Index / NumPixelsPerTileInCurrentRow;

        // Make index tile-relative
        Index %= NumPixelsPerTileInCurrentRow;

        // all tiles up to the current tile can cover more pixels than the actual resolution
        const uint NumPixelsCoveredByTilesX = (TileX + 1) << log2MacroTileSizeX;
        const uint ClampedTileSizeX = min(NumPixelsCoveredByTilesX, texSizeXInPixels) - (TileX << log2MacroTileSizeX);

        const uint TileRelativeCoordY = Index / ClampedTileSizeX;
        const uint TileRelativeCoordX = Index % ClampedTileSizeX;

        OutputCoord.x = (TileX << log2MacroTileSizeX) + TileRelativeCoordX;
        OutputCoord.y = (TileY << log2MacroTileSizeY) + TileRelativeCoordY;
    }

    Output[OutputCoord] = float3(Accum) * 0.5f / float(NumTriples);
}
