//--------------------------------------------------------------------------------------
// ReadHTileCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Read HTile
// Use an append buffer to store values which have the "MayPass" bit set in SR*
// As a debug counter, the shader also calculates if "MayFail" bit is set
// or if SMem indicates that the tile passes and has a single value
//--------------------------------------------------------------------------------------
#include "SMemSRValues.hlsli"
#include "RootSignature.hlsli"

#ifndef ADDITIONAL_COUNTERS
#define ADDITIONAL_COUNTERS 0
#endif

#ifndef INCLUDE_SR1_RESULTS
#define INCLUDE_SR1_RESULTS 0
#endif

ByteAddressBuffer HTile : register(t0);

AppendStructuredBuffer<uint> MayPassBuffer : register(u0);
RWStructuredBuffer<uint> MayPassCountBuffer : register(u1);

#if ADDITIONAL_COUNTERS
AppendStructuredBuffer<uint> MayFailBuffer : register(u2);
// The count buffer for MayFailBuffer will be in u3
AppendStructuredBuffer<uint> SMemSingleValueBuffer : register(u4);
// The count buffer for SMemSingleValueBuffer will be in u5
#endif

struct HTileParamsStruct
{
	uint hTilePitch;
	uint isHTileLinear;
};

ConstantBuffer<HTileParamsStruct> HTileParams : register(b0);

// Get the HTile cordinates from the index used to access the Htile as a buffer
// Use the Htile index value to read coordinates from the depth or stencil buffer
uint2 GetHTileCoordFromHTileIndex(uint hTileIndex)
{
	uint cachelineDimsWidth = 64;
#if XBOX_ONE_X
	uint cachelineDimsHeight = 64;
#else
	uint cachelineDimsHeight = 32;
#endif
	uint2 cachelineDims;

	if (HTileParams.isHTileLinear)
	{
		// Linear mode ignores cachelines, so pretend the whole surface is one cacheline
		cachelineDims = uint2(HTileParams.hTilePitch, 1U << 16);
	}
	else
	{
		cachelineDims = uint2(cachelineDimsWidth, cachelineDimsHeight);
	}

#if XBOX_ONE_X
	uint2 iTileBits[3];
	uint iPipeNumBits = 3;
	const uint2 macroTileDims = uint2(8, 4);
	const uint2 coordTileDims = uint2(8, 8);
#else
	uint2 iTileBits[2];
	uint iPipeNumBits = 2;
	const uint2 macroTileDims = uint2(4, 4);
	const uint2 coordTileDims = uint2(4, 4);
#endif
	const uint iCachelinePitchInMacroTiles = cachelineDims.x / macroTileDims.x;
	uint iMacroTileNumBits = firstbithigh(iCachelinePitchInMacroTiles * cachelineDims.y / macroTileDims.y);
	uint iMacroTileNumBitsLow = 4;
	uint iMacroTileNumBitsHigh = iMacroTileNumBits - iMacroTileNumBitsLow;


	uint iBankNumBits = 2;
	uint iBankBits = __XB_UBFE(iBankNumBits, 0, hTileIndex);// hTileIndex & ((1 >> iBankNumBits) - 1);
	uint iMacroTile = __XB_UBFE(iMacroTileNumBitsLow, iBankNumBits, hTileIndex);
	uint iPipeBits = __XB_UBFE(iPipeNumBits, iBankNumBits + iMacroTileNumBitsLow, hTileIndex); // (hTileIndex >> (iBankNumBits + iMacroTileNumBitsLow)) & ((1 >> iPipeNumBits) - 1);
	iMacroTile |= (__XB_UBFE(iMacroTileNumBitsHigh, iBankNumBits + iMacroTileNumBitsLow + iPipeNumBits, hTileIndex) << iMacroTileNumBitsLow);
	uint iCacheline = __XB_UBFE(32 - (iBankNumBits + iMacroTileNumBits + iPipeNumBits), iBankNumBits + iMacroTileNumBits + iPipeNumBits, hTileIndex);

	iTileBits[1].x = (iBankBits >> 1) & 1;
	iTileBits[0].y = (iBankBits & 1) ^ iTileBits[1].x;

	iTileBits[1].y = ((iPipeBits >> 1) & 1) ^ iTileBits[1].x;
	iTileBits[0].x = (iPipeBits & 1) ^ iTileBits[0].y ^ iTileBits[1].x;

	uint2 coordTile;
	coordTile.x = (iTileBits[0].x) | (iTileBits[1].x << 1);
	coordTile.y = (iTileBits[0].y) | (iTileBits[1].y << 1);
#if XBOX_ONE_X
	if (HTileParams.isHTileLinear)
		iTileBits[2].y = __XB_UBFE(1, iBankNumBits + iMacroTileNumBitsLow + iPipeNumBits + 1, hTileIndex);
	else
		iTileBits[2].y = __XB_UBFE(1, iBankNumBits + iMacroTileNumBitsLow - 1, hTileIndex);

	iTileBits[2].x = ((iPipeBits >> 2) & 1) ^ iTileBits[2].y;
	coordTile.x |= (iTileBits[2].x << 2);
	coordTile.y |= (iTileBits[2].y << 2);
#endif
	uint2 coordMacroTile;
	coordMacroTile.y = iMacroTile / iCachelinePitchInMacroTiles;
	coordMacroTile.x = iMacroTile % iCachelinePitchInMacroTiles;

	uint2 coordSubCacheline;
	coordSubCacheline = coordMacroTile * macroTileDims;
	uint2 extraModValue;
	extraModValue = coordSubCacheline % coordTileDims;
	coordSubCacheline = coordSubCacheline - extraModValue + coordTile;

	uint2 coord;
	if (HTileParams.isHTileLinear)
	{
		coord = coordSubCacheline;
	}
	else
	{
		const uint iHtilePitchInCachelines = ceil(HTileParams.hTilePitch / (1.0f * cachelineDims.x));
		uint2 coordCacheline;
		coordCacheline.y = iCacheline / iHtilePitchInCachelines;
		coordCacheline.x = iCacheline % iHtilePitchInCachelines;
		coord = coordCacheline * cachelineDims + coordSubCacheline;
	}

	return coord;
}

