//--------------------------------------------------------------------------------------
// PrimeHTileCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "DepthDecompressUtility.hlsli"

#define PrimeHTileRS \
    "CBV(b0, visibility=SHADER_VISIBILITY_ALL),"\
    "DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),"\
    "DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)"

RWBuffer<uint> HTileUAV			: register(u0);
Texture2D depthTexture			: register(t0);

#ifndef g_bStencil
#   define g_bStencil 0
#endif

#ifdef REVERSE_Z
#   define ZDirectionCompare3 __XB_Min3_F32
#else
#   define ZDirectionCompare3 __XB_Max3_F32
#endif

cbuffer HTileParams : register(b0)
{
    uint HTileInfo;
    uint g_iDepthBias;
};

//-------------------------------------------------------------------------------------------------------------
// Encode depth for HTile
//-------------------------------------------------------------------------------------------------------------
uint EncodeDepth(float depth)
{
    const float maxRange = float((1U << 14) - 1);
#ifdef REVERSE_Z
    int iDepth = floor(depth * maxRange);					// smaller values are further away
    //	uint uDepth = max(0, iDepth - g_iDepthBias);			// global bias
#else
    int iDepth = ceil(depth * maxRange);					// larger values are further away
    //	uint uDepth = min(maxRange, iDepth + g_iDepthBias);		// global bias
#endif
    // !!!NOTE!!! sample allows negative global bias which a real title would not do
    uint uDepth = clamp(iDepth + g_iDepthBias, 0, int(maxRange));
    uint encodedValue;

    if (g_bStencil)
    {
        // If stencil is enabled, we have a 14bit near, and a 6bit delta towards the far plane
        // since we only have one depth value, the delta is zero
        // The bottom 4 bits is zMask, which we set to zero for clear	
        encodedValue = (uDepth << 18);
    }
    else
    {
        // If stencil is not enabled, min/max depth is encoded into two 14 bit pairs
        // The bottom 4 bits is zMask, which we set to zero for clear	
        encodedValue = (uDepth << 18) | (uDepth << 4);
    }
    return encodedValue;
}


//-------------------------------------------------------------------------------------------------------------
// Prime HTile from depth texture
//-------------------------------------------------------------------------------------------------------------
[RootSignature(PrimeHTileRS)]
[numthreads(8, 8, 1)]
void PrimeHTileCS(uint2 id : SV_DispatchThreadID)
{
    float depth = depthTexture[id].x;

    HTileUAV[GetHTileAddress(id, HTileInfo) / 4] = EncodeDepth(depth);
}


//-------------------------------------------------------------------------------------------------------------
// Prime HTile from depth texture including per sample bias
//-------------------------------------------------------------------------------------------------------------
groupshared float g_Depths10x10[10][10];

float CalculatePhantom(float B, float C)
{
	return saturate(B - (C - B));
}

