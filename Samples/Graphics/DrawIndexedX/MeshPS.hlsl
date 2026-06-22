//--------------------------------------------------------------------------------------
// MeshPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Pixel shader for drawing mesh. For DrawIndexedX, it uses a texture array. The index 
// is chosen based on data passed in the DrawData struct.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"
#include "Helper.hlsli"

struct PSInput
{
	float2 texCoord : TEXCOORD;
#ifdef DRAW_INDEXED_X 
	uint drawID     : DRAWID0;
#endif
};

// DrawID specific data can be passed into the shader.
// The higher bits of the vertex buffer can be used to pack the drawID
struct DrawData
{
	uint materialID;
	uint baseVertexID; // Unused here - just to show that more data can be available to the Pixel Shader
	uint drawID;       // Unused here 
	uint padding;
};
StructuredBuffer<DrawData> drawData : register(t2);

// Buffer to store count of total and divergent waves
RWStructuredBuffer<uint> bufferWaves : register(u0);

// Texture array. For DrawIndexedInstanced, the correct
// texture will be present at index 0
Texture2D<float4> diffuseTex[] : register(t0, space1);
sampler simpleSampler : register(s0);

// [earlydepthstencil] is forced here as the shader compiler will force LATE_Z
// when UAV writes are on in a Pixel Shader. If this is not forced, there's an 
// increase in the number of waves when running with DEBUG_DATA as that writes 
// to a UAV to count the total and divergent waves
[earlydepthstencil]
[ROOT_SIGNATURE_MESH]
float4 main(PSInput psIn) : SV_TARGET0
{
	float4 color;
	uint materialID;
#ifdef DRAW_INDEXED_X
	materialID = drawData[psIn.drawID].materialID;
#else
	materialID = 0;
#endif

#ifdef DEBUG_DATA
	// Count total waves
	// Get the exec mask and let the first active thread increment the counter
	// to count the number of total waves run for the PS
	uint2 execMask = __XB_GetEntryActiveMask64();
	uint firstActiveThread = GetFirstActiveThread(execMask);

	if (__XB_GetLaneID() == firstActiveThread)
	{
		InterlockedAdd(bufferWaves[BUFFER_OFFSET_TOTAL_WAVES_PS], 1);
	}

	// Count Divergent waves
	// Compare the materialID value for the first active thread with the rest
	// of the threads to check if the wave diverges.
	// Use the first active thread in the wave to count number of Divergent Waves
	uint uniformMaterialID = __XB_MakeUniform(materialID);
    uint2 ballotValue = __XB_Ballot64(materialID != uniformMaterialID);
	bool2 isUniform = (ballotValue == uint2(0, 0));
		
	if (!isUniform.x || !isUniform.y)
	{
		uint firstActiveBallotThread = GetFirstActiveThread(ballotValue);
		if (__XB_GetLaneID() == firstActiveBallotThread)
		{
			InterlockedAdd(bufferWaves[BUFFER_OFFSET_DIVERGENT_WAVES_PS], 1);
		}
#ifdef SHOW_DEBUG_COLORS
		color = float4(1.0f, 0.f, 0.f, 0.f);
		return color;
	}
	else
	{
		color = float4(0.5f, 0.5f, 0.5f, 0.f);
		return color;
#endif
	}
#endif
	
#ifdef DRAW_INDEXED_X
	color = diffuseTex[NonUniformResourceIndex(materialID)].Sample(simpleSampler, psIn.texCoord);
#else
	color = diffuseTex[materialID].Sample(simpleSampler, psIn.texCoord);
#endif
	return color;
}