[ROOT_SIGNATURE_MAIN]
[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x == 0)
	{
		// Fill up buffer for the ExecuteIndirect call
		MayPassCountBuffer[1] = 1;
		MayPassCountBuffer[2] = 1;
	}

	uint hTileValue = HTile.Load(DTid.x * 4);

	uint SR0 = __XB_UBFE(2, 4, hTileValue);
	uint SR1 = __XB_UBFE(2, 6, hTileValue);
	uint SMem = __XB_UBFE(2, 8, hTileValue);

#if ADDITIONAL_COUNTERS
	if (((SR0 & STENCIL_MAY_PASS) && (SMem == SMEM_SINGLE_VALUE))
#if INCLUDE_SR1_RESULTS
		|| ((SR1 & STENCIL_MAY_PASS) && (SMem == SMEM_SINGLE_VALUE))
#endif
		)
	{
		SMemSingleValueBuffer.Append(DTid.x);
	}
	if (SR0 == STENCIL_CLEAR
//#if INCLUDE_SR1_RESULTS
//		|| (SR1 & STENCIL_MAY_FAIL)
//#endif
		)
	{
		MayFailBuffer.Append(DTid.x);
	}
#endif
	if (((SMem == SMEM_EXPANDED) || (SMem == SMEM_SINGLE_VALUE)) &&
		((SR0 & STENCIL_MAY_PASS)
#if INCLUDE_SR1_RESULTS
		|| (SR1 & STENCIL_MAY_PASS)
#endif
		))
	{
		uint2 htileCoord = GetHTileCoordFromHTileIndex(DTid.x);

		// |31  30|29       24|23           12|11            0|
		// | SMEM |X X X X X X|  htileCoord.y |  htileCoord.x |  
		uint value = htileCoord.x | (htileCoord.y << 12);
		value |= (SMem << 30);

		MayPassBuffer.Append(value);
	}
}
