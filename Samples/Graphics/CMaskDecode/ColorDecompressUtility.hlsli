//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author(s):  James Stanard
//

#if !defined(_XBOX_ONE) && !defined(_XBOX_SCARLETT)
#error "This shader utility is only usable on Xbox"
#endif

// Returns a 4-bit ClearMask value for an 8x8 tile.  The order of the bits for
// the 4x4 sub-tiles are:
//
//   0001 = Upper-Left
//   0010 = Upper-Right
//   0100 = Lower-Left
//   1000 = Lower-Right
//
// If a bit is set, that sub-tile is NOT clear.  This function assumes the tiling pattern
// used with non-MSAA render targets.
//

#if _XBOX_ONE

// Only P4_16x16 (Durango) and P8_32x32_16x16 (Scorpio) are currently supported.  Each is indicated
// by the pipe count (4 or 8) in the CMaskInfo header.
uint GetCMask(ByteAddressBuffer CMaskBuffer, uint2 TileCoord, uint CMaskInfo)
{
    // Unpack the CMaskInfo descriptor passed in by the application
    const uint2 NumTiles = uint2(CMaskInfo, CMaskInfo >> 12) & 0xFFF;
    const uint PipeCount = (CMaskInfo >> 24) & 0x7F;
    const bool LinearAddressing = (CMaskInfo >> 31) == 1;

    // Dimensions of the macro tile for non-linear mode.  (In units of tiles, not pixels.)
    uint macroTileWidth = 8 * PipeCount;
    uint macroTileHeight = 32;

    uint tileY0 = TileCoord.y & 1;
    uint tileX1 = (TileCoord.x >> 1) & 1;
    uint elemIdx = (tileX1 ^ tileY0) | tileX1 << 1U;
    uint elemIdxBits = 2;

    uint pipeMask = PipeCount - 1;
    uint pipe = (TileCoord.x ^ TileCoord.y ^ tileX1) & pipeMask;
    uint pipeBits = countbits(pipeMask);
    uint microRightShift = elemIdxBits + pipeBits - 4;

    // tilesPerPipe = macroTileWidth * macroTileHeight / PipeCount
    const uint tilesPerPipe = 256;

    const uint macroTileCountX = NumTiles.x / macroTileWidth; // clPitch
    const uint macroTileCountY = NumTiles.y / macroTileHeight;

    const uint slicePitch = NumTiles.x * NumTiles.y / PipeCount;

    // for 2D array and 3D textures (including cube maps)
    uint tileSlice = 0; // tileZ
    uint sliceOffset = slicePitch * tileSlice;

    // macro tile location
    uint macroX = TileCoord.x / macroTileWidth;
    uint macroY = TileCoord.y / macroTileHeight;
    uint macroOffset = LinearAddressing ? 0 : (macroX + macroTileCountX * macroY) * tilesPerPipe;

    // micro (4x4 tile) tiling
    uint microX = (LinearAddressing ? TileCoord.x : (TileCoord.x % macroTileWidth)) / 4;
    uint microY = (LinearAddressing ? TileCoord.y : (TileCoord.y % macroTileHeight)) / 4;
    uint microPitch = (LinearAddressing ? NumTiles.x : macroTileWidth) / 4;
    uint microOffset = ((microX + microY * microPitch) >> microRightShift) << elemIdxBits | elemIdx;

    uint tileIndex = sliceOffset + macroOffset + microOffset;

    // Squeeze the 2-bit pipe value into the address.
    uint nibbleAddress = (tileIndex & ~0x1FF) << pipeBits | pipe << 9 | (tileIndex & 0x1FF);

    // Convert the nibble address to the address of the word and the bit offset
    // into the word.
    uint wordAddress = (nibbleAddress / 8) * 4;	// 4 bytes per word
    uint bitOffset = (nibbleAddress % 8) * 4;	// 4 bits per nibble

    return (CMaskBuffer.Load(wordAddress) >> bitOffset) & 0xF;
}

#else // Scarlett

uint X3(uint2 xy, uint bitPos) { return ((xy.x >> 0) & 1) << bitPos; }
uint X4(uint2 xy, uint bitPos) { return ((xy.x >> 1) & 1) << bitPos; }
uint X5(uint2 xy, uint bitPos) { return ((xy.x >> 2) & 1) << bitPos; }
uint X6(uint2 xy, uint bitPos) { return ((xy.x >> 3) & 1) << bitPos; }
uint X7(uint2 xy, uint bitPos) { return ((xy.x >> 4) & 1) << bitPos; }
uint X8(uint2 xy, uint bitPos) { return ((xy.x >> 5) & 1) << bitPos; }
uint X9(uint2 xy, uint bitPos) { return ((xy.x >> 6) & 1) << bitPos; }

uint Y3(uint2 xy, uint bitPos) { return ((xy.y >> 0) & 1) << bitPos; }
uint Y4(uint2 xy, uint bitPos) { return ((xy.y >> 1) & 1) << bitPos; }
uint Y5(uint2 xy, uint bitPos) { return ((xy.y >> 2) & 1) << bitPos; }
uint Y6(uint2 xy, uint bitPos) { return ((xy.y >> 3) & 1) << bitPos; }
uint Y7(uint2 xy, uint bitPos) { return ((xy.y >> 4) & 1) << bitPos; }
uint Y8(uint2 xy, uint bitPos) { return ((xy.y >> 5) & 1) << bitPos; }
uint Y9(uint2 xy, uint bitPos) { return ((xy.y >> 6) & 1) << bitPos; }

