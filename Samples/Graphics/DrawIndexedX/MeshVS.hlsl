//--------------------------------------------------------------------------------------
// MeshVS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Vertex shader for drawing mesh
//--------------------------------------------------------------------------------------

#include "Common.hlsli"
#include "Helper.hlsli"

struct VSInput
{
	float3 position : SV_Position;
	float3 normal   : NORMAL;
	float2 texCoord : TEXCOORD;
};

struct VSOutput
{
	float2 texCoord   : TEXCOORD;
#ifdef DRAW_INDEXED_X 
	uint drawID       : DRAWID0;
#endif
	float4 positionPS : SV_Position;
};

struct VSInputDIX
{
	float3 position0; // DXGI_FORMAT_R32G32B32_FLOAT
	// If using a packed R11G11B10 format for a normal, you will need to unpack it using
	// intrinsics XB_TypedLoad before using the values
	float3 normal0;   // DXGI_FORMAT_R32G32B32_FLOAT
	float2 texCoord0; // DXGI_FORMAT_R32G32_FLOAT
};
StructuredBuffer<VSInputDIX> vsInputDIX : register(t3);

cbuffer Parameters : register(b0)
{
	float4x4 WorldViewProj;
};

// Buffer to store count of total waves
RWStructuredBuffer<uint> bufferWaves : register(u0);

[ROOT_SIGNATURE_MESH]
VSOutput main(VSInput vsIn, uint vertexID : SV_VERTEXID)
{
	VSOutput vsOut;
	uint drawID = 0;
#ifdef DEBUG_DATA
	// Count total waves
	// Get the exec mask and let the first active thread increment the counter
	// to count the number of total waves run for the VS
	uint2 execMask = __XB_GetEntryActiveMask64();
	uint firstActiveThread = GetFirstActiveThread(execMask);

	if (__XB_GetLaneID() == firstActiveThread)
	{
		InterlockedAdd(bufferWaves[BUFFER_OFFSET_TOTAL_WAVES_VS], 1);
	}
#endif

#ifdef DRAW_INDEXED_X
	// Extract vertex ID and draw ID
	// The DrawID can be used in the pixel shader to differentiate between draws
	uint actualVertexID = vertexID & VERTEX_ID_MASK;
	uint vertexIDBits = countbits(VERTEX_ID_MASK);
	uint maxDrawBits = countbits(MAX_DRAWS);
	drawID = __XB_UBFE(maxDrawBits, vertexIDBits, vertexID); //drawID = (vertexID >> vertexIDBits) & MAX_DRAWS;
	vsOut.positionPS = mul(float4(vsInputDIX[actualVertexID].position0, 1.0f), WorldViewProj);
	vsOut.texCoord = vsInputDIX[actualVertexID].texCoord0;
	vsOut.drawID = drawID;
#else
	vsOut.positionPS = mul(float4(vsIn.position.xyz, 1.0f), WorldViewProj);
	vsOut.texCoord = vsIn.texCoord;
#endif

	return vsOut;
}