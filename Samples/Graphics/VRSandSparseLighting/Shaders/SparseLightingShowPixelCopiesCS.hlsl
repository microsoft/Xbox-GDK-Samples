//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "SparseLightingCommon.hlsli"




// although common resolutions (2160p, 1440p) do divide into exactly 16 (lighting tile size)
// better to use a minimal group size for DRS
[RootSignature(GlobalRS)]
[numthreads(8, 4, 1)]
void SparseLightingShowPixelCopies(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID)
{
    /*
        By default we were processing all sky tiles first, which created ~60us of VALU downtime while we spin
        clear tiles. Inverting Y to process sky tiles last had a similar result, just at the end of the dispatch.

        Instead dispatch.x/y are swapped, so we are processing the screen left to right instead of top
        to bottom. This puts sky tiles in the mix with work items which is a lot better

        We could build a seperate list of tiles to process and use an ExecuteIndirect, but this is
        unlikely to be quicker, and a gaurenteed net loss when there is no sky

        We could also use a 1D dispatch and use a divide to turn into a 2D tile coordinate, but integer divides
        are nasty, so this trick of swapping x/y around seems to work well enough
    */
    Gid.xy = Gid.yx;

    // there are 8 wave32s per 16x16 sparse lighting tile, calculate which 16x16 tile we're dealing with
    uint2 tileId                = uint2(Gid.x / 2, Gid.y / 4);
    uint tileIndex              = GetSparseLightingTileLinearIndexScalar(tileId);
    uint sparseLightingCount    = SparseLightingCountSRV[tileIndex];

    // in the case of a full rate tile, we didn't write any coordinates / it would be pointless reading them
    bool sparseLightingEnabled  = sparseLightingCount && (sparseLightingCount <= (256 - 32));

    if (!sparseLightingEnabled)      // scalar
        return;
    
    // this is scalar, as long as we avoid __XB_MadU24 and __XB_MulU24 which are vector instructions
    uint waveStartIndex         = ((Gid.x & 1) * 4 + (Gid.y & 3));                      // which wave of all the waves that are going to process this tile [0..7]
    uint lightCoordIndex        = waveStartIndex * 32 + WaveGetLaneIndex();

    if (lightCoordIndex >= sparseLightingCount)
        return;

    uint sparseCoord            = SparseLightingCoordsSRV[tileIndex * MAX_COORDINATES_PER_SCREEN_TILE + lightCoordIndex];
    uint2 coord;
    coord.x                     = tileId.x * 16 + SPARSE_DATA_GET_X(sparseCoord);
    coord.y                     = tileId.y * 16 + SPARSE_DATA_GET_Y(sparseCoord);

    // sqrt makes non holes values tend towards 1.0, which lightens any pixels which are not holes
    PostProcessUAV[uint2(coord.x, coord.y)] = sqrt(PostProcessUAV[uint2(coord.x, coord.y)]);

    uint2 copyCoord = coord.xy ^ 0x1;

    if (SPARSE_DATA_COPY_H(sparseCoord))
    {
        PostProcessUAV[uint2(copyCoord.x, coord.y)] = 0.0;
    }
    if (SPARSE_DATA_COPY_V(sparseCoord))
    {
        PostProcessUAV[uint2(coord.x, copyCoord.y)] = 0.0;
    }
    if (SPARSE_DATA_COPY_D(sparseCoord))
    {
        PostProcessUAV[uint2(copyCoord.x, copyCoord.y)] = 0.0;
    }
}
