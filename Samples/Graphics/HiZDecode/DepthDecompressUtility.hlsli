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
//             Ivan Nevraev
//             Alex Nankervis
//
// Version:    201102 (November 2, 2020)
//

#ifndef __DEPTH_DECOMPRESS_UTILITY__
#define __DEPTH_DECOMPRESS_UTILITY__

// With the approximate version, plane slopes are reduced from 28 bits to 24.  This is
// generally acceptable unless you need to match the hardware decompress or are very picky
// about precision.  When decompressing D16, approximation is nearly exact.
#ifndef DEPTH_UTIL_APPROXIMATE
#  define DEPTH_UTIL_APPROXIMATE 1
#endif

// It is not clear at the time of writing that the hardware can emit subnormal floats.
// The first define controls whether to check for them.  
#ifndef DEPTH_UTIL_HANDLE_DENORMS
#  define DEPTH_UTIL_HANDLE_DENORMS 0
#endif

// The second controls whether to flush them to zero or underflow.
#ifndef DEPTH_UTIL_FLUSH_DENORMS
#  define DEPTH_UTIL_FLUSH_DENORMS 1
#endif

// A D16 buffer is UNORM16, but interpolated plane values are still full precision.  Enable this
// to quantize the result to UNORM16 on return.  If writing to an R16_UNORM buffer, the conversion
// will happen automatically.  In fact, if writing to an R16_UNORM buffer, don't enable this.
#ifndef DEPTH_UTIL_EMULATE_D16_PRECISION
#  define DEPTH_UTIL_EMULATE_D16_PRECISION 0
#endif

// It is possible to have tiles be with Depth both clear and decompressed. If this option is enabled
// and the zMask is 13, then we don't have to decompress, and we can return the clear value.
#ifndef DEPTH_UTIL_DEPTH_EXPANDED_CLEAR_MODE
#  define DEPTH_UTIL_DEPTH_EXPANDED_CLEAR_MODE 0
#endif

// It is possible to have tiles be with Stencil both clear and decompressed. If this option is enabled
// and the SMem is 2, then we don't have to decompress, and we can return the clear value.
#ifndef DEPTH_UTIL_STENCIL_EXPANDED_CLEAR_MODE
#  define DEPTH_UTIL_STENCIL_EXPANDED_CLEAR_MODE 0
#endif

//==================================================================================================
// Function Prototypes
//==================================================================================================

float DecompressDepth(
    Texture2D<float> depthSrc,	// depth plane SRV (automatic decompression disabled)
    uint zMask,					// 4-bit Z Mask from HTile; GetZMask()
    uint2 st,					// PixelCoord = TileCoord * 8 + SwizzleLinearIndex(threadIndex)
    uint threadIndex,			// SV_GroupIndex
    float depthClear = 0.0,		// Clear value (0 implies reversed Z ordering ideal for F32)
    bool D16 = false			// True for 16-bit depth buffers
);

float DecompressDepthMSAA(Texture2DMS<float> depthSrc, uint numSamples, uint sampleIdx,
    uint zMask, uint2 st, uint threadIndex, float depthClear, bool D16, bool firstTime, int2 samplePos);

uint DecompressStencil( Texture2D<uint> stencilSrc,
    uint stencilState, uint2 st, uint threadIndex, uint stencilClear = 0);

uint DecompressStencilMSAA( Texture2DMS<uint> stencilSrc, uint numSamples, uint sampleIdx,
    uint stencilState, uint2 st, uint threadIndex, uint stencilClear = 0);

//==================================================================================================
// Utility Functions
//==================================================================================================

// Never more than 112 words are read from a compressed tile (but sometimes offset by 8 words)
groupshared uint sharedDepthData[128];

#if _XBOX_ONE