uint Z1_X4_Y4(uint2 xy, uint bitPos)
{
    return 0 ^ X4(xy, bitPos) ^ Y4(xy, bitPos);
}

uint Z0_X5_Y5(uint2 xy, uint bitPos)
{
    return 0 ^ X5(xy, bitPos) ^ Y5(xy, bitPos);
}

uint Y4_X7_Y7(uint2 xy, uint bitPos)
{
    return Y4(xy, bitPos) ^ X7(xy, bitPos) ^ Y7(xy, bitPos);
}

uint Z0_Y5_X6(uint2 xy, uint bitPos)
{
    return 0 ^ Y5(xy, bitPos) ^ X6(xy, bitPos);
}

uint X5_Y6(uint2 xy, uint bitPos)
{
    return X5(xy, bitPos) ^ Y6(xy, bitPos);
}

uint X6_Y6(uint2 xy, uint bitPos)
{
    return X6(xy, bitPos) ^ Y6(xy, bitPos);
}

uint Y4_X6_Y6(uint2 xy, uint bitPos)
{
    return Y4(xy, bitPos) ^ X6(xy, bitPos) ^ Y6(xy, bitPos);
}

// Arden (Anaconda): 32 pipes, 8 packers, 4 bpe, 256B pipe interleave
uint SwizzleCMaskArden(uint2 tileXY, uint pitch)
{
    // Compute bit offset into block for 4-bit mask
    uint blkOffset =    X3      (tileXY, 2)
                   +    Y3      (tileXY, 3)
                   +    Y6      (tileXY, 4)
                   +    X7      (tileXY, 5)
                   +    Y7      (tileXY, 6)
                   +    X8      (tileXY, 7)
                   +    Y8      (tileXY, 8)
                   +    X9      (tileXY, 9)
                   +    Y9      (tileXY, 10)
                   +    Y4_X7_Y7(tileXY, 11)
                   +    Z1_X4_Y4(tileXY, 12)
                   +    Z0_Y5_X6(tileXY, 13)
                   +    X5_Y6   (tileXY, 14)
                   +    X6_Y6   (tileXY, 15);

    const uint metaBlkWidth = 128;
    const uint metaBlkHeight = 128;
    const uint xb = tileXY.x / metaBlkWidth;
    const uint yb = tileXY.y / metaBlkHeight;
    const uint pb = pitch    / metaBlkWidth;
    const uint blkIndex = (yb * pb) + xb;
    const uint blkSize = 8192;

    // NOP:  pipeXor = 0
    const uint blkMask = blkSize - 1;
    const uint numPipes = 32;
    const uint pipeMask = numPipes - 1;
    const uint pipeXorIn = 0; 
    const uint pipeInterleaveLog2 = 8;
    const uint pipeXor = ((pipeXorIn & pipeMask) << pipeInterleaveLog2) & blkMask;

    // Return bit address
    return (blkIndex * blkSize * 8) + (blkOffset ^ (pipeXor * 8));
}

// Sparkman (Lockhart): 8 pipes, 4 packers, 4 bpe, 256B pipe interleave
uint SwizzleCMaskSparkman(uint2 tileXY, uint pitch)
{
    // Compute bit offset into block for 4-bit mask
    uint blkOffset =    X3      (tileXY, 2)
                   +    Y3      (tileXY, 3)
                   +    X5      (tileXY, 4)
                   +    X6      (tileXY, 5)
                   +    Y6      (tileXY, 6)
                   +    X7      (tileXY, 7)
                   +    Y7      (tileXY, 8)
                   +    X8      (tileXY, 9)
                   +    Y8      (tileXY, 10)
                   +    Y4_X6_Y6(tileXY, 11)
                   +    Z1_X4_Y4(tileXY, 12)
                   +    Z0_X5_Y5(tileXY, 13)
                   +    X9      (tileXY, 14);

    const uint metaBlkWidth = 128;
    const uint metaBlkHeight = 64;
    const uint xb = tileXY.x / metaBlkWidth;
    const uint yb = tileXY.y / metaBlkHeight;
    const uint pb = pitch    / metaBlkWidth;
    const uint blkIndex = (yb * pb) + xb;
    const uint blkSize = 4096;

    // NOP:  pipeXor = 0
    const uint blkMask = blkSize - 1;
    const uint numPipes = 8;
    const uint pipeMask = numPipes - 1;
    const uint pipeXorIn = 0; 
    const uint pipeInterleaveLog2 = 8;
    const uint pipeXor = ((pipeXorIn & pipeMask) << pipeInterleaveLog2) & blkMask;

    // Return bit address
    return (blkIndex * blkSize * 8) + (blkOffset ^ (pipeXor * 8));
}

uint GetCMask(ByteAddressBuffer CMaskBuffer, uint2 TileCoord, uint CMaskInfo)
{
    const uint PitchInTiles = CMaskInfo & 0xFFF;
    const uint PipeCount = (CMaskInfo >> 24) & 0x7F;

    uint bitAddress;

    if (PipeCount == 32)
    {
        bitAddress = SwizzleCMaskArden(TileCoord, PitchInTiles);
    }
    else
    {
        bitAddress = SwizzleCMaskSparkman(TileCoord, PitchInTiles);
    }

    // Convert the bit address to the address of the word and the bit offset
    // into the word.
    uint dwordAddress = bitAddress / 32;
    uint bitOffset = bitAddress % 32;

    return __XB_UBFE(4, bitOffset, CMaskBuffer.Load(dwordAddress * 4));
}

#endif
