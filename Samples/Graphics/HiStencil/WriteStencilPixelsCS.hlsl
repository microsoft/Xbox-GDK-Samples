//--------------------------------------------------------------------------------------
// WriteStencilPixelsCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Write Stencil Pixels using a compute shader.
// The shader blends color (green or blue) with the existing target.
// Green indicates that the tile has a single value for stencil - use SGPRs.
// Blue indicates that the tile might have been expanded - use VGPRs.
//--------------------------------------------------------------------------------------
#include "SMemSRValues.hlsli"
#include "RootSignature.hlsli"

// Tile data decoded using a compute shader. Contains tile location and SMem value
StructuredBuffer<uint> TileData : register(t32);

// Use stencil texture when comparing expanded values
Texture2D<uint4> StencilTex : register(t1);

// Output target
RWTexture2D<float4> UAVTex : register(u0);

#define STENCIL_VALUE 0x2

struct StencilParamsStruct
{
	uint StencilValue;
};

ConstantBuffer<StencilParamsStruct> StencilParams : register(b0);

[ROOT_SIGNATURE_MAIN]
[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_GroupID,
	uint3 GTid : SV_GroupThreadID)
{
	// Get pixel coord from HTile
	uint value = __XB_MakeUniform(TileData[DTid.x]);
	uint2 htileCoord = __XB_MakeUniform(uint2(value & 0xFFF, __XB_UBFE(12, 12, value)));
	uint sMem = __XB_MakeUniform(__XB_UBFE(2, 30, value));
	uint2 pixelCoordFromTile = __XB_MakeUniform(htileCoord << 3);

	// Get x, y coordinate from GroupThreadID
	if (sMem == SMEM_SINGLE_VALUE)
	{
		uint2 coordInsideTile;
		coordInsideTile.x = (uint)(GTid % 8);
		coordInsideTile.y = (uint)(GTid / 8);
		UAVTex[pixelCoordFromTile + coordInsideTile] = UAVTex[pixelCoordFromTile + coordInsideTile] * float4(0.5, 0.5, 0.5, 1) + float4(0, 0.25, 0, 0);
	}
	else if (sMem == SMEM_EXPANDED)
	{
		uint2 coordInsideTile;
		coordInsideTile.x = (uint)(GTid % 8);
		coordInsideTile.y = (uint)(GTid / 8);
		uint2 currPixelPos = pixelCoordFromTile + coordInsideTile;
		if (StencilTex[currPixelPos].g >= StencilParams.StencilValue)
		{
			UAVTex[currPixelPos] = UAVTex[currPixelPos] * float4(0.5, 0.5, 0.5, 1) + float4(0, 0, 0.25, 0);
		}
	}
}
