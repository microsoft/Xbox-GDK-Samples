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
// Author:  James Stanard 
//

#include "SSAORS.hlsli"
#include "Constants.h"

Texture2D<float> Depth : register(t0);
ByteAddressBuffer HTile : register(t5);

RWTexture2D<float> LinearZ : register(u0);
RWTexture2D<float2> DS2x : register(u1);
RWTexture2DArray<float> DS2xAtlas : register(u2);
RWTexture2D<float2> DS4x : register(u3);
RWTexture2DArray<float> DS4xAtlas : register(u4);
ConstantBuffer<SSAOConstants> CB0 : register(b0);

groupshared float g_CacheW[256];


#define DEPTH_UTIL_APPROXIMATE 1
#include "DepthDecompressUtility.hlsli"


float DecompressAndLinearize(uint2 tile, uint2 localCoord, uint linearIndex)
{
    uint zMask = GetZMask(HTile, CB0.HTileInfo, tile);
    uint2 st = tile << 3 | localCoord;
    float depth = DecompressDepth(Depth, zMask, st, linearIndex, CB0.ZClear);
    float dist = 1.0 / (CB0.ZMagic * depth + 1.0);
    LinearZ[st] = dist;
    return dist;
}


[RootSignature(SSAO_RootSig)]
[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    uint linearIndex = GI & 63;
    uint2 pixelCoord = SwizzleLinearIndex(linearIndex);
    uint destIdx = pixelCoord.x + pixelCoord.y * 16;

    uint2 tile1 = Gid.xy << 1;
    g_CacheW[destIdx + 0] = DecompressAndLinearize(tile1 | uint2(0, 0), pixelCoord, linearIndex);
    g_CacheW[destIdx + 8] = DecompressAndLinearize(tile1 | uint2(1, 0), pixelCoord, linearIndex);
    g_CacheW[destIdx + 128] = DecompressAndLinearize(tile1 | uint2(0, 1), pixelCoord, linearIndex);
    g_CacheW[destIdx + 136] = DecompressAndLinearize(tile1 | uint2(1, 1), pixelCoord, linearIndex);

    GroupMemoryBarrierWithGroupSync();

    uint ldsIndex = (GTid.x << 1) | (GTid.y << 5);

    float w1 = g_CacheW[ldsIndex];

    uint2 st = DTid.xy;
    uint slice = (st.x & 3) | ((st.y & 3) << 2);
    DS2x[st] = w1;
    DS2xAtlas[uint3(st >> 2, slice)] = w1;

    if ((GI & 011) == 0)
    {
        st = DTid.xy >> 1;
        slice = (st.x & 3) | ((st.y & 3) << 2);
        DS4x[st] = w1;
        DS4xAtlas[uint3(st >> 2, slice)] = w1;
    }

}