// Only P4_16x16 (Durango) and P8_32x32_16x16 (Scorpio) are currently supported.  Each is indicated
// by the pipe count (4 or 8) in the HTileInfo header.
uint GetHTileAddress(uint2 TileCoord, uint HTileInfo)
{
    // Unpack the HTileInfo descriptor passed in by the application
    const uint2 NumTiles = uint2(HTileInfo, HTileInfo >> 12) & 0xFFF;
    const uint PipeCount = (HTileInfo >> 24) & 0x7F;
    const bool LinearAddressing = (HTileInfo >> 31) == 1;

    // Dimensions of the macro tile for non-linear mode.  (In units of tiles, not pixels.)
    uint macroTileWidth = 64;
    uint macroTileHeight = 8 * PipeCount;

    uint tileY0 = TileCoord.y & 1;
    uint tileX1 = (TileCoord.x >> 1U) & 1;
    uint elemIdx = (tileX1 ^ tileY0) | tileX1 << 1U;
    uint elemIdxBits = 2;

    uint pipe = (TileCoord.x ^ TileCoord.y ^ tileX1) & (PipeCount - 1);
    uint pipeBits = firstbitlow(PipeCount);
    uint microRightShift = elemIdxBits + pipeBits - 4;

    // tilesPerPipe = macroTileWidth * macroTileHeight / PipeCount
    const uint tilesPerPipe = 512;

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

    // Each element accessed by a tile index is four bytes.  So address offset is 4 * tileIndex.
    uint tileByteOffset = tileIndex << 2;

    // The pipe value gets inserted into the tileByteOffset at bit 8
    return (tileByteOffset & ~0xff) << pipeBits | (pipe << 8) | (tileByteOffset & 0xff);
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

uint Y4_X7_Y7(uint2 xy, uint bitPos)
{
    return Y4(xy, bitPos) ^ X7(xy, bitPos) ^ Y7(xy, bitPos);
}

uint Z1_X4_Y4(uint2 xy, uint bitPos)
{
    return X4(xy, bitPos) ^ Y4(xy, bitPos) ^ 0;
}

uint Z0_X6_Y5(uint2 xy, uint bitPos)
{
    return Y5(xy, bitPos) ^ X6(xy, bitPos) ^ 0;
}

uint Y4_X6_Y6(uint2 xy, uint bitPos)
{
    return Y4(xy, bitPos) ^ X6(xy, bitPos) ^ Y6(xy, bitPos);
}

uint Z0_X5_Y5(uint2 xy, uint bitPos)
{
    return X5(xy, bitPos) ^ Y5(xy, bitPos) ^ 0;
}

uint X5_Y6(uint2 xy, uint bitPos)
{
    return X5(xy, bitPos) ^ Y6(xy, bitPos);
}

uint X6_Y6(uint2 xy, uint bitPos)
{
    return X6(xy, bitPos) ^ Y6(xy, bitPos);
}

uint SwizzleHTileAnaconda(uint2 tileXY, uint tilePitch)
{
    // 15     14      13         12          11              10               9                 8            7      6      5      4      3      2         1        0
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------
    // y9     x9      y8      x6 ^ y6      x5 ^ y6      z0 ^ y5 ^ x6     z1 ^ x4 ^ y4      y4 ^ x7 ^ y7     x8     y7     x7     y6     y3     x3         0        0
    //
    // 64KB blocks, block is square 1024 pixels in DB space, or 128 square in htile space
    //

    uint byteOffset =  X3      (tileXY,  2)
                    +  Y3      (tileXY,  3)
                    +  Y6      (tileXY,  4)
                    +  X7      (tileXY,  5)
                    +  Y7      (tileXY,  6)
                    +  X8      (tileXY,  7)
                    +  Y4_X7_Y7(tileXY,  8)
                    +  Z1_X4_Y4(tileXY,  9)
                    +  Z0_X6_Y5(tileXY, 10)
                    +  X5_Y6   (tileXY, 11)
                    +  X6_Y6   (tileXY, 12)
                    +  Y8      (tileXY, 13)
                    +  X9      (tileXY, 14)
                    +  Y9      (tileXY, 15);

    uint blockWidthInHtilePixels = 128;
    uint blockHeightInHtilePixels = 128;
    uint blockSizeBytes = blockWidthInHtilePixels * blockHeightInHtilePixels * 4;
    uint blockIndexX = tileXY.x / blockWidthInHtilePixels;
    uint blockIndexY = tileXY.y / blockHeightInHtilePixels;
    uint blockPitch = tilePitch / blockWidthInHtilePixels;
    uint blockIndex = blockIndexX + blockIndexY * blockPitch;

    return blockIndex * blockSizeBytes + byteOffset;
}


uint SwizzleHTileLockhart(uint2 tileXY, uint tilePitch)
{
    // 15     14      13         12          11              10               9                 8            7      6      5      4      3      2      1      0
    //---------------------------------------------------------------------------------------------------------------------------------------------------------------
    //                y8        x8           y7     z0 ^ x5 ^ y5          z1 ^ x4 ^ y4       y4 ^ x6 ^ y6    x7     y6     x6     x5     y3     x3     0      0
    //
    // 16KB blocks, block is square 512 pixels in DB space, or 64 square in htile space
    //

    uint byteOffset =  X3      (tileXY,  2)
                    +  Y3      (tileXY,  3)
                    +  X5      (tileXY,  4)
                    +  X6      (tileXY,  5)
                    +  Y6      (tileXY,  6)
                    +  X7      (tileXY,  7)
                    +  Y4_X6_Y6(tileXY,  8)
                    +  Z1_X4_Y4(tileXY,  9)
                    +  Z0_X5_Y5(tileXY, 10)
                    +  Y7      (tileXY, 11)
                    +  X8      (tileXY, 12)
                    +  Y8      (tileXY, 13);

    uint blockWidthInHtilePixels = 64;
    uint blockHeightInHtilePixels = 64;
    uint blockSizeBytes = blockWidthInHtilePixels * blockHeightInHtilePixels * 4;
    uint blockIndexX = tileXY.x / blockWidthInHtilePixels;
    uint blockIndexY = tileXY.y / blockHeightInHtilePixels;
    uint blockPitch = tilePitch / blockWidthInHtilePixels;
    uint blockIndex = blockIndexX + blockIndexY * blockPitch;

    return blockIndex * blockSizeBytes + byteOffset;
}

uint GetHTileAddress(uint2 TileCoord, uint HTileInfo)
{
    const uint PitchInTiles = HTileInfo & 0xFFF;
    const uint PipeCount = (HTileInfo >> 24) & 0x7F;

    if (PipeCount == 32)
    {
        return SwizzleHTileAnaconda(TileCoord, PitchInTiles);
    }
    else
    {
        return SwizzleHTileLockhart(TileCoord, PitchInTiles);
    }
}

#endif

uint2 GetZMaskAndStencilState(ByteAddressBuffer HTileBuffer, uint HTileInfo, uint2 TileCoord)
{
    uint HTileValue = HTileBuffer.Load(GetHTileAddress(TileCoord, HTileInfo));
    uint ZMask = __XB_UBFE(4, 0, HTileValue);
    uint StencilState = __XB_UBFE(2, 8, HTileValue);
    return uint2(ZMask, StencilState);
}

uint GetZMask(ByteAddressBuffer HTileBuffer, uint HTileInfo, uint2 TileCoord)
{
    return GetZMaskAndStencilState(HTileBuffer, HTileInfo, TileCoord).x;
}

uint GetStencilState(ByteAddressBuffer HTileBuffer, uint HTileInfo, uint2 TileCoord)
{
    return GetZMaskAndStencilState(HTileBuffer, HTileInfo, TileCoord).y;
}

// The flattened index of a pixel within a tile
uint LinearIndexFromPixelCoord(uint2 coord)
{
    // Optimized bit interleaving (to avoid a LUT)
    return  __XB_MadU24(0x408100, (uint)__XB_MulU24(coord.x, 0x101010) & 0x402010, 
        (uint)__XB_MulU24(0x810200, __XB_MulU24(coord.y, 0x101010) & 0x402010) ) >> 26;

    //return
    //	(coord.x & 1) << 0 | (coord.x & 2) << 1 | (coord.x & 4) << 2 |
    //	(coord.y & 1) << 1 | (coord.y & 2) << 2 | (coord.y & 4) << 3;
}

uint Pad(uint val, uint pow2)
{
    return (val + pow2 - 1) & ~(pow2 - 1);
}

uint CeilLog2(uint val)
{
    return val < 2 ? 0 : firstbithigh(val - 1) + 1;
}

uint ZLayoutForMask( uint zMask, uint sampleCount )
{
    uint NumPlanes = zMask;
    if (NumPlanes >= 11)
        NumPlanes += 2;
    else if (NumPlanes >= 9)
        NumPlanes += 1;

    uint ZPlaneChunkSize = NumPlanes * 12;
    uint PaddedZPlaneChunkSize = Pad(ZPlaneChunkSize, 32);

    uint PMaskSizeBits = CeilLog2(NumPlanes);
    if (sampleCount > 1 && NumPlanes > 1)
        PMaskSizeBits = max(PMaskSizeBits, 2);

    uint PMaskChunkSize = 64 * sampleCount * PMaskSizeBits / 8;
    uint PaddedPMaskChunkSize = Pad(PMaskChunkSize, 32);
    uint PaddedDepthChunkSize = PaddedZPlaneChunkSize + PaddedPMaskChunkSize;
    uint PackedDepthChunkSize = Pad(ZPlaneChunkSize + PMaskChunkSize, 32);
    uint PMaskOffset = PackedDepthChunkSize < PaddedDepthChunkSize ? ZPlaneChunkSize : PaddedZPlaneChunkSize;

    PMaskOffset = PackedDepthChunkSize > 256 ? Pad(PMaskOffset, 256) : Pad(PMaskOffset, 8);

    uint planeDataWords = (PMaskOffset + PMaskChunkSize) / 4;

    // Pack important information into 32 bits
    uint packedLayout = NumPlanes;
    packedLayout |= PMaskSizeBits << 5;
#if _XBOX_ONE
    // is32ByteChunk
    packedLayout |= (PackedDepthChunkSize <= 32 ? 1 : 0) << 16;
#endif
    packedLayout |= (PMaskOffset / 4) << 17;
    packedLayout |= planeDataWords << 24;
    return packedLayout;
}

struct ZDataLayout
{
    uint planeCount;        // Number of separate Z planes
    uint indexBits;         // The size of each plane index in bits
#if _XBOX_ONE
    uint is32ByteChunk;     // Some tiles have 32 bytes before plane data
#endif
    uint pMaskOffset;       // Offset to pMask (plane indices)
    uint planeDataWords;    // Size of plane and plane index data in words
};

ZDataLayout GetZDataLayout( uint zMask, uint sampleCountLog2 )
{
    const uint lookup[64] =
    {
        ZLayoutForMask(0x0, 1),	// Clear
        ZLayoutForMask(0x1, 1),
        ZLayoutForMask(0x2, 1),
        ZLayoutForMask(0x3, 1),
        ZLayoutForMask(0x4, 1),
        ZLayoutForMask(0x5, 1),
        ZLayoutForMask(0x6, 1),
        ZLayoutForMask(0x7, 1),
        ZLayoutForMask(0x8, 1),
        ZLayoutForMask(0x9, 1),
        ZLayoutForMask(0xA, 1),
        ZLayoutForMask(0xB, 1),
        ZLayoutForMask(0xC, 1),
        ZLayoutForMask(0xD, 1),
        ZLayoutForMask(0xE, 1),
        ZLayoutForMask(0xF, 1),	// Uncompressed

        ZLayoutForMask(0x0, 2),	// Clear
        ZLayoutForMask(0x1, 2),
        ZLayoutForMask(0x2, 2),
        ZLayoutForMask(0x3, 2),
        ZLayoutForMask(0x4, 2),
        ZLayoutForMask(0x5, 2),
        ZLayoutForMask(0x6, 2),
        ZLayoutForMask(0x7, 2),
        ZLayoutForMask(0x8, 2),
        ZLayoutForMask(0x9, 2),
        ZLayoutForMask(0xA, 2),
        ZLayoutForMask(0xB, 2),
        ZLayoutForMask(0xC, 2),
        ZLayoutForMask(0xD, 2),
        ZLayoutForMask(0xE, 2),
        ZLayoutForMask(0xF, 2),	// Uncompressed

        ZLayoutForMask(0x0, 4),	// Clear
        ZLayoutForMask(0x1, 4),
        ZLayoutForMask(0x2, 4),
        ZLayoutForMask(0x3, 4),
        ZLayoutForMask(0x4, 4),
        ZLayoutForMask(0x5, 4),
        ZLayoutForMask(0x6, 4),
        ZLayoutForMask(0x7, 4),
        ZLayoutForMask(0x8, 4),
        ZLayoutForMask(0x9, 4),
        ZLayoutForMask(0xA, 4),
        ZLayoutForMask(0xB, 4),
        ZLayoutForMask(0xC, 4),
        ZLayoutForMask(0xD, 4),
        ZLayoutForMask(0xE, 4),
        ZLayoutForMask(0xF, 4),	// Uncompressed

        ZLayoutForMask(0x0, 8),	// Clear
        ZLayoutForMask(0x1, 8),
        ZLayoutForMask(0x2, 8),
        ZLayoutForMask(0x3, 8),
        ZLayoutForMask(0x4, 8),
        ZLayoutForMask(0x5, 8),
        ZLayoutForMask(0x6, 8),
        ZLayoutForMask(0x7, 8),
        ZLayoutForMask(0x8, 8),
        ZLayoutForMask(0x9, 8),
        ZLayoutForMask(0xA, 8),
        ZLayoutForMask(0xB, 8),
        ZLayoutForMask(0xC, 8),
        ZLayoutForMask(0xD, 8),
        ZLayoutForMask(0xE, 8),
        ZLayoutForMask(0xF, 8),	// Uncompressed
    };

    uint packedLayout = lookup[sampleCountLog2 * 16 + zMask];

    ZDataLayout layout;
    layout.planeCount = __XB_UBFE(5, 0, packedLayout);
    layout.indexBits = __XB_UBFE(3, 5, packedLayout);
#if _XBOX_ONE
    layout.is32ByteChunk = __XB_UBFE(4, 13, packedLayout);
#endif
    layout.pMaskOffset = __XB_UBFE(7, 17, packedLayout);
    layout.planeDataWords = __XB_UBFE(8, 24, packedLayout);
    return layout;
}

// the coord for the given micro tile index
uint2 PixelCoordFromLinearIndex(uint element)
{
    // This is some tricky bit twiddling.  Too bad it doesn't outperform a table
    // lookup.  :-(
    //element = (__XB_MulU24(element, 0x808101) & 0x10051005) + 0x10001;
    //element |= element >> 9;
    //return uint2( __XB_UBFE(3, 1, element), __XB_UBFE(3, 17, element) );

    return uint2(
    	((element >> 0) & 1) | ((element >> 1) & 2) | ((element >> 2) & 4),
    	((element >> 1) & 1) | ((element >> 2) & 2) | ((element >> 3) & 4));
}

uint2 SwizzleLinearIndex(uint element)
{
    const uint2 lookup[64] =
    {
        PixelCoordFromLinearIndex(8 * 0 + 0),
        PixelCoordFromLinearIndex(8 * 0 + 1),
        PixelCoordFromLinearIndex(8 * 0 + 2),
        PixelCoordFromLinearIndex(8 * 0 + 3),
        PixelCoordFromLinearIndex(8 * 0 + 4),
        PixelCoordFromLinearIndex(8 * 0 + 5),
        PixelCoordFromLinearIndex(8 * 0 + 6),
        PixelCoordFromLinearIndex(8 * 0 + 7),
        
        PixelCoordFromLinearIndex(8 * 1 + 0),
        PixelCoordFromLinearIndex(8 * 1 + 1),
        PixelCoordFromLinearIndex(8 * 1 + 2),
        PixelCoordFromLinearIndex(8 * 1 + 3),
        PixelCoordFromLinearIndex(8 * 1 + 4),
        PixelCoordFromLinearIndex(8 * 1 + 5),
        PixelCoordFromLinearIndex(8 * 1 + 6),
        PixelCoordFromLinearIndex(8 * 1 + 7),
        
        PixelCoordFromLinearIndex(8 * 2 + 0),
        PixelCoordFromLinearIndex(8 * 2 + 1),
        PixelCoordFromLinearIndex(8 * 2 + 2),
        PixelCoordFromLinearIndex(8 * 2 + 3),
        PixelCoordFromLinearIndex(8 * 2 + 4),
        PixelCoordFromLinearIndex(8 * 2 + 5),
        PixelCoordFromLinearIndex(8 * 2 + 6),
        PixelCoordFromLinearIndex(8 * 2 + 7),
        
        PixelCoordFromLinearIndex(8 * 3 + 0),
        PixelCoordFromLinearIndex(8 * 3 + 1),
        PixelCoordFromLinearIndex(8 * 3 + 2),
        PixelCoordFromLinearIndex(8 * 3 + 3),
        PixelCoordFromLinearIndex(8 * 3 + 4),
        PixelCoordFromLinearIndex(8 * 3 + 5),
        PixelCoordFromLinearIndex(8 * 3 + 6),
        PixelCoordFromLinearIndex(8 * 3 + 7),
        
        PixelCoordFromLinearIndex(8 * 4 + 0),
        PixelCoordFromLinearIndex(8 * 4 + 1),
        PixelCoordFromLinearIndex(8 * 4 + 2),
        PixelCoordFromLinearIndex(8 * 4 + 3),
        PixelCoordFromLinearIndex(8 * 4 + 4),
        PixelCoordFromLinearIndex(8 * 4 + 5),
        PixelCoordFromLinearIndex(8 * 4 + 6),
        PixelCoordFromLinearIndex(8 * 4 + 7),
        
        PixelCoordFromLinearIndex(8 * 5 + 0),
        PixelCoordFromLinearIndex(8 * 5 + 1),
        PixelCoordFromLinearIndex(8 * 5 + 2),
        PixelCoordFromLinearIndex(8 * 5 + 3),
        PixelCoordFromLinearIndex(8 * 5 + 4),
        PixelCoordFromLinearIndex(8 * 5 + 5),
        PixelCoordFromLinearIndex(8 * 5 + 6),
        PixelCoordFromLinearIndex(8 * 5 + 7),
        
        PixelCoordFromLinearIndex(8 * 6 + 0),
        PixelCoordFromLinearIndex(8 * 6 + 1),
        PixelCoordFromLinearIndex(8 * 6 + 2),
        PixelCoordFromLinearIndex(8 * 6 + 3),
        PixelCoordFromLinearIndex(8 * 6 + 4),
        PixelCoordFromLinearIndex(8 * 6 + 5),
        PixelCoordFromLinearIndex(8 * 6 + 6),
        PixelCoordFromLinearIndex(8 * 6 + 7),
        
        PixelCoordFromLinearIndex(8 * 7 + 0),
        PixelCoordFromLinearIndex(8 * 7 + 1),
        PixelCoordFromLinearIndex(8 * 7 + 2),
        PixelCoordFromLinearIndex(8 * 7 + 3),
        PixelCoordFromLinearIndex(8 * 7 + 4),
        PixelCoordFromLinearIndex(8 * 7 + 5),
        PixelCoordFromLinearIndex(8 * 7 + 6),
        PixelCoordFromLinearIndex(8 * 7 + 7),
    };

    return lookup[element];
}

// Pack the final float value and optionally handle denormalization
float FinalizeZValue(int exp, uint mantissa)
{
    int shift = firstbithigh(mantissa) - 23;
    exp += shift;

#if DEPTH_UTIL_HANDLE_DENORMS
    // Check for denormals with min exponent
    if (exp < 1)
    {
  #if DEPTH_UTIL_FLUSH_DENORMS
        // Flush to zero
        return 0.0;
  #else
        // Don't delete the implicit 1 and do underflow
        return asfloat((mantissa << (8 - shift)) >> (9 - exp));
  #endif
    }
#endif

    // Delete the implicit one.  Move mantissa to the lower 23 bits
    return min(1.0, asfloat(exp << 23 | (mantissa << (9 - shift)) >> 9));
}

// Interpolate Z on the plane at the specified pixel position.
// The difference from the MSAA version is that pixel position can be expressed
// as 4 bits rather than 7, keeping the computed offset inside 32-bits.  (28 + 4)
float GetDepthFromZPlane(uint3 plane, int2 pixelPos)
{
#if DEPTH_UTIL_APPROXIMATE
    int exp = __XB_UBFE(8, 24, plane.y) + 4;
    int centerZ = 0;
    int depth = __XB_IBFE(28, 3, plane.z);
#else
    int exp = __XB_UBFE(8, 24, plane.y);
    int centerZ = __XB_IBFE(31, 0, plane.z) << 1;
    int depth = 0;
#endif

    // Use the high 24 bits of the 28-bit slopes to offset from the starting depth
    depth = __XB_MadI24(plane.x >> 4, pixelPos.x, depth);
    depth = __XB_MadI24(plane.y, pixelPos.y, depth);

#if !DEPTH_UTIL_APPROXIMATE
    // Use the four remaining bits of slope
    depth = __XB_MadI24(plane.x & 0xF, pixelPos.x, depth << 4);
    depth = __XB_MadI24(plane.x >> 28, pixelPos.y, depth);
#endif

    // Avoid overflowing max positive signed value due to addition addition
    if (depth <= -centerZ)
        return 0.0;

    depth += centerZ;

    return FinalizeZValue(exp, depth);
}

float GetDepthFromZPlaneMSAA(uint3 plane, int2 pixelPos)
{
    int exp = __XB_UBFE(8, 24, plane.y) + 1;
    int depth = __XB_IBFE(31, 0, plane.z);

    // Use the high 24 bits of the 28-bit slopes to offset from the starting depth
    depth = __XB_MadI24(plane.x >> 4, pixelPos.x, depth);
    depth = __XB_MadI24(plane.y, pixelPos.y, depth);

#if !DEPTH_UTIL_APPROXIMATE
    // Use the remaining four bits of slope
    int extra = __XB_MulI24(plane.x & 0xF, pixelPos.x);
    extra = __XB_MadI24(plane.x >> 28, pixelPos.y, extra);
    depth += extra >> 4;

    // We need no more than 24 bits; decide if the 4-bit remainder is needed.
    // Negative values will fail this test and stay negative.
    if ((uint)depth < 0x1000000)
    {
        depth = depth << 4 | (extra & 0xF);
        exp -= 4;
    }
#endif

    if (depth <= 0)
        return 0.0;

    return FinalizeZValue(exp, depth);
}

// Returns the size in words to skip to the start of useful ZData.
uint GetZDataOffset(ZDataLayout layout, uint2 pixelCoord)
{
#if _XBOX_ONE
    // is32ByteChunk is either 0 or 8.  This returns either 8 or 0 depending on the horizontal
    // position of the tile.  
    return layout.is32ByteChunk & pixelCoord.x;
#else
    return 0;
#endif
}

float GetZValue(ZDataLayout layout, uint zDataOffset, uint2 globalPixel, uint flattenedIndex,
    uint numSamples, uint sampleIdx, int2 sampleOffset)
{
    GroupMemoryBarrierWithGroupSync();

    uint planeIndex = 0;
    int2 pixelPos = globalPixel & 7;

    [branch] // scalar branch
    if (layout.planeCount > 1)
    {
        uint element = __XB_MadU24(flattenedIndex, numSamples, sampleIdx);
        uint bitOffset = __XB_MulU24(element, layout.indexBits);
        uint indexOffset = zDataOffset + layout.pMaskOffset + (bitOffset >> 5);

        bitOffset &= 31;

        planeIndex = sharedDepthData[indexOffset] >> bitOffset;
        uint bitsRead = 32 - bitOffset;
        if (layout.indexBits > bitsRead)
            planeIndex |= sharedDepthData[indexOffset + 1] << bitsRead;

        planeIndex &= __XB_BFM(layout.indexBits, 0);

#if _XBOX_ONE
        // This logic is needed on Scorpio to handle an "optimization" without setting the chicken bit.
        if (layout.planeCount == 2 && numSamples == 4)
            planeIndex &= 1;
#endif
    }

    uint planeOffset = __XB_MadU24(planeIndex, 3, zDataOffset);
    uint3 plane = uint3(sharedDepthData[planeOffset], sharedDepthData[planeOffset + 1], sharedDepthData[planeOffset + 2]);

    // probable static branch
    if (numSamples == 1)
    {
        // 4-bit pixel centers are {-7, -5, -3, -1, +1, +3, +5, +7}
        pixelPos = pixelPos * 2 - 7;
        return GetDepthFromZPlane(plane, pixelPos);
    }
    else
    {
        // 7-bit pixel centers are {-56, -40, -24, -8, +8, +24, +40, +56}
        pixelPos = pixelPos * 16 - 56;

        // Is MSAA actually enabled?
        if ((int)plane.z < 0)
            pixelPos += sampleOffset;

        return GetDepthFromZPlaneMSAA(plane, pixelPos);
    }
}

bool IsTileDepthClear(uint zMask)
{
#if DEPTH_UTIL_DEPTH_EXPANDED_CLEAR_MODE
    return zMask == 0 || zMask == 13;
#else
    return zMask == 0;
#endif
}

float DecompressDepth(Texture2D<float> depthSrc,
    uint zMask, uint2 st, uint threadIndex, float depthClear, bool D16 )
{
    // Clear
    if (IsTileDepthClear(zMask))
        return depthClear;

    // Uncompressed
    if (zMask == 15)
        return depthSrc[st];

    ZDataLayout ZLayout = GetZDataLayout(zMask, 0);
    uint zDataOffset = GetZDataOffset(ZLayout, st);
    uint maxLoadIndex = zDataOffset + ZLayout.planeDataWords;

    // Compressed
    if (D16)
    {
        // This doesn't seem to help, probably because there are only 32 words to load in the worst case.
        //if (threadIndex < 2 * maxLoadIndex)
        {
            float PreFetchDepth = depthSrc[st];
            // This does two redundant writes from adjacent threads, but it's branchless.
            sharedDepthData[threadIndex / 2] = __XB_PackF32ToUNORM16(
                __XB_LaneSwizzle(PreFetchDepth, 0x1E),	// threadIndex & ~1
                __XB_LaneSwizzle(PreFetchDepth, 0x3E)); // threadIndex |  1
        }
    }
    else if (threadIndex < maxLoadIndex)
    {
        sharedDepthData[threadIndex] = asuint(depthSrc[st]);
    }

    float Z = GetZValue(ZLayout, zDataOffset, st, threadIndex, 1, 0, 0);

#if DEPTH_UTIL_EMULATE_D16_PRECISION
    if (D16)
        Z = round(Z * 0xFFFF) / 0xFFFF;
#endif

    return Z;
}

float DecompressDepthMSAA(Texture2DMS<float> depthSrc, uint numSamples, uint sampleIdx,
    uint zMask, uint2 st, uint threadIndex, float depthClear, bool D16, bool firstTime, int2 samplePos)
{
    // Clear
    if (IsTileDepthClear(zMask))
        return depthClear;

    // Uncompressed
    if (zMask == 15)
        return depthSrc.Load(st, sampleIdx);

    uint numSamplesLog2 = firstbithigh(numSamples);
    ZDataLayout ZLayout = GetZDataLayout(zMask, numSamplesLog2);
    uint zDataOffset = GetZDataOffset(ZLayout, st);

    if (firstTime)
    {
        uint maxLoadIndex = zDataOffset + ZLayout.planeDataWords;
        uint2 CornerPixel = st & ~0x7;

        // We only need to load the words between zDataOffset and the maxLoadIndex, but we have to 
        // translate those offsets into specific pixel and sample locations.
        if (D16)
        {
            const uint numSamplePairs = numSamples / 2;
            const uint sampleShift = numSamplesLog2 - 1;
            const uint sampleMask = numSamplePairs - 1;

            for (uint iSamplePair = threadIndex + zDataOffset; iSamplePair < maxLoadIndex; iSamplePair += 64)
            {
                uint2 pixelCoord = CornerPixel + SwizzleLinearIndex(iSamplePair >> sampleShift);
                uint sampleIdx = (iSamplePair & sampleMask) * 2;
                sharedDepthData[iSamplePair] = __XB_PackF32ToUNORM16(depthSrc.Load(pixelCoord, sampleIdx),
                    depthSrc.Load(pixelCoord, sampleIdx + 1));
            }
        }
        else
        {
            const uint sampleShift = numSamplesLog2;
            const uint sampleMask = numSamples - 1;

            for (uint iSample = threadIndex + zDataOffset; iSample < maxLoadIndex; iSample += 64)
            {
                uint2 pixelCoord = CornerPixel + SwizzleLinearIndex(iSample >> sampleShift);
                uint sampleIdx = iSample & sampleMask;
                sharedDepthData[iSample] = asuint(depthSrc.Load(pixelCoord, sampleIdx));
            }
        }

        GroupMemoryBarrierWithGroupSync();
    }

    uint flattenedIndex = LinearIndexFromPixelCoord(st & 7);
    float Z = GetZValue(ZLayout, zDataOffset, st, flattenedIndex, numSamples, sampleIdx, samplePos);

#if DEPTH_UTIL_EMULATE_D16_PRECISION
    if (D16)
        Z = round(Z * 0xFFFF) / 0xFFFF;
#endif

    return Z;

}

bool IsTileStencilClear(uint SMem)
{
#if DEPTH_UTIL_STENCIL_EXPANDED_CLEAR_MODE
    // SMem == 0 means Clear, SMem == 2 means Expanded and Clear
    return (SMem & 0x1) == 0;
#else
    return SMem == 0;
#endif
}

uint DecompressStencil( Texture2D<uint> stencilSrc,
    uint sMem, uint2 st, uint threadIndex, uint stencilClear)
{
    // Clear
    if (IsTileStencilClear(sMem))
        return stencilClear;

    // Single
    if (sMem == 1)
    {
        // grab the first stencil value and replicate it
#if _XBOX_ONE
        uint offset = (st.x & 8) * 4;
        return stencilSrc[(st & ~7) | SwizzleLinearIndex(offset)];
#else
        return stencilSrc[st & ~7];
#endif
    }

    // Expanded
    return stencilSrc[st];
}

uint DecompressStencilMSAA( Texture2DMS<uint> stencilSrc, uint numSamples, uint sampleIdx,
    uint sMem, uint2 st, uint threadIndex, uint stencilClear)
{
    // Clear
    if (IsTileStencilClear(sMem))
        return stencilClear;

    // Single
    if (sMem == 1)
    {
        // grab the first stencil value and replicate it
#if _XBOX_ONE
        uint offset = (st.x & 8) * 4 / numSamples;
        return stencilSrc[(st & ~7) | SwizzleLinearIndex(offset)];
#else
        return stencilSrc[st & ~7];
#endif
    }

    // Expanded
    return stencilSrc.Load(st, sampleIdx);
}

#endif // __DEPTH_DECOMPRESS_UTILITY__