// assumes [numthreads(8, 8, 1)]
void PrimeHTilePerSampleBias(uint2 threadId, uint2 groupId, const bool includePhantoms)
{
	uint2 tileId = groupId * 8;

	if ((threadId.x < 5) && (threadId.y < 5))	// 5x load of 4x depths to make up the full 10x10 in LDS
	{
		uint2 ldsId = threadId.xy * 2;			// 0, 2, 4, 6, 8
		uint2 loadId = tileId + ldsId;			// loads may go out of bounds

		// attempted gather but it wasn't faster and the code is less clear
		g_Depths10x10[ldsId.x    ][ldsId.y    ] = depthTexture[int2(loadId.x - 1, loadId.y - 1)].x;
		g_Depths10x10[ldsId.x + 1][ldsId.y    ] = depthTexture[int2(loadId.x    , loadId.y - 1)].x;
		g_Depths10x10[ldsId.x    ][ldsId.y + 1] = depthTexture[int2(loadId.x - 1, loadId.y    )].x;
		g_Depths10x10[ldsId.x + 1][ldsId.y + 1] = depthTexture[int2(loadId.x    , loadId.y    )].x;
	}
	// bang in some ALU between loads and using the results
	uint destIndex = GetHTileAddress(tileId + threadId, HTileInfo) / 4;

	// load from LDS
	float depth00 = g_Depths10x10[threadId.x    ][threadId.y    ];
	float depth01 = g_Depths10x10[threadId.x    ][threadId.y + 1];
	float depth02 = g_Depths10x10[threadId.x    ][threadId.y + 2];
	float depth10 = g_Depths10x10[threadId.x + 1][threadId.y    ];
	float depth11 = g_Depths10x10[threadId.x + 1][threadId.y + 1];
	float depth12 = g_Depths10x10[threadId.x + 1][threadId.y + 2];
	float depth20 = g_Depths10x10[threadId.x + 2][threadId.y    ];
	float depth21 = g_Depths10x10[threadId.x + 2][threadId.y + 1];
	float depth22 = g_Depths10x10[threadId.x + 2][threadId.y + 2];

	// DirectX standard returns 0 if reading out of bounds
	// with reverse-Z tiles out of bounds would be pushed to the far plane, which would lead to things passing
	// with normal-Z tiles out of bounds would be pushed to the near plane, which would be disasterous
	// either way, disallow zero, an alternative would be to use a sampler to read depth and clamp
	float depth = depth11;

	if (includePhantoms)
	{
		depth = depth00 > 0.0 ? ZDirectionCompare3(depth, depth00, CalculatePhantom(depth11, depth00)) : depth;
		depth = depth01 > 0.0 ? ZDirectionCompare3(depth, depth01, CalculatePhantom(depth11, depth01)) : depth;
		depth = depth02 > 0.0 ? ZDirectionCompare3(depth, depth02, CalculatePhantom(depth11, depth02)) : depth;
		depth = depth10 > 0.0 ? ZDirectionCompare3(depth, depth10, CalculatePhantom(depth11, depth10)) : depth;
		depth = depth12 > 0.0 ? ZDirectionCompare3(depth, depth12, CalculatePhantom(depth11, depth12)) : depth;
		depth = depth20 > 0.0 ? ZDirectionCompare3(depth, depth20, CalculatePhantom(depth11, depth20)) : depth;
		depth = depth21 > 0.0 ? ZDirectionCompare3(depth, depth21, CalculatePhantom(depth11, depth21)) : depth;
		depth = depth22 > 0.0 ? ZDirectionCompare3(depth, depth22, CalculatePhantom(depth11, depth22)) : depth;
	}
	else
	{
		depth00 = depth00 > 0.0 ? depth00 : depth;
		depth01 = depth01 > 0.0 ? depth01 : depth;
		depth02 = depth02 > 0.0 ? depth02 : depth;
		depth10 = depth10 > 0.0 ? depth10 : depth;
		depth12 = depth12 > 0.0 ? depth12 : depth;
		depth20 = depth20 > 0.0 ? depth20 : depth;
		depth21 = depth21 > 0.0 ? depth21 : depth;
		depth22 = depth22 > 0.0 ? depth22 : depth;

		depth = ZDirectionCompare3(depth, depth00, depth01);
		depth = ZDirectionCompare3(depth, depth02, depth10);
		depth = ZDirectionCompare3(depth, depth12, depth20);
		depth = ZDirectionCompare3(depth, depth21, depth22);
	}
	HTileUAV[destIndex] = EncodeDepth(depth);
}


[RootSignature(PrimeHTileRS)]
[numthreads(8, 8, 1)]
void PrimeHTileMaxDepthCS(uint2 threadId : SV_GroupThreadId, uint2 groupId : SV_GroupID)
{
	PrimeHTilePerSampleBias(threadId, groupId, false);
}

[RootSignature(PrimeHTileRS)]
[numthreads(8, 8, 1)]
void PrimeHTilePhantomDepthCS(uint2 threadId : SV_GroupThreadId, uint2 groupId : SV_GroupID)
{
	PrimeHTilePerSampleBias(threadId, groupId, true);
}
